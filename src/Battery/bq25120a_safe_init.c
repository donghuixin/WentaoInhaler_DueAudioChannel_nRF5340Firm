#include "bq25120a_safe_init.h"

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

#include "boot_diag.h"

LOG_MODULE_REGISTER(bq25120a_safe, LOG_LEVEL_INF);

/* -------------------------------------------------------------------------
 *
 * BQ25120A pin map (verified against user-provided schematic / netlist):
 *
 *   Schematic net      MCU pin    BQ25120A pin    DTS property
 *   -----------------  ---------  --------------  ----------------------
 *   CC_#CD             P0.17      CD              cd-gpios   active-high
 *   CC_#PG             P0.18      PG#             pg-gpios   active-low
 *   LSCTRL             P0.14      LSCTRL          load-switch.enable-gpios
 *   CC_INT             P0.19      INT#            int-gpios  active-low
 *   I2C (IIC0)         P0.21/24   SDA/SCL         i2c1 bus
 *
 * BQ25120A datasheet note on CD:
 *   "CD high -> device operational (I2C accessible).
 *    CD low  -> device in high-Z, I2C disabled."
 * DTS uses GPIO_ACTIVE_HIGH so gpio_pin_set_dt(cd_pin, 1) drives the pin
 * physically HIGH = chip operational = I2C accessible. Confirmed.
 *
 * BQ25120A LSCTRL pin behaviour:
 *   The LSCTRL HW pin only enables/disables the VOUTLS output buffer.
 *   It does NOT gate I2C access to register 0x07; I2C register writes
 *   are gated by CD, not LSCTRL. So we can program LSCTRL register while
 *   the LSCTRL hw pin is LOW (output disabled) - this is in fact the
 *   safest order: register first, then enable output via the pin.
 *
 *   In this board, P0.14 doubles as the Zephyr `load_switch_3_3` device
 *   enable GPIO. board_init.c configures P0.14 as GPIO_OUTPUT_INACTIVE
 *   (= LOW) at POST_KERNEL prio 80, BEFORE we ever run. The PowerManager
 *   later calls pm_device_runtime_enable(ls_3_3) followed by
 *   pm_device_runtime_get(ls_3_3) which drives P0.14 HIGH and finally
 *   enables VOUTLS. So by the time we're called, P0.14 == LOW (= VOUTLS
 *   in high-Z) and we are safe to program LSCTRL register without
 *   any rail glitches.
 *
 * Reg 0x07 (LSCTRL) bit layout per BQ25120A SLUSCD9 table 8-13:
 *   bit 7      EN_LS_LDO   1 = output ON, 0 = output high-Z
 *   bits 6:2   LS_LDO[4:0] voltage code, V = 0.8 + code * 0.1 V
 *                          codes 0..25 valid -> 0.8 V .. 3.3 V LDO
 *                          code 31 (0b11111) = special LS passthrough
 *                                              (VOUTLS = VINLS = VBAT/VBUS)
 *   bits 1:0   reserved
 *
 * POR default = 0xFC = EN=1, code=31 (LS passthrough). To get 3.3 V LDO:
 *   code = (3.3 - 0.8) / 0.1 = 25 = 0b11001
 *   target reg value = (1 << 7) | (25 << 2) = 0x80 | 0x64 = 0xE4
 *
 * ------------------------------------------------------------------------- */

#define BQ25120A_NODE        DT_NODELABEL(bq25120a)

#if !DT_NODE_EXISTS(BQ25120A_NODE)
#error "DTS node `bq25120a` is missing; cannot run BQ25120A safe init"
#endif

#if !DT_NODE_HAS_PROP(BQ25120A_NODE, cd_gpios)
#error "DTS node `bq25120a` is missing `cd-gpios`; CD pin is required to wake PMIC I2C"
#endif

