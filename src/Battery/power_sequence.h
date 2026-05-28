#ifndef POWER_SEQUENCE_H_
#define POWER_SEQUENCE_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Early-boot PMIC bring-up used on boards where BQ25120A LSCTRL register
 * voltage-code bits only latch when the LSCTRL hardware pin is asserted.
 *
 * Sequence (matches XinyiIMU5340FirmWare):
 *   1. Drive LSCTRL enable GPIO (P0.14) HIGH
 *   2. Wait for PMIC / rail settle
 *   3. I2C write LSCTRL (0x07) = 0xE4 (3.3 V LDO, EN=1)
 */
void force_load_switches_on(void);

#ifdef __cplusplus
}
#endif

#endif /* POWER_SEQUENCE_H_ */
