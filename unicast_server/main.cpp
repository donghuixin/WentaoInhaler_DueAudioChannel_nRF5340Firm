/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_uart.h>

//#include "../src/modules/sd_card.h"

#include <zephyr/settings/settings.h>

#include "macros_common.h"
#include "openearable_common.h"
#include "streamctrl.h"

#include "../src/Battery/PowerManager.h"
#include "../src/SensorManager/SensorManager.h"
#include "../src/utils/StateIndicator.h"

#include "device_info.h"
#include "battery_service.h"
#include "button_service.h"
#include "sensor_service.h"
#include "led_service.h"

#include "SensorScheme.h"
#include "DefaultSensors.h"

#include "time_sync.h"

#include "../src/SD_Card/SDLogger/SDLogger.h"

#include "uicr.h"

#include "streamctrl.h"

#include "bt_mgmt.h"

#include "bt_mgmt_conn_interval.h"
#include "conn_interval/conn_intvl_linear.h"

//#include "sd_card.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, CONFIG_MAIN_LOG_LEVEL);
//BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_console), zephyr_cdc_acm_uart),
//	     "Console device is not ACM CDC UART device");

/* STEP 5.4 - Include header for USB */
#include <zephyr/usb/usb_device.h>

#if IS_ENABLED(CONFIG_OPENEARABLE_I2C3_SCAN_TEST)
static int i2c_ping_addr(const struct device *bus, uint8_t addr)
{
	uint8_t dummy = 0;

	return i2c_write(bus, &dummy, 0, addr);
}

static void i2c_log_probe(const char *name, const struct device *bus, uint8_t addr)
{
	uint8_t reg0 = 0xff;
	int ping_ret = i2c_ping_addr(bus, addr);
	int read_ret = i2c_burst_read(bus, addr, 0x00, &reg0, sizeof(reg0));

	LOG_INF("%s probe addr=0x%02x ping=%d reg0_read=%d reg0=0x%02x",
		name, addr, ping_ret, read_ret, reg0);
}

static void i2c_scan_bus(const char *name, const struct device *bus,
			 const char *speed_name, uint32_t speed)
{
	int ret = i2c_configure(bus, I2C_SPEED_SET(speed));

	LOG_INF("%s scan speed=%s configure=%d", name, speed_name, ret);
	k_msleep(10);

	i2c_log_probe(name, bus, 0x12); /* BMM150 direct I2C address, if routed */
	i2c_log_probe(name, bus, 0x18); /* BMA580 on original OpenEarable */
	i2c_log_probe(name, bus, 0x55); /* BQ27220 */
	i2c_log_probe(name, bus, 0x62); /* MAXM86161 */
	i2c_log_probe(name, bus, 0x64); /* ADAU1860 */
	i2c_log_probe(name, bus, 0x68); /* BMI270/BMX160 default */
	i2c_log_probe(name, bus, 0x69); /* BMI270/BMX160 alternate */
	i2c_log_probe(name, bus, 0x6a); /* BQ25120A */
	i2c_log_probe(name, bus, 0x76); /* BMP388 */

	for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
		ret = i2c_ping_addr(bus, addr);
		if (ret == 0) {
			uint8_t reg0 = 0xff;
			int read_ret = i2c_burst_read(bus, addr, 0x00, &reg0, sizeof(reg0));

			LOG_INF("%s ACK addr=0x%02x reg0_read=%d reg0=0x%02x",
				name, addr, read_ret, reg0);
		}
	}
}

static void run_i2c3_scan_test(void)
{
	const struct device *i2c1 = DEVICE_DT_GET(DT_NODELABEL(i2c1));
	const struct device *i2c2 = DEVICE_DT_GET(DT_NODELABEL(i2c2));
	const struct device *i2c3 = DEVICE_DT_GET(DT_NODELABEL(i2c3));
	int ret;

	LOG_INF("I2C bus scan diagnostic start");
	LOG_INF("I2C1 device ready=%d", device_is_ready(i2c1));
	LOG_INF("I2C2 device ready=%d", device_is_ready(i2c2));
	LOG_INF("I2C3 device ready=%d", device_is_ready(i2c3));

	ret = pm_device_runtime_get(ls_1_8);
	LOG_INF("I2C bus scan ls_1_8 get ret=%d", ret);
	k_msleep(100);

	i2c_scan_bus("I2C1", i2c1, "100k", I2C_SPEED_STANDARD);
	i2c_scan_bus("I2C1", i2c1, "400k", I2C_SPEED_FAST);
	i2c_scan_bus("I2C2", i2c2, "100k", I2C_SPEED_STANDARD);
	i2c_scan_bus("I2C2", i2c2, "400k", I2C_SPEED_FAST);
	i2c_scan_bus("I2C3", i2c3, "100k", I2C_SPEED_STANDARD);
	i2c_scan_bus("I2C3", i2c3, "400k", I2C_SPEED_FAST);
	i2c_scan_bus("I2C3", i2c3, "1M", I2C_SPEED_FAST_PLUS);

	LOG_INF("I2C bus scan diagnostic done");
}
#endif


int main(void) {
	int ret;

	LOG_DBG("nRF5340 APP core started");

	ret = power_manager.begin();
	ERR_CHK(ret);

	uint8_t standalone = uicr_standalone_get();

	LOG_INF("Standalone mode: %i", standalone);

#if IS_ENABLED(CONFIG_OPENEARABLE_I2C3_SCAN_TEST)
	run_i2c3_scan_test();
#endif

	/*sdcard_manager.init();

	sdcard_manager.mount();*/

	/* STEP 5.5 - Enable USB */
	if (IS_ENABLED(CONFIG_USB_DEVICE_STACK)) {
		ret = usb_enable(NULL);
		if (ret) {
			LOG_ERR("Failed to enable USB");
			return 0;
		}
	}

	streamctrl_start();

	uint32_t sirk = uicr_sirk_get();

	if (sirk == 0xFFFFFFFFU) {
		state_indicator.set_pairing_state(SET_PAIRING);
	} else if (bonded_device_count > 0 && !oe_boot_state.timer_reset) {
		state_indicator.set_pairing_state(PAIRED);
	} else {
		state_indicator.set_pairing_state(BONDING);
	}

	init_sensor_manager();

	//sensor_config imu = {ID_IMU, 80, 0};
	//sensor_config imu = {ID_PPG, 400, 0};
	//sensor_config temp = {ID_OPTTEMP, 10, 0};
	// sensor_config temp = {ID_BONE_CONDUCTION, 100, 0};

	//config_sensor(&temp);

	//sensor_config ppg = {ID_PPG, 400, 0};
	//config_sensor(&ppg);

    ret = init_led_service();
	ERR_CHK(ret);

	ret = init_battery_service();
	ERR_CHK(ret);

	ret = init_button_service();
	ERR_CHK(ret);

	ret = initParseInfoService(&defaultSensorIds, defaultSensors);
	ERR_CHK(ret);

	ret = init_sensor_service();
	ERR_CHK(ret);

	bt_mgmt_conn_interval_init(new ConnIntvlLinear(
	    1,
	    CONFIG_BLE_ACL_CONN_INTERVAL,
	    CONFIG_BLE_ACL_CONN_INTERVAL
	));

	ret = init_time_sync();
	ERR_CHK(ret);

	// error test
	//long *a = nullptr;
	//*a = 10;

	return 0;
}