#define BQ25120A_REG_STATUS            0x00U
#define BQ25120A_REG_FAULT             0x01U
#define BQ25120A_REG_TSCTRL            0x02U
#define BQ25120A_REG_FAST_CHG          0x03U
#define BQ25120A_REG_TERM_PRECHG       0x04U
#define BQ25120A_REG_BAT_VOLT          0x05U
#define BQ25120A_REG_SYSVOUT           0x06U
#define BQ25120A_REG_LSCTRL            0x07U
#define BQ25120A_REG_PUSHBTN           0x08U
#define BQ25120A_REG_ILIM_UVLO         0x09U
#define BQ25120A_REG_VBMON             0x0AU

#define BQ25120A_LSCTRL_EN_BIT         (1U << 7)
#define BQ25120A_LSCTRL_CODE_MASK      (0x1FU << 2)
#define BQ25120A_LSCTRL_CODE_3V3       (25U << 2)        /* 0x64 */
#define BQ25120A_LSCTRL_3V3_LDO        \
	(BQ25120A_LSCTRL_EN_BIT | BQ25120A_LSCTRL_CODE_3V3) /* 0xE4 */
#define BQ25120A_LSCTRL_CODE_PASSTHRU  (31U << 2)        /* 0x7C */

/* Timing per BQ25120A datasheet section 7.5 and "PowerOn Sequence":
 *   t_wakeup (CD low->high to I2C ready) <= 1 ms typical, allow 5 ms
 *   t_settle (register write to internal latch) ~ a few hundred us, allow 5 ms
 * We use generous margins because the BLE log already incurs ms-scale
 * delays anyway, and being too aggressive here can mask real failures.
 */
#define BQ25120A_CD_WAKE_DELAY_MS      20
#define BQ25120A_LDO_SETTLE_DELAY_MS   20
#define BQ25120A_LDO_WRITE_RETRIES     4

static const struct device *const i2c_bus =
	DEVICE_DT_GET(DT_BUS(BQ25120A_NODE));

static const struct gpio_dt_spec cd_pin =
	GPIO_DT_SPEC_GET(BQ25120A_NODE, cd_gpios);

static const uint16_t bq25120a_addr = DT_REG_ADDR(BQ25120A_NODE);

#if DT_NODE_HAS_PROP(BQ25120A_NODE, pg_gpios)
static const struct gpio_dt_spec pg_pin =
	GPIO_DT_SPEC_GET(BQ25120A_NODE, pg_gpios);
#define BQ25120A_HAS_PG 1
#else
#define BQ25120A_HAS_PG 0
#endif

/* The 1V8 (P1.11) and 3V3 / VOUTLS (P0.14) enable lines:
 *   ls_1v8_en -> board top-level `load_switch` (gpio1.11)
 *   ls_3v3_en -> child of bq25120a `load-switch` (gpio0.14, == LSCTRL pin)
 */
#define LS_1V8_NODE DT_NODELABEL(load_switch)
#define LS_3V3_NODE DT_CHILD(BQ25120A_NODE, load_switch)

#if DT_NODE_EXISTS(LS_1V8_NODE) && DT_NODE_HAS_PROP(LS_1V8_NODE, enable_gpios)
static const struct gpio_dt_spec ls_1v8_en =
	GPIO_DT_SPEC_GET(LS_1V8_NODE, enable_gpios);
#define BQ25120A_HAS_LS_1V8 1
#else
#define BQ25120A_HAS_LS_1V8 0
#endif

#if DT_NODE_EXISTS(LS_3V3_NODE) && DT_NODE_HAS_PROP(LS_3V3_NODE, enable_gpios)
static const struct gpio_dt_spec ls_3v3_en =
	GPIO_DT_SPEC_GET(LS_3V3_NODE, enable_gpios);
#define BQ25120A_HAS_LS_3V3 1
#else
#define BQ25120A_HAS_LS_3V3 0
#endif

#if BQ25120A_HAS_PG
static struct gpio_callback pg_cb_data;
static bool pg_isr_installed;
#endif

