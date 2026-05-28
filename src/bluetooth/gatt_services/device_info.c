#include "device_info.h"
#include <generated/version.h>

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#include "../../Battery/BootState.h"
#include "../../utils/uicr.h"
#include "../../utils/boot_diag.h"
#include "openearable_common.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

static char device_identifier[sizeof(uint64_t) * 2 + 3];
char device_generation[16];
static char firmware[] = FIRMWARE_VERSION;
/* HW status JSON must be large enough for full I2C scans (3 buses x up to 112
 * addresses) plus the boot diagnostic ring (up to 32 lines x ~100 chars).
 */
static char hw_status[8192];

#define I2C_ADDR_MIN 0x08
#define I2C_ADDR_MAX 0x77
#define I2C_SCAN_MAX_FOUND (I2C_ADDR_MAX - I2C_ADDR_MIN + 1)

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c1), okay)
static const struct device *const iic0_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));
#else
static const struct device *const iic0_dev = NULL;
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c2), okay)
static const struct device *const iic1_dev = DEVICE_DT_GET(DT_NODELABEL(i2c2));
#else
static const struct device *const iic1_dev = NULL;
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c3), okay)
static const struct device *const iic2_dev = DEVICE_DT_GET(DT_NODELABEL(i2c3));
#else
static const struct device *const iic2_dev = NULL;
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(adau1860), enable_gpios)
static const struct gpio_dt_spec adau_enable_pin =
	GPIO_DT_SPEC_GET(DT_NODELABEL(adau1860), enable_gpios);
#define ADAU_ENABLE_AVAILABLE 1
#else
#define ADAU_ENABLE_AVAILABLE 0
#endif

struct probe_power_guard {
	bool got_1v8;
	bool got_3v3;
};

static size_t appendf(char *buf, size_t buf_size, size_t pos, const char *fmt, ...)
{
	if (pos >= buf_size) {
		return buf_size;
	}

	va_list ap;
	va_start(ap, fmt);
	int written = vsnprintf(&buf[pos], buf_size - pos, fmt, ap);
	va_end(ap);

	if (written < 0) {
		return pos;
	}

	size_t next = pos + (size_t)written;
	if (next >= buf_size) {
		return buf_size - 1;
	}
	return next;
}

static bool bus_ready(const struct device *bus)
{
	return bus != NULL && device_is_ready(bus);
}

static bool probe_i2c_address(const struct device *bus, uint8_t addr)
{
	if (!bus_ready(bus)) {
		return false;
	}

	uint8_t dummy = 0;
	int ret = i2c_read(bus, &dummy, sizeof(dummy), addr);
	if (ret == 0) {
		return true;
	}

	/* Fallback for register-based devices that NACK plain read. */
	uint8_t reg = 0x00;
	ret = i2c_write_read(bus, addr, &reg, sizeof(reg), &dummy, sizeof(dummy));
	return ret == 0;
}

static void probe_power_up(struct probe_power_guard *guard)
{
	if (guard == NULL) {
		return;
	}

	guard->got_1v8 = (pm_device_runtime_get(ls_1_8) == 0);
	guard->got_3v3 = (pm_device_runtime_get(ls_3_3) == 0);

#if ADAU_ENABLE_AVAILABLE
	if (device_is_ready(adau_enable_pin.port)) {
		/* ADAU1860 may not ACK until PD/enable goes high. */
		(void)gpio_pin_configure_dt(&adau_enable_pin, GPIO_OUTPUT_ACTIVE);
	}
#endif

	/* Settle delay before probing.
	 *
	 * The previous 5 ms was enough for ADAU1860 / fast slaves but FAR
	 * too short for MLX90642: per datasheet 3.2.2 the device needs
	 *   T_valid_data = 10 ms (Init) + 70 ms (max) + RT (up to 500 ms @ 2 Hz)
	 * before it will respond, because its EEPROM->RAM bootstrap can
	 * tail-end on top of the supply ramp + PCA9306 EN settle. Giving
	 * 250 ms here is the smallest value that consistently lets
	 * MLX90632 / MLX90642 / MAXM86161 (all on the IIC1 PCA9306 side)
	 * ACK during the scan.
	 */
	k_msleep(250);
}

