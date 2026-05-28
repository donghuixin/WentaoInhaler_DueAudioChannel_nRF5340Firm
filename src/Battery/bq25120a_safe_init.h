/*
 * BQ25120A safe initialization for the LDO / Load-Switch output (VOUTLS).
 *
 * Hardware context:
 *   - The BQ25120A boots with register 0x07 (LSCTRL) defaulted to LS
 *     passthrough mode (code=31). In that mode VOUTLS = VINLS, which on
 *     this board is V_PMID (max of VBAT and VBUS) -> 4 V to 5 V on the
 *     "3.3 V" rail.
 *   - VOUTLS feeds the 3.3 V rail that powers MLX90642 (Vmax = 3.6 V) and
 *     the PCA9306 high side. Driving > 3.6 V there will damage MLX90642.
 *
 * Pin mapping confirmed against board schematic / netlist:
 *      CC_#CD  -> P0.17 (cd-gpios)    -> chip enable (active-high in DTS)
 *      CC_#PG  -> P0.18 (pg-gpios)    -> power good output (active-low)
 *      LSCTRL  -> P0.14 (load-switch.enable-gpios) -> VOUTLS HW enable
 *      I2C     -> i2c1 (= IIC0)       -> SDA P0.21, SCL P0.24
 *
 * This function MUST run as the very first I2C transaction the MCU issues
 * to the PMIC after reset. Sequence:
 *   1. Verify I2C bus + CD GPIO ready
 *   2. Drive CD high (chip operational) and wait t_wakeup
 *   3. Snapshot all 11 user registers (POR fingerprint, posted to BLE log)
 *   4. Read-modify-write register 0x07 -> preserve EN bit, set voltage code
 *      to 25 (= 3.3 V LDO). Retries up to 3x with 20 ms settle in between.
 *   5. If readback shows code != 25 after all retries the chip is in LS
 *      passthrough; force LS_3V3 / LS_1V8 enable GPIOs LOW so VOUTLS does
 *      not get gated through.
 */

#ifndef BQ25120A_SAFE_INIT_H_
#define BQ25120A_SAFE_INIT_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Safely bring up BQ25120A VOUTLS as a 3.3 V LDO.
 *
 * @return 0 on success.
 *         -ENODEV if the I2C bus or CD GPIO device is not ready.
 *         -EIO    on readback mismatch.
 *         Negative errno on GPIO / I2C transaction failure.
 */
int bq25120a_safe_init_ldo_3v3(void);

/**
 * Sample the BQ25120A PG (power-good) pin and, if VBUS is present, drive
 * the ls_1_8 / ls_3_3 enable GPIOs low to prevent leaked USB 5 V from
 * reaching 3V3 / 1V8 downstream loads (MLX90642, sensors, etc.).
 *
 * Intended for boards where a TVS / cap / PMIC FET has been shown to leak
 * VBUS onto the 3V3 net. Gated by CONFIG_BQ25120A_VBUS_RAIL_LOCKOUT;
 * disable that Kconfig only after the hardware path has been repaired.
 *
 * @return  1 if VBUS detected and rails were locked out.
 *          0 if VBUS not detected (battery-only).
 *          -ENODEV / -errno on GPIO failure.
 */
int bq25120a_vbus_safety_check(void);

/**
 * Install an interrupt callback on the PG pin so the same lockout fires
 * whenever USB is plugged in at runtime. Must be called after the PG GPIO
 * has been configured as input (BQ25120a::begin already does that).
 *
 * @return 0 on success, negative errno on failure.
 */
int bq25120a_vbus_guard_install_isr(void);

#ifdef __cplusplus
}
#endif

#endif /* BQ25120A_SAFE_INIT_H_ */