#if BQ25120A_HAS_LS_1V8 || BQ25120A_HAS_LS_3V3
static void force_rails_off(void);
#endif

/* Single-byte register write. Layout on the wire is [reg, val] in one
 * transaction with no restart, exactly matching what i2c_burst_write
 * (used by the legacy driver) emits. We use i2c_write directly so we
 * can log raw return codes without going through the wrapper.
 */
static int write_reg(uint8_t reg, uint8_t val)
{
	uint8_t buf[2] = { reg, val };

	int ret = i2c_write(i2c_bus, buf, sizeof(buf), bq25120a_addr);
	if (ret != 0) {
		LOG_ERR("BQ25120A: I2C write reg 0x%02X = 0x%02X failed (%d)",
			reg, val, ret);
		BOOT_DIAG("PMIC: I2C write 0x%02X<-0x%02X failed ret=%d",
			  reg, val, ret);
	}
	return ret;
}

/* Single-byte register read using the standard I2C "write-restart-read"
 * pattern. Returns the value via *out, or leaves *out untouched on error.
 */
static int read_reg(uint8_t reg, uint8_t *out)
{
	int ret = i2c_write_read(i2c_bus, bq25120a_addr, &reg, sizeof(reg),
				 out, sizeof(*out));
	if (ret != 0) {
		LOG_ERR("BQ25120A: I2C read reg 0x%02X failed (%d)", reg, ret);
		BOOT_DIAG("PMIC: I2C read 0x%02X failed ret=%d", reg, ret);
	}
	return ret;
}

/* Decode LSCTRL into a human-readable description and millivolt estimate.
 * The OUTPUT state is gated by EN bit AND the LSCTRL HW pin, so when EN=0
 * we report "high-Z (output disabled)" instead of confusing the user with
 * a "passthrough" string that only matters when EN=1.
 */
static void log_lsctrl_decode(const char *label, uint8_t v)
{
	const unsigned int en   = (v & 0x80U) ? 1U : 0U;
	const unsigned int code = (v >> 2) & 0x1FU;

	if (en == 0U) {
		BOOT_DIAG("PMIC %s LSCTRL=0x%02X (EN=0 code=%u -> output disabled at I2C level)",
			  label, v, code);
		return;
	}

	if (code == 31U) {
		BOOT_DIAG("PMIC %s LSCTRL=0x%02X (EN=1 code=31 -> LS passthrough = VINLS = V_PMID)",
			  label, v);
	} else {
		const unsigned int mv = 800U + code * 100U;
		BOOT_DIAG("PMIC %s LSCTRL=0x%02X (EN=1 code=%u -> LDO %u mV)",
			  label, v, code, mv);
	}
}

/* Strategy descriptions kept short so they fit in the boot-diag line buffer. */
static const char *lsctrl_strategy_name(int attempt)
{
	switch (attempt) {
	case 1:  return "direct";
	case 2:  return "EN-first-then-code";
	case 3:  return "CD-pulse";
	case 4:  return "CD-pulse+EN-first";
	default: return "retry";
	}
}