static void probe_power_down(const struct probe_power_guard *guard)
{
	if (guard == NULL) {
		return;
	}

	if (guard->got_3v3) {
		(void)pm_device_runtime_put(ls_3_3);
	}
	if (guard->got_1v8) {
		(void)pm_device_runtime_put(ls_1_8);
	}
}

static uint8_t scan_i2c_bus(const struct device *bus, const char *label,
			    uint8_t *found, uint8_t found_cap)
{
	if (!bus_ready(bus)) {
		BOOT_DIAG("scan %s: bus not ready", label);
		return 0;
	}

	const uint32_t t0_ms = (uint32_t)k_uptime_get_32();
	uint8_t total = 0;
	for (uint8_t addr = I2C_ADDR_MIN; addr <= I2C_ADDR_MAX; addr++) {
		if (probe_i2c_address(bus, addr)) {
			if (found != NULL && total < found_cap) {
				found[total] = addr;
			}
			BOOT_DIAG("scan %s: ACK at 0x%02X", label, addr);
			total++;
		}
	}
	const uint32_t dt_ms = (uint32_t)k_uptime_get_32() - t0_ms;
	BOOT_DIAG("scan %s: %u device(s) in %u ms", label,
		  (unsigned int)total, (unsigned int)dt_ms);
	return total;
}

/* Slow-mode rescan of the PCA9306 high-side devices on IIC1.
 *
 * Background: a brand new MLX90642 (and two other 3.3V sensors behind the
 * same level shifter) are NACKing at 400 kHz, while SDA2/SCL2 measures
 * 2.5 V statically (should be 3.3 V if NMOS is fully cut off). That looks
 * like the PCA9306 is leaking ~170 uA, slewing the rising edge too slow
 * for FM-mode I2C. Per MLX90642 datasheet section 4.6:
 *
 *   "Capacitive loading on an I2C can degrade the communication. ...
 *    Another option might be to go for a slower communication (clock
 *    speed), as the MLX90642 implements Schmidt triggers on its inputs
 *    in I2C compatible mode and is therefore not really sensitive to
 *    rise time of the bus."
 *
 * So we reconfigure the IIC1 bus down to 100 kHz, probe the four 3.3V
 * addresses, then restore the 400 kHz config. If a sensor ACKs only at
 * 100 kHz, that proves the failure mode is rise-time / PCA9306, not
 * something fundamentally broken about the chip wiring or VDD.
 */
static void slow_rescan_high_side(void)
{
	if (!bus_ready(iic1_dev)) {
		return;
	}

	const uint32_t fast_cfg = I2C_SPEED_SET(I2C_SPEED_FAST) | I2C_MODE_CONTROLLER;
	const uint32_t slow_cfg = I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER;

	int cret = i2c_configure(iic1_dev, slow_cfg);
	if (cret != 0) {
		BOOT_DIAG("IIC1 slow rescan: reconfigure to 100 kHz failed ret=%d", cret);
		return;
	}
	BOOT_DIAG("IIC1 slow rescan @ 100 kHz (rise-time tolerance test):");

	const bool mlx66 = probe_i2c_address(iic1_dev, 0x66);
	const bool mlx33 = probe_i2c_address(iic1_dev, 0x33);
	const bool mlx32 = probe_i2c_address(iic1_dev, 0x3A);
	const bool max86 = probe_i2c_address(iic1_dev, 0x62);
	const bool adau  = probe_i2c_address(iic1_dev, 0x64);

	BOOT_DIAG("  @100kHz: MLX90642 0x66=%s 0x33=%s  MLX90632 0x3A=%s",
		  mlx66 ? "ACK" : "NACK",
		  mlx33 ? "ACK" : "NACK",
		  mlx32 ? "ACK" : "NACK");
	BOOT_DIAG("  @100kHz: MAXM86161 0x62=%s  ADAU1860 0x64=%s",
		  max86 ? "ACK" : "NACK",
		  adau  ? "ACK" : "NACK");

	if ((mlx66 || mlx33 || mlx32 || max86) && !adau) {
		BOOT_DIAG("  -> 3V3 sensors ACK only at 100 kHz: PCA9306 slew "
			  "limited. Fix by smaller pull-ups (2.2k) on SDA2/SCL2.");
	} else if (mlx66 || mlx33 || mlx32 || max86) {
		BOOT_DIAG("  -> At least one 3V3 sensor ACKs at 100 kHz. "
			  "Drop IIC1 to 100 kHz in DTS, or shrink high-side pull-ups.");
	} else {
		BOOT_DIAG("  -> Still NACK at 100 kHz. Not a rise-time issue. "
			  "Look at VREF1 / PCA9306 EN / cold solder.");
	}

	/* Restore 400 kHz so other drivers don't see a surprise speed change. */
	int rret = i2c_configure(iic1_dev, fast_cfg);
	if (rret != 0) {
		BOOT_DIAG("IIC1 slow rescan: restore to 400 kHz FAILED ret=%d "
			  "-- IIC1 stuck at 100 kHz until reboot!", rret);
	}
}

