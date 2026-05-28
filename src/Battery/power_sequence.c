#include "power_sequence.h"
#include "bq25120a_safe_init.h"

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "boot_diag.h"

LOG_MODULE_REGISTER(power_sequence, LOG_LEVEL_INF);

/* P0.14 == BQ25120A LSCTRL pin == load_switch_3_3 enable.
 *
 * Hardware topology on the post-rework board (confirmed 2026-05-27):
 *
 *   VBAT --> BQ25120A --> VOUTLS (P0.14 LSCTRL gates it)
 *                            |
 *                            +--> SGM2036 VIN
 *                                  |     EN tied to VIN (always on while VIN > UVLO)
 *                                  +--> SGM2036 VOUT = stable 3.3 V LDO
 *                                        |
 *                                        +--> MLX90642 VDD
 *                                        +--> PCA9306 VREF2 (and EN, which is
 *                                              tied to VREF2 on this board)
 *
 * Two implications follow:
 *
 *   A) SGM2036 is a true LDO. Whether VOUTLS is the regulated 3.3 V LDO
 *      output of BQ25120A reg 0x07 = 0xE4 *or* the POR passthrough value
 *      (~4-5 V VINLS), SGM2036 still clamps its output to 3.3 V. The
 *      MLX90642 (Vmax 3.6 V) is therefore protected against BQ25120A
 *      mis-programming by the LDO downstream.
 *
 *   B) SGM2036 stops the moment VOUTLS goes high-Z, because its EN is
 *      tied to VIN. If we hold LSCTRL LOW (P0.14 = 0), MLX90642 has no
 *      VDD at all. So the worst possible "protection" we can implement
 *      here is to keep LSCTRL LOW after a failed reg 0x07 write -- that
 *      would brick the sensor instead of save it.
 *
 * Conclusion: we still take LSCTRL LOW *briefly* while reprogramming
 * reg 0x07 (so the rail can settle to the new code rather than glitching
 * between passthrough and LDO), but we ALWAYS bring LSCTRL HIGH at the
 * end of the sequence -- even on safe_init failure -- so SGM2036 keeps
 * sourcing 3.3 V to MLX90642 / PCA9306. The LDO downstream is what
 * actually protects MLX90642 from over-voltage, not LSCTRL.
 */
#define LS_3V3_NODE DT_CHILD(DT_NODELABEL(bq25120a), load_switch)

#if DT_NODE_EXISTS(LS_3V3_NODE) && DT_NODE_HAS_PROP(LS_3V3_NODE, enable_gpios)
static const struct gpio_dt_spec ls_3v3_en =
	GPIO_DT_SPEC_GET(LS_3V3_NODE, enable_gpios);
#define POWER_SEQ_HAS_LS_3V3 1
#else
#define POWER_SEQ_HAS_LS_3V3 0
#endif

/* DELIBERATE BEHAVIOUR CHANGE (2026-05-27, revised):
 *
 * History of bugs in this file:
 *
 *   v1 (original): drove LSCTRL HIGH *before* programming reg 0x07,
 *                  never asserted CD. reg 0x07 write always failed
 *                  (`ret=-5`) so VOUTLS stayed in POR passthrough.
 *
 *   v2 (first fix): forced LSCTRL LOW for the duration of safe_init,
 *                   re-armed it HIGH on success but kept it LOW on
 *                   failure "to protect MLX90642 from over-voltage".
 *
 *   v3 (this one): On the production board, MLX90642 / PCA9306 sit
 *                  behind a dedicated SGM2036 LDO whose VIN is VOUTLS
 *                  and whose EN is tied to VIN. That means:
 *                    - SGM2036 already protects the sensors from a
 *                      passthrough VOUTLS (it regulates down to 3.3 V).
 *                    - But SGM2036 needs *some* voltage on its VIN; if
 *                      we leave LSCTRL LOW (v2 "protection" path), VIN
 *                      goes to ~0 V and MLX90642 has no rail at all.
 *                  So v2 actually disabled the sensor it was trying
 *                  to protect. v3 always brings LSCTRL HIGH at the end
 *                  of the sequence and relies on SGM2036 to clamp the
 *                  rail to 3.3 V even if reg 0x07 programming failed.
 *
 * bq25120a_safe_init_ldo_3v3() still:
 *   - asserts CD with t_wakeup
 *   - keeps LSCTRL LOW while it programs reg 0x07 = 0xE4 (3.3 V LDO),
 *     with 4 retry strategies
 *   - on permanent failure, leaves rails in safe state (passthrough
 *     remains, but downstream LDO will clip)
 *
 * If reg 0x07 = 0xE4 (regulated LDO): SGM2036 sees ~3.3 V VIN, regulated
 *   output ~3.3 V (just below dropout, but stable enough for MLX90642).
 *   Better: many SGM2036 LDOs require >=0.3 V headroom; you may want
 *   to set BQ25120A LDO to a slightly higher voltage (e.g. 3.4 V) on
 *   future boards. For now we keep 3.3 V because the LDO works at
 *   sub-dropout with reduced load regulation.
 *
 * If reg 0x07 stays 0xFC (POR passthrough): SGM2036 sees 4-5 V VIN,
 *   regulates cleanly to 3.3 V. MLX90642 is still safe; the only
 *   penalty is ~10 mA continuous LDO loss heating the SGM2036.
 */