int bq25120a_safe_init_ldo_3v3(void)
{
	int ret;

	/* ------------------------------------------------------------------
	 * 1) Sanity-check the DT bindings and GPIO ports we depend on.
	 * ------------------------------------------------------------------ */
	if (!device_is_ready(i2c_bus)) {
		LOG_ERR("BQ25120A: I2C bus %s not ready", i2c_bus->name);
		BOOT_DIAG("PMIC: I2C bus not ready");
		return -ENODEV;
	}
	if (!device_is_ready(cd_pin.port)) {
		LOG_ERR("BQ25120A: CD GPIO port %s not ready", cd_pin.port->name);
		BOOT_DIAG("PMIC: CD GPIO port not ready");
		return -ENODEV;
	}

	BOOT_DIAG("PMIC safe init: bus=%s addr=0x%02X CD=%s%u PG=%s%u",
		  i2c_bus->name, bq25120a_addr,
		  cd_pin.port->name, cd_pin.pin,
#if BQ25120A_HAS_PG
		  pg_pin.port->name, pg_pin.pin
#else
		  "n/a", 0U
#endif
	);

	/* ------------------------------------------------------------------
	 * 2) Log the pre-existing GPIO state for P0.14 (LSCTRL), P0.17 (CD),
	 *    P0.18 (PG). board_init.c already configured P0.14 as
	 *    GPIO_OUTPUT_INACTIVE (LOW) so VOUTLS is guaranteed in high-Z
	 *    while we re-program LSCTRL register safely.
	 * ------------------------------------------------------------------ */
#if BQ25120A_HAS_LS_3V3
	int ls_pin_raw = gpio_pin_get_raw(ls_3v3_en.port, ls_3v3_en.pin);
	BOOT_DIAG("PMIC: LSCTRL pin (%s%u) raw=%d (LOW = VOUTLS high-Z, safe for programming)",
		  ls_3v3_en.port->name, ls_3v3_en.pin, ls_pin_raw);
#endif

	/* ------------------------------------------------------------------
	 * 3) Drive CD high -> chip operational -> I2C accessible.
	 *    We keep CD high for the rest of the boot; if the application
	 *    later wants to put the chip into high-Z it can drive CD low.
	 * ------------------------------------------------------------------ */
	ret = gpio_pin_configure_dt(&cd_pin, GPIO_OUTPUT_ACTIVE);
	if (ret != 0) {
		LOG_ERR("BQ25120A: configure CD pin failed (%d)", ret);
		BOOT_DIAG("PMIC: CD pin configure failed ret=%d", ret);
		return ret;
	}
	k_msleep(BQ25120A_CD_WAKE_DELAY_MS);
	BOOT_DIAG("PMIC: CD=HIGH (chip operational); waited %d ms for t_wakeup",
		  BQ25120A_CD_WAKE_DELAY_MS);

	/* ------------------------------------------------------------------
	 * 4) Read all 11 user registers BEFORE writing anything. This is
	 *    the chip's OTP fingerprint - useful both for diagnostics
	 *    (compare against datasheet POR defaults to verify OPN) and
	 *    for the user to inspect via the BLE log.
	 * ------------------------------------------------------------------ */
	uint8_t por[11];
	for (uint8_t r = 0; r < 11; r++) {
		por[r] = 0xFFU;
		(void)read_reg(r, &por[r]);
	}
	BOOT_DIAG("PMIC POR dump:");
	BOOT_DIAG("  R00 STATUS  =0x%02X  R01 FAULT  =0x%02X  R02 TSCTRL =0x%02X",
		  por[0], por[1], por[2]);
	BOOT_DIAG("  R03 FASTCHG =0x%02X  R04 TERM   =0x%02X  R05 BATV   =0x%02X",
		  por[3], por[4], por[5]);
	BOOT_DIAG("  R06 SYSVOUT =0x%02X  R07 LSCTRL =0x%02X  R08 PUSHBTN=0x%02X",
		  por[6], por[7], por[8]);
	BOOT_DIAG("  R09 ILIM    =0x%02X  R0A VBMON  =0x%02X",
		  por[9], por[10]);
	log_lsctrl_decode("POR", por[BQ25120A_REG_LSCTRL]);

	/* ------------------------------------------------------------------
	 * 5) Program LSCTRL = LDO 3.3 V.
	 *
	 *    POR observation from real hardware:
	 *      Some BQ25120A OPNs (notably the BQ25120AYFPR we have in this
	 *      batch) ship with LSCTRL POR = 0x7C, i.e. EN_LS_LDO bit = 0
	 *      at power-up. Other batches ship with the generic POR = 0xFC
	 *      (EN = 1). The legacy "preserve EN bit, overwrite code bits"
	 *      sequence happens to work on the 0xFC variant (it ends up
	 *      writing 0xE4) but FAILS on the 0x7C variant because it ends
	 *      up writing 0x64 (EN = 0, code = 25). Many OPNs silently
	 *      ignore voltage-code writes when EN = 0 ("you're turning the
	 *      LDO off, code value is irrelevant") and keep the OTP default
	 *      code (= 31, LS passthrough).
	 *
	 *    Fix: ALWAYS write target = 0xE4 explicitly, i.e. force EN = 1
	 *    AND code = 25 in the same byte. The LSCTRL HW pin (P0.14)
	 *    is still held LOW by board_init.c at this point, so VOUTLS
	 *    stays in high-Z - the chip just remembers "next time the HW
	 *    enable pin goes high, deliver 3.3 V LDO".
	 *
	 *    Try up to 4 different strategies before giving up:
	 *      1. Direct write 0xE4
	 *      2. Write 0x80 (set EN=1 only), settle, then write 0xE4
	 *      3. Pulse CD low->high (full chip wake-up), then write 0xE4
	 *      4. CD pulse + EN-first + final write
	 * ------------------------------------------------------------------ */
	uint8_t verify = por[BQ25120A_REG_LSCTRL];
	bool latched   = false;

	for (int attempt = 1; attempt <= BQ25120A_LDO_WRITE_RETRIES; attempt++) {
		const char *strategy = lsctrl_strategy_name(attempt);

		/* Optional chip-side preconditioning before the actual write. */
		if (attempt == 3 || attempt == 4) {
			BOOT_DIAG("PMIC: LSCTRL attempt %d (%s): pulsing CD low->high",
				  attempt, strategy);
			(void)gpio_pin_set_dt(&cd_pin, 0);
			k_msleep(50);
			(void)gpio_pin_set_dt(&cd_pin, 1);
			k_msleep(BQ25120A_CD_WAKE_DELAY_MS);
		}

		if (attempt == 2 || attempt == 4) {
			BOOT_DIAG("PMIC: LSCTRL attempt %d (%s): pre-write 0x80 (EN=1 only)",
				  attempt, strategy);
			(void)write_reg(BQ25120A_REG_LSCTRL, BQ25120A_LSCTRL_EN_BIT);
			k_msleep(BQ25120A_LDO_SETTLE_DELAY_MS);
		}

		uint8_t cur = 0xFFU;
		(void)read_reg(BQ25120A_REG_LSCTRL, &cur);

		BOOT_DIAG("PMIC: LSCTRL attempt %d (%s): cur=0x%02X -> write 0x%02X (EN=1, code=25 forced)",
			  attempt, strategy, cur, BQ25120A_LSCTRL_3V3_LDO);

		ret = write_reg(BQ25120A_REG_LSCTRL, BQ25120A_LSCTRL_3V3_LDO);
		if (ret != 0) {
			k_msleep(BQ25120A_LDO_SETTLE_DELAY_MS);
			continue;
		}

		k_msleep(BQ25120A_LDO_SETTLE_DELAY_MS);

		ret = read_reg(BQ25120A_REG_LSCTRL, &verify);
		if (ret != 0) {
			BOOT_DIAG("PMIC: LSCTRL verify read failed on attempt %d", attempt);
			continue;
		}

		const unsigned int verify_code = (verify >> 2) & 0x1FU;
		const unsigned int verify_en   = (verify & 0x80U) ? 1U : 0U;
		if (verify_code == 25U) {
			latched = true;
			BOOT_DIAG("PMIC: LSCTRL latched (attempt %d, %s): EN=%u code=25 readback=0x%02X",
				  attempt, strategy, verify_en, verify);
			break;
		}

		BOOT_DIAG("PMIC: LSCTRL attempt %d (%s) not latched: wrote 0x%02X, read 0x%02X (EN=%u code=%u)",
			  attempt, strategy, BQ25120A_LSCTRL_3V3_LDO, verify,
			  verify_en, verify_code);
	}

	log_lsctrl_decode("FINAL", verify);

	/* ------------------------------------------------------------------
	 * 6) Outcome handling.
	 *    - latched=true  -> LDO is programmed to 3.3 V. The Zephyr
	 *                       load_switch_3_3 device will drive LSCTRL pin
	 *                       HIGH later when PowerManager calls
	 *                       pm_device_runtime_get(ls_3_3), and VOUTLS
	 *                       will rise to 3.3 V LDO.
	 *    - latched=false -> chip kept bits 6:2 at 31 (LS passthrough).
	 *                       On boards where VINLS is wired to V_PMID,
	 *                       enabling VOUTLS in this state would put
	 *                       VBAT (~4 V) or VBUS (~5 V) onto the 3V3
	 *                       rail. We force the LSCTRL pin LOW (= VOUTLS
	 *                       in high-Z) by tearing down ls_3_3 enable
	 *                       GPIO directly. Downstream 3V3 loads must
	 *                       be powered another way.
	 * ------------------------------------------------------------------ */
	uint8_t snap_status   = 0xFF;
	uint8_t snap_fault    = 0xFF;
	uint8_t snap_sysvout  = 0xFF;
	uint8_t snap_ilim     = 0xFF;

	(void)read_reg(BQ25120A_REG_STATUS,    &snap_status);
	(void)read_reg(BQ25120A_REG_FAULT,     &snap_fault);
	(void)read_reg(BQ25120A_REG_SYSVOUT,   &snap_sysvout);
	(void)read_reg(BQ25120A_REG_ILIM_UVLO, &snap_ilim);

	BOOT_DIAG("PMIC post-init: STAT=0x%02X FAULT=0x%02X SYSVOUT=0x%02X ILIM=0x%02X",
		  snap_status, snap_fault, snap_sysvout, snap_ilim);

	if (latched) {
		LOG_INF("BQ25120A: safe init OK, LSCTRL programmed for 3.3 V LDO (0x%02X)",
			verify);
		BOOT_DIAG("PMIC OK: LSCTRL=3.3 V LDO; enable via ls_3_3 PM device for VOUTLS up");
		return 0;
	}

	LOG_ERR("BQ25120A: LSCTRL did not latch 3.3 V code after %d attempts (last=0x%02X)",
		BQ25120A_LDO_WRITE_RETRIES, verify);
	BOOT_DIAG("PMIC FAIL: LSCTRL refused 3.3 V LDO code (last 0x%02X)", verify);
	BOOT_DIAG("PMIC FAIL: chip is in LS passthrough; VOUTLS would = VINLS (~VBAT/VBUS)");
	BOOT_DIAG("PMIC FAIL: forcing ls_3_3 (LSCTRL pin) LOW to protect 3V3 loads");

#if BQ25120A_HAS_LS_1V8 || BQ25120A_HAS_LS_3V3
	force_rails_off();
#endif

	return -EIO;
}