static void build_hw_status_report(void)
{
	struct probe_power_guard pwr = {0};
	probe_power_up(&pwr);

	uint8_t found0[I2C_SCAN_MAX_FOUND] = {0};
	uint8_t found1[I2C_SCAN_MAX_FOUND] = {0};
	uint8_t found2[I2C_SCAN_MAX_FOUND] = {0};
	BOOT_DIAG("scan: start (IIC0+IIC1+IIC2)");
	const uint8_t n0 = scan_i2c_bus(iic0_dev, "IIC0", found0, ARRAY_SIZE(found0));
	const uint8_t n1 = scan_i2c_bus(iic1_dev, "IIC1", found1, ARRAY_SIZE(found1));
	const uint8_t n2 = scan_i2c_bus(iic2_dev, "IIC2", found2, ARRAY_SIZE(found2));

	const bool bq27220_iic0_ok = probe_i2c_address(iic0_dev, 0x55);
	const bool bq27220_iic1_ok = probe_i2c_address(iic1_dev, 0x55);
	const bool bq27220_iic2_ok = probe_i2c_address(iic2_dev, 0x55);
	const bool bq27220_ok = bq27220_iic0_ok || bq27220_iic1_ok || bq27220_iic2_ok;
	const char *bq27220_bus = bq27220_iic0_ok ? "IIC0" : (bq27220_iic1_ok ? "IIC1" : (bq27220_iic2_ok ? "IIC2" : "IIC0"));
	const bool bq25120a_iic0_ok = probe_i2c_address(iic0_dev, 0x6A);
	const bool bq25120a_iic1_ok = probe_i2c_address(iic1_dev, 0x6A);
	const bool bq25120a_iic2_ok = probe_i2c_address(iic2_dev, 0x6A);
	const bool bq25120a_ok = bq25120a_iic0_ok || bq25120a_iic1_ok || bq25120a_iic2_ok;
	const char *bq25120a_bus = bq25120a_iic0_ok ? "IIC0" : (bq25120a_iic1_ok ? "IIC1" : (bq25120a_iic2_ok ? "IIC2" : "IIC0"));
	const bool bmi270_iic0_ok = probe_i2c_address(iic0_dev, 0x68);
	const bool bmi270_iic1_ok = probe_i2c_address(iic1_dev, 0x68);
	const bool bmi270_iic2_ok = probe_i2c_address(iic2_dev, 0x68);
	const bool bmi270_ok = bmi270_iic0_ok || bmi270_iic1_ok || bmi270_iic2_ok;
	const char *bmi270_bus = bmi270_iic0_ok ? "IIC0" : (bmi270_iic1_ok ? "IIC1" : (bmi270_iic2_ok ? "IIC2" : "IIC0"));
	const bool adau1860_iic0_ok = probe_i2c_address(iic0_dev, 0x64);
	const bool adau1860_iic1_ok = probe_i2c_address(iic1_dev, 0x64);
	const bool adau1860_iic2_ok = probe_i2c_address(iic2_dev, 0x64);
	const bool adau1860_ok = adau1860_iic0_ok || adau1860_iic1_ok || adau1860_iic2_ok;
	const char *adau1860_bus = adau1860_iic0_ok ? "IIC0" : (adau1860_iic1_ok ? "IIC1" : (adau1860_iic2_ok ? "IIC2" : "IIC0"));
	const bool mlx90642_66_ok = probe_i2c_address(iic1_dev, 0x66);
	const bool mlx90642_33_ok = probe_i2c_address(iic1_dev, 0x33);
	const bool mlx90642_ok = mlx90642_66_ok || mlx90642_33_ok;
	const uint8_t mlx90642_addr = mlx90642_66_ok ? 0x66 : (mlx90642_33_ok ? 0x33 : 0x66);
	const bool maxm86161_ok = probe_i2c_address(iic1_dev, 0x62);
	const bool mlx90632_ok = probe_i2c_address(iic1_dev, 0x3A);

	/* PCA9306 high-side diagnostic.
	 *
	 * On this board the IIC1 (P1.00/P1.15) bus is split by a PCA9306DCUR
	 * level translator: the low side (VREF1 = 1.8 V, pin 2) is shared with
	 * ADAU1860 (0x64) and the nRF5340, while the high side (VREF2 = 3.3 V,
	 * fed by SGM2036, pin 7) carries MLX90642 (0x66), MLX90632 (0x3A) and
	 * MAXM86161 (0x62).
	 *
	 * IMPORTANT (per TI PCA9306 datasheet SCPS113C section 10):
	 *   "The VREF2 pin must be connected to the VDPU power supply through a
	 *    200-kOhm resistor. Failure to have a high impedance resistor
	 *    between VREF2 and VDPU results in excessive current draw and
	 *    unreliable device operation."
	 *
	 * Symptom of missing 200k: SDA2/SCL2 sit at ~2.5 V statically (not
	 * 3.3 V) because the internal charge pump can't lift the gate -- the
	 * pass NMOS stays half-open and the 4.7 k high-side pull-up shares
	 * current with the 1.8 V low-side pull-up through the partially-on
	 * NMOS. Result: every 3.3 V sensor NACKs even though every voltage
	 * "looks" correct at first glance.
	 *
	 * Reference: TI SCPS113C, fig 8-1 (correct) vs fig 8-2 (incorrect).
	 *
	 * If ADAU1860 ACKs but none of the high-side devices does, the failure
	 * is in the PCA9306 setup -- VREF2 missing 200 k, EN dead, no high-side
	 * pull-ups, or cold solder on SCL2/SDA2. If at least one high-side
	 * device ACKs, the level translator is working and the missing device
	 * is a single chip / single-pin issue.
	 *
	 * Emit this as BOOT_DIAG so the web UI shows it without rebuild.
	 */
	const unsigned int high_side_total =
		(unsigned int)mlx90642_66_ok + (unsigned int)mlx90642_33_ok +
		(unsigned int)maxm86161_ok + (unsigned int)mlx90632_ok;
	BOOT_DIAG("PCA9306 high-side probe (IIC1, 3.3V via SGM2036):");
	BOOT_DIAG("  MLX90642 @0x66 = %s, @0x33 = %s",
		  mlx90642_66_ok ? "ACK" : "NACK",
		  mlx90642_33_ok ? "ACK" : "NACK");
	BOOT_DIAG("  MLX90632 @0x3A = %s,  MAXM86161 @0x62 = %s",
		  mlx90632_ok ? "ACK" : "NACK",
		  maxm86161_ok ? "ACK" : "NACK");
	BOOT_DIAG("  ADAU1860 @0x64 on IIC1 = %s (low side, 1.8V)",
		  adau1860_iic1_ok ? "ACK" : "NACK");
	if (adau1860_iic1_ok && high_side_total == 0U) {
		/* All three 3V3 sensors NACK at 400 kHz. Run a 100 kHz rescan to
		 * distinguish slew-rate problem from a dead level shifter. */
		slow_rescan_high_side();

		BOOT_DIAG("VERDICT: PCA9306 high-side NOT conducting at 400 kHz.");
		BOOT_DIAG("  3 different 3.3V sensors all NACK -> not a single-chip fault.");
		BOOT_DIAG("  PCA9306 pin map (all TI packages): 1=GND 2=VREF1 3=SCL1");
		BOOT_DIAG("    4=SDA1 5=SDA2 6=SCL2 7=VREF2 8=EN");
		BOOT_DIAG("  Most common root cause on this kind of board:");
		BOOT_DIAG("   VREF2 (pin 7) directly shorted to 3.3 V rail.");
		BOOT_DIAG("   TI datasheet SCPS113C sec 10 REQUIRES a 200k resistor");
		BOOT_DIAG("   between VREF2 and 3.3V (VDPU). Direct short kills the");
		BOOT_DIAG("   internal charge pump -> NMOS stays half-on -> SDA2/SCL2");
		BOOT_DIAG("   static droop to ~2.5 V instead of clean 3.3 V.");
		BOOT_DIAG("  Correct wiring: VREF2 + EN shorted -> 200k -> 3.3V VDPU,");
		BOOT_DIAG("   with optional 100 pF from VREF2 to GND.");
		BOOT_DIAG("  Verify static voltages (LSCTRL HIGH, no I2C traffic):");
		BOOT_DIAG("   - MLX90642 pin 2 VDD     = 3.3 V");
		BOOT_DIAG("   - PCA9306 pin 2 VREF1    = 1.8 V");
		BOOT_DIAG("   - PCA9306 pin 7 VREF2    = ~2.4 V (after 200k, NOT 3.3 V)");
		BOOT_DIAG("   - PCA9306 pin 8 EN       = same as VREF2 (~2.4 V)");
		BOOT_DIAG("   - PCA9306 pin 5/6 SDA/SCL2 = 3.3 V static (high-side pull-ups)");
	} else if (high_side_total > 0U && !mlx90642_66_ok && !mlx90642_33_ok) {
		BOOT_DIAG("VERDICT: PCA9306 high side IS conducting but MLX90642 silent.");
		BOOT_DIAG("  Other 3.3V sensors ACKed, so the level shifter works.");
		BOOT_DIAG("  Likely a single-chip issue:");
		BOOT_DIAG("   (a) MLX90642 VDD not connected to SGM2036 output,");
		BOOT_DIAG("   (b) MLX90642 SDA/SCL not bonded to PCA9306 SCL2/SDA2,");
		BOOT_DIAG("   (c) chip defective (rare on a brand new part).");
	} else if (mlx90642_ok) {
		BOOT_DIAG("VERDICT: MLX90642 ACK at 0x%02X -- PMIC + level shifter +"
			  " wiring all good.", mlx90642_addr);
	}

	size_t pos = 0;
	memset(hw_status, 0, sizeof(hw_status));

	pos = appendf(hw_status, sizeof(hw_status), pos,
		      "{\"i2c\":[{\"bus\":\"IIC0\",\"ready\":%d,\"count\":%u,\"found\":[",
		      bus_ready(iic0_dev) ? 1 : 0, n0);
	for (uint8_t i = 0; i < MIN(n0, ARRAY_SIZE(found0)); i++) {
		pos = appendf(hw_status, sizeof(hw_status), pos, "%s\"0x%02X\"",
			      i == 0 ? "" : ",", found0[i]);
	}
	pos = appendf(hw_status, sizeof(hw_status), pos,
		      "]},{\"bus\":\"IIC1\",\"ready\":%d,\"count\":%u,\"found\":[",
		      bus_ready(iic1_dev) ? 1 : 0, n1);
	for (uint8_t i = 0; i < MIN(n1, ARRAY_SIZE(found1)); i++) {
		pos = appendf(hw_status, sizeof(hw_status), pos, "%s\"0x%02X\"",
			      i == 0 ? "" : ",", found1[i]);
	}
	pos = appendf(hw_status, sizeof(hw_status), pos,
		      "]},{\"bus\":\"IIC2\",\"ready\":%d,\"count\":%u,\"found\":[",
		      bus_ready(iic2_dev) ? 1 : 0, n2);
	for (uint8_t i = 0; i < MIN(n2, ARRAY_SIZE(found2)); i++) {
		pos = appendf(hw_status, sizeof(hw_status), pos, "%s\"0x%02X\"",
			      i == 0 ? "" : ",", found2[i]);
	}

	pos = appendf(hw_status, sizeof(hw_status), pos,
		      "]}],\"devices\":["
		      "{\"name\":\"bq27220_fuel_gauge\",\"bus\":\"%s\",\"addr\":\"0x55\",\"ok\":%d},"
		      "{\"name\":\"bq25120a_pm\",\"bus\":\"%s\",\"addr\":\"0x6A\",\"ok\":%d},"
		      "{\"name\":\"bmi270_imu\",\"bus\":\"%s\",\"addr\":\"0x68\",\"ok\":%d},"
		      "{\"name\":\"adau1860_audio_codec\",\"bus\":\"%s\",\"addr\":\"0x64\",\"ok\":%d},"
		      "{\"name\":\"mlx90642_thermal\",\"bus\":\"IIC1\",\"addr\":\"0x%02X\",\"ok\":%d},"
		      "{\"name\":\"maxm86161_ppg\",\"bus\":\"IIC1\",\"addr\":\"0x62\",\"ok\":%d},"
		      "{\"name\":\"mlx90632_opt_temp\",\"bus\":\"IIC1\",\"addr\":\"0x3A\",\"ok\":%d}"
		      "],\"derived\":{"
		      "\"fuel_gauge\":%d,"
		      "\"imu\":%d,"
		      "\"microphone_inner\":%d,"
		      "\"microphone_outer\":%d,"
		      "\"thermal_ir\":%d,"
		      "\"ppg\":%d,"
		      "\"opt_temp\":%d"
		      "},\"diag\":{",
		      bq27220_bus,
		      bq27220_ok ? 1 : 0,
		      bq25120a_bus,
		      bq25120a_ok ? 1 : 0,
		      bmi270_bus,
		      bmi270_ok ? 1 : 0,
		      adau1860_bus,
		      adau1860_ok ? 1 : 0,
		      mlx90642_addr,
		      mlx90642_ok ? 1 : 0,
		      maxm86161_ok ? 1 : 0,
		      mlx90632_ok ? 1 : 0,
		      bq27220_ok ? 1 : 0,
		      bmi270_ok ? 1 : 0,
		      adau1860_ok ? 1 : 0,
		      adau1860_ok ? 1 : 0,
		      mlx90642_ok ? 1 : 0,
		      maxm86161_ok ? 1 : 0,
		      mlx90632_ok ? 1 : 0);

	/* Append the boot/scan diagnostic ring as ,"diag":{"dropped":N,"log":[...]}.
	 * boot_diag_emit_json writes the contents of the "diag" object, and we
	 * close the outer report object below.
	 */
	pos = boot_diag_emit_json(hw_status, sizeof(hw_status), pos);
	pos = appendf(hw_status, sizeof(hw_status), pos, "}}");

	hw_status[sizeof(hw_status) - 1] = '\0';
	probe_power_down(&pwr);
}