static int power_sequence_init(void)
{
	/* Step 1: tear down what board_init.c did to P0.14 because of
	 * `default-on`. We want VOUTLS in high-Z briefly so that when
	 * reg 0x07 transitions from POR passthrough to 3.3 V LDO the rail
	 * doesn't glitch through a brief over-voltage spike.
	 *
	 * Note: this temporarily drops MLX90642 VDD to 0 V via SGM2036.
	 * MLX90642 datasheet allows arbitrary power-cycling; the POR
	 * delay we add in MLX90642::begin() (250 ms) already covers a
	 * fresh bootstrap, so this is harmless.
	 */
#if POWER_SEQ_HAS_LS_3V3
	if (gpio_is_ready_dt(&ls_3v3_en)) {
		int gret = gpio_pin_configure_dt(&ls_3v3_en, GPIO_OUTPUT_INACTIVE);
		BOOT_DIAG("PMIC power_seq: LSCTRL %s%u forced LOW (was HIGH due to default-on), ret=%d",
			  ls_3v3_en.port->name, ls_3v3_en.pin, gret);
		/* Give the rail a moment to actually drop. */
		k_msleep(5);
	}
#endif

	/* Step 2: run the proper safe-init sequence (CD up, snapshot, 4-attempt
	 * retry of reg 0x07 = 0xE4). On failure it leaves the rails in their
	 * own safe state internally, but we then override that below because
	 * the SGM2036 downstream protects MLX90642 either way.
	 */
	int ret = bq25120a_safe_init_ldo_3v3();

	/* Step 3 (UNCONDITIONAL): bring LSCTRL back HIGH so VOUTLS is alive,
	 * which is the only way SGM2036 (and therefore MLX90642 / PCA9306)
	 * gets a VIN. SGM2036 LDO will clamp whatever VOUTLS is to 3.3 V.
	 *
	 * This used to be inside the `ret == 0` branch only, which meant a
	 * failed I2C programming would brick the sensor by leaving it
	 * completely unpowered. See v2 history above.
	 */
#if POWER_SEQ_HAS_LS_3V3
	if (gpio_is_ready_dt(&ls_3v3_en)) {
		int gret2 = gpio_pin_configure_dt(&ls_3v3_en, GPIO_OUTPUT_ACTIVE);
		BOOT_DIAG("PMIC power_seq: LSCTRL %s%u driven HIGH (VOUTLS -> SGM2036 VIN), ret=%d",
			  ls_3v3_en.port->name, ls_3v3_en.pin, gret2);
		/* power-delay-us in DTS = 600us, MLX90642 POR takes longer
		 * but that wait happens in MLX90642::begin().
		 */
		k_msleep(5);
	}
#endif

	if (ret == 0) {
		BOOT_DIAG("PMIC power_seq: complete (BQ25120A reg 0x07 = 0xE4 -> 3.3 V LDO; SGM2036 VIN healthy)");
		LOG_INF("Power sequence complete");
	} else {
		/* Even on failure, MLX90642 is still safe because SGM2036
		 * downstream regulates passthrough V_PMID (4-5 V) down to
		 * 3.3 V. Log loudly so the user can see this in hw_status
		 * and decide whether to investigate the I2C path to BQ25120A.
		 */
		BOOT_DIAG("PMIC power_seq: safe init returned %d -- BQ25120A may be in passthrough, SGM2036 still regulates 3.3 V downstream",
			  ret);
		LOG_WRN("Power sequence: safe init returned %d; SGM2036 protects MLX90642 anyway", ret);
	}

	return 0;
}

/* Priority 81 - runs AFTER board_init.c load-switch device init (priority
 * 80) so we can override its `default-on` driven HIGH on P0.14 if needed.
 * Still well before the sensor stack (priority 90+).
 */
SYS_INIT(power_sequence_init, POST_KERNEL, 81);

/* Backwards-compatible API. The legacy helper just delegates now. */
void force_load_switches_on(void)
{
	int ret = bq25120a_safe_init_ldo_3v3();
	if (ret != 0) {
		LOG_ERR("force_load_switches_on: safe_init failed ret=%d", ret);
		BOOT_DIAG("PMIC force_load_switches_on: safe_init ret=%d", ret);
	}
}