/* ------------------------------------------------------------------------- */
/* VBUS rail lockout                                                         */
/* ------------------------------------------------------------------------- */

#if BQ25120A_HAS_LS_1V8 || BQ25120A_HAS_LS_3V3
static void force_rails_off(void)
{
#if BQ25120A_HAS_LS_3V3
	if (device_is_ready(ls_3v3_en.port)) {
		(void)gpio_pin_configure_dt(&ls_3v3_en, GPIO_OUTPUT_INACTIVE);
	}
#endif
#if BQ25120A_HAS_LS_1V8
	if (device_is_ready(ls_1v8_en.port)) {
		(void)gpio_pin_configure_dt(&ls_1v8_en, GPIO_OUTPUT_INACTIVE);
	}
#endif
}
#endif

#if BQ25120A_HAS_PG
static void vbus_pg_isr(const struct device *port, struct gpio_callback *cb,
			uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	if (!IS_ENABLED(CONFIG_BQ25120A_VBUS_RAIL_LOCKOUT)) {
		return;
	}

	int pg = gpio_pin_get_dt(&pg_pin);
	if (pg == 1) {
#if BQ25120A_HAS_LS_1V8 || BQ25120A_HAS_LS_3V3
		force_rails_off();
#endif
		BOOT_DIAG("VBUS guard ISR: USB plug detected, rails forced OFF");
	}
}
#endif