static ssize_t read_device_identifier(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr,
			  void *buf,
			  uint16_t len,
			  uint16_t offset)
{
	snprintf(device_identifier, sizeof(device_identifier), "0x%08X", oe_boot_state.device_id);
	
	return bt_gatt_attr_read(conn, attr, buf, len, offset, device_identifier,
					 sizeof(device_identifier));
}

static ssize_t read_device_generation(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr,
			  void *buf,
			  uint16_t len,
			  uint16_t offset)
{
	uicr_hw_revision_get(device_generation);
	
	return bt_gatt_attr_read(conn, attr, buf, len, offset, device_generation,
					 strlen(device_generation));
}

static ssize_t read_firmware(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr,
			  void *buf,
			  uint16_t len,
			  uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, firmware,
					 sizeof(firmware));
}

static ssize_t read_hw_status(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr,
			  void *buf,
			  uint16_t len,
			  uint16_t offset)
{
	build_hw_status_report();
	return bt_gatt_attr_read(conn, attr, buf, len, offset, hw_status,
				 strlen(hw_status));
}

BT_GATT_SERVICE_DEFINE(device_svc,
BT_GATT_PRIMARY_SERVICE(BT_UUID_DEVICE_INFO),
BT_GATT_CHARACTERISTIC(BT_UUID_IDENT,
            BT_GATT_CHRC_READ,
            BT_GATT_PERM_READ,
            read_device_identifier, NULL, device_identifier),
BT_GATT_CHARACTERISTIC(BT_UUID_GENERATION,
            BT_GATT_CHRC_READ,
            BT_GATT_PERM_READ,
            read_device_generation, NULL, device_generation),
BT_GATT_CHARACTERISTIC(BT_UUID_FIRMWARE,
            BT_GATT_CHRC_READ,
            BT_GATT_PERM_READ,
            read_firmware, NULL, firmware),
BT_GATT_CHARACTERISTIC(BT_UUID_HW_STATUS,
	    BT_GATT_CHRC_READ,
	    BT_GATT_PERM_READ,
	    read_hw_status, NULL, hw_status),
);