int bq25120a_vbus_safety_check(void)
{
	if (!IS_ENABLED(CONFIG_BQ25120A_VBUS_RAIL_LOCKOUT)) {
		BOOT_DIAG("VBUS guard: disabled by Kconfig");
		return 0;
	}

#if !BQ25120A_HAS_PG
	BOOT_DIAG("VBUS guard: PG pin not in DTS, skipping");
	return -ENODEV;
#else
	if (!device_is_ready(pg_pin.port)) {
		BOOT_DIAG("VBUS guard: PG port not ready");
		return -ENODEV;
	}

	int ret = gpio_pin_configure_dt(&pg_pin, GPIO_INPUT);
	if (ret != 0) {
		BOOT_DIAG("VBUS guard: PG configure failed ret=%d", ret);
		return ret;
	}

	int pg = gpio_pin_get_dt(&pg_pin);
	if (pg < 0) {
		BOOT_DIAG("VBUS guard: PG read failed ret=%d", pg);
		return pg;
	}

	if (pg != 1) {
		BOOT_DIAG("VBUS guard: PG inactive (battery only); rails not locked");
		return 0;
	}

	LOG_WRN("VBUS detected at boot - locking out 3V3 / 1V8 rails to protect "
		"downstream loads from leaked 5 V");
	BOOT_DIAG("!! VBUS DETECTED: USB plugged in at boot !!");
	BOOT_DIAG("VBUS guard: 3V3 net may carry leaked 5V; locking out rails");

#if BQ25120A_HAS_LS_3V3 || BQ25120A_HAS_LS_1V8
	force_rails_off();
#endif
#if BQ25120A_HAS_LS_3V3
	BOOT_DIAG("VBUS guard: ls_3_3 enable (P0.14) -> LOW (off)");
#endif
#if BQ25120A_HAS_LS_1V8
	BOOT_DIAG("VBUS guard: ls_1_8 enable (P1.11) -> LOW (off)");
#endif
	BOOT_DIAG("VBUS guard: unplug USB and reboot to restore rails");

	return 1;
#endif
}

int bq25120a_vbus_guard_install_isr(void)
{
	if (!IS_ENABLED(CONFIG_BQ25120A_VBUS_RAIL_LOCKOUT)) {
		return 0;
	}

#if !BQ25120A_HAS_PG
	BOOT_DIAG("VBUS guard ISR: no PG pin, not installed");
	return -ENODEV;
#else
	if (pg_isr_installed) {
		return 0;
	}

	if (!device_is_ready(pg_pin.port)) {
		BOOT_DIAG("VBUS guard ISR: PG port not ready");
		return -ENODEV;
	}

	gpio_init_callback(&pg_cb_data, vbus_pg_isr, BIT(pg_pin.pin));
	int ret = gpio_add_callback(pg_pin.port, &pg_cb_data);
	if (ret != 0) {
		BOOT_DIAG("VBUS guard ISR: add_callback failed ret=%d", ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&pg_pin, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		BOOT_DIAG("VBUS guard ISR: int_configure failed ret=%d", ret);
		(void)gpio_remove_callback(pg_pin.port, &pg_cb_data);
		return ret;
	}

	pg_isr_installed = true;
	BOOT_DIAG("VBUS guard ISR: armed on PG edge -> ACTIVE");
	return 0;
#endif
}
