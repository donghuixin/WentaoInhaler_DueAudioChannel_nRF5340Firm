#include "IMU.h"

#include "SensorManager.h"
#include "../bluetooth/gatt_services/sensor_service.h"

#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
#include <errno.h>
LOG_MODULE_DECLARE(sensor_manager);

static struct sensor_msg msg_imu;

#define IMU_WORK_QUEUE_STACK_SIZE 2048U
#define IMU_WORK_QUEUE_PRIORITY 4

static struct k_work_q imu_work_q;
static K_THREAD_STACK_DEFINE(imu_work_q_stack, IMU_WORK_QUEUE_STACK_SIZE);
static bool imu_work_q_started;

#define BMI270_NODE DT_NODELABEL(bmi270)
static const struct device *const bmi270 = DEVICE_DT_GET(BMI270_NODE);
static const struct i2c_dt_spec bmi270_i2c = I2C_DT_SPEC_GET(BMI270_NODE);

#define BMI270_REG_AUX_X_LSB       0x04U
#define BMI270_REG_AUX_CONF        0x44U
#define BMI270_REG_AUX_DEV_ID      0x4BU
#define BMI270_REG_AUX_IF_CONF     0x4CU
#define BMI270_REG_AUX_RD_ADDR     0x4DU
#define BMI270_REG_AUX_WR_ADDR     0x4EU
#define BMI270_REG_AUX_WR_DATA     0x4FU
#define BMI270_REG_PWR_CTRL        0x7DU
#define BMI270_PWR_CTRL_AUX_EN     0x01U
#define BMI270_AUX_MANUAL_ENABLE   0x80U
#define BMI270_AUX_BURST_8_BYTES   0x03U

#define BMM150_AUX_I2C_ADDR        0x10U
#define BMM150_REG_X_L             0x42U
#define BMM150_REG_POWER           0x4BU
#define BMM150_REG_OPMODE_ODR      0x4CU
#define BMM150_REG_REP_XY          0x51U
#define BMM150_REG_REP_Z           0x52U
#define BMM150_POWER_ON            0x01U
#define BMM150_MODE_NORMAL         0x00U
#define BMM150_REP_XY_REGULAR      0x04U
#define BMM150_REP_Z_REGULAR       0x0EU
#define BMM150_RAW_TO_UT           0.3f

static bool bmm150_aux_ready;

static int bmi270_reg_update_byte(uint8_t reg, uint8_t mask, uint8_t value)
{
	uint8_t old_value;
	int ret = i2c_reg_read_byte_dt(&bmi270_i2c, reg, &old_value);

	if (ret) {
		return ret;
	}

	uint8_t new_value = (old_value & ~mask) | (value & mask);
	if (new_value == old_value) {
		return 0;
	}

	return i2c_reg_write_byte_dt(&bmi270_i2c, reg, new_value);
}

static int bmm150_aux_write(uint8_t reg, uint8_t value)
{
	int ret = i2c_reg_write_byte_dt(&bmi270_i2c, BMI270_REG_AUX_WR_DATA, value);

	if (ret) {
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&bmi270_i2c, BMI270_REG_AUX_WR_ADDR, reg);
	if (ret) {
		return ret;
	}

	k_msleep(2);
	return 0;
}

static uint8_t bmm150_odr_reg_for_rate(float rate_hz)
{
	if (rate_hz <= 2.0f) {
		return 0x01U;
	}
	if (rate_hz <= 6.0f) {
		return 0x02U;
	}
	if (rate_hz <= 8.0f) {
		return 0x03U;
	}
	if (rate_hz <= 10.0f) {
		return 0x00U;
	}
	if (rate_hz <= 15.0f) {
		return 0x04U;
	}
	if (rate_hz <= 20.0f) {
		return 0x05U;
	}
	if (rate_hz <= 25.0f) {
		return 0x06U;
	}

	return 0x07U;
}

static uint8_t bmi270_aux_odr_reg_for_rate(float rate_hz)
{
	if (rate_hz <= 25.0f) {
		return 0x06U;
	}
	if (rate_hz <= 50.0f) {
		return 0x07U;
	}
	if (rate_hz <= 100.0f) {
		return 0x08U;
	}
	if (rate_hz <= 200.0f) {
		return 0x09U;
	}
	if (rate_hz <= 400.0f) {
		return 0x0AU;
	}
	return 0x0BU;
}

static int bmm150_aux_configure(float imu_rate_hz, uint8_t bmi270_aux_odr_code)
{
	if (!i2c_is_ready_dt(&bmi270_i2c)) {
		return -ENODEV;
	}

	int ret = bmi270_reg_update_byte(BMI270_REG_PWR_CTRL,
					 BMI270_PWR_CTRL_AUX_EN,
					 BMI270_PWR_CTRL_AUX_EN);
	if (ret) {
		return ret;
	}
	k_msleep(2);

	ret = i2c_reg_write_byte_dt(&bmi270_i2c, BMI270_REG_AUX_DEV_ID,
				    BMM150_AUX_I2C_ADDR);
	if (ret) {
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&bmi270_i2c, BMI270_REG_AUX_IF_CONF,
				    BMI270_AUX_MANUAL_ENABLE);
	if (ret) {
		return ret;
	}
	k_msleep(2);

	ret = bmm150_aux_write(BMM150_REG_POWER, BMM150_POWER_ON);
	if (ret) {
		return ret;
	}
	k_msleep(3);

	ret = bmm150_aux_write(BMM150_REG_REP_XY, BMM150_REP_XY_REGULAR);
	if (ret) {
		return ret;
	}
	ret = bmm150_aux_write(BMM150_REG_REP_Z, BMM150_REP_Z_REGULAR);
	if (ret) {
		return ret;
	}

	const uint8_t bmm_odr = bmm150_odr_reg_for_rate(imu_rate_hz);
	const uint8_t bmm_opmode_odr = (uint8_t)((bmm_odr << 3) |
						 (BMM150_MODE_NORMAL << 1));
	ret = bmm150_aux_write(BMM150_REG_OPMODE_ODR, bmm_opmode_odr);
	if (ret) {
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&bmi270_i2c, BMI270_REG_AUX_RD_ADDR,
				    BMM150_REG_X_L);
	if (ret) {
		return ret;
	}
	ret = i2c_reg_write_byte_dt(&bmi270_i2c, BMI270_REG_AUX_CONF,
				    bmi270_aux_odr_code & 0x0fU);
	if (ret) {
		return ret;
	}
	ret = i2c_reg_write_byte_dt(&bmi270_i2c, BMI270_REG_AUX_IF_CONF,
				    BMI270_AUX_BURST_8_BYTES);
	if (ret) {
		return ret;
	}

	bmm150_aux_ready = true;
	LOG_INF("BMM150 auxiliary magnetometer configured via BMI270 ASD/ASC at %u Hz request",
		(unsigned int)imu_rate_hz);
	return 0;
}

static bool bmm150_aux_read(float mag_ut[3])
{
	if (!bmm150_aux_ready || (mag_ut == NULL)) {
		return false;
	}

	uint8_t data[8];
	int ret = i2c_burst_read_dt(&bmi270_i2c, BMI270_REG_AUX_X_LSB,
				    data, sizeof(data));
	if (ret) {
		return false;
	}

	const int16_t raw_x = (int16_t)sys_get_le16(&data[0]) >> 3;
	const int16_t raw_y = (int16_t)sys_get_le16(&data[2]) >> 3;
	const int16_t raw_z = (int16_t)sys_get_le16(&data[4]) >> 1;

	mag_ut[0] = raw_x * BMM150_RAW_TO_UT;
	mag_ut[1] = raw_y * BMM150_RAW_TO_UT;
	mag_ut[2] = raw_z * BMM150_RAW_TO_UT;
	return true;
}

IMU IMU::sensor;

const SampleRateSetting<6> IMU::sample_rates = {
	{ 25, 50, 100, 200, 255, 255 },

	{ 25, 50, 100, 200, 400, 800 },

	{ 25.0, 50.0, 100.0, 200.0, 400.0, 800.0 }
};

void IMU::update_sensor(struct k_work *work) {
	int ret;

	struct sensor_value accel[3];
	struct sensor_value gyro[3];
	float accel_data[3];
	float gyro_data[3];
	float magno_data[3] = { 0.0f, 0.0f, 0.0f };
	static uint8_t sample_log_count;

	ret = sensor_sample_fetch(bmi270);
	if (ret) {
		LOG_WRN("BMI270 sample fetch failed: %d", ret);
		return;
	}

	ret = sensor_channel_get(bmi270, SENSOR_CHAN_ACCEL_XYZ, accel);
	if (ret) {
		LOG_WRN("BMI270 accel read failed: %d", ret);
		return;
	}

	ret = sensor_channel_get(bmi270, SENSOR_CHAN_GYRO_XYZ, gyro);
	if (ret) {
		LOG_WRN("BMI270 gyro read failed: %d", ret);
		return;
	}

	for (int i = 0; i < 3; i++) {
		accel_data[i] = sensor_value_to_float(&accel[i]);
		gyro_data[i] = sensor_value_to_float(&gyro[i]) * 57.2957795f;
	}

	(void)bmm150_aux_read(magno_data);

	if (sample_log_count < 5) {
		LOG_INF("BMI270 sample ax=%d.%06d ay=%d.%06d az=%d.%06d gx=%d.%06d gy=%d.%06d gz=%d.%06d",
			accel[0].val1, accel[0].val2, accel[1].val1, accel[1].val2,
			accel[2].val1, accel[2].val2, gyro[0].val1, gyro[0].val2,
			gyro[1].val1, gyro[1].val2, gyro[2].val1, gyro[2].val2);
		sample_log_count++;
	}

	size_t size = 3 * sizeof(float);
	
	msg_imu.sd = sensor._sd_logging;
	msg_imu.stream = false;

	msg_imu.data.id = ID_IMU;
	msg_imu.data.size = 3 * size;
	msg_imu.data.time = micros();

	memcpy(msg_imu.data.data, &accel_data,size);
	memcpy(msg_imu.data.data + size, &gyro_data, size);
	memcpy(msg_imu.data.data + 2 * size, &magno_data, size);

	if (sensor._ble_stream) {
		ret = sensor_service_submit_imu_sample(&msg_imu.data);
		if ((ret != 0) && (ret != -EACCES)) {
			static uint32_t ble_drop_count;
			ble_drop_count++;
			if ((ble_drop_count == 1U) || ((ble_drop_count % 100U) == 0U)) {
				LOG_WRN("IMU BLE batch enqueue failed: %d drops=%u",
					ret, ble_drop_count);
			}
		}
	}

	if (sensor._sd_logging) {
		ret = k_msgq_put(sensor_queue, &msg_imu, K_NO_WAIT);
		if (ret) {
			LOG_WRN("sensor msg queue full");
		}
	}
}

/**
* @brief Submit a k_work on timer expiry.
*/
void IMU::sensor_timer_handler(struct k_timer *dummy)
{
	ARG_UNUSED(dummy);
	k_work_submit_to_queue(&imu_work_q, &sensor.sensor_work);
};

bool IMU::init(struct k_msgq * queue) {
	int ret;

	if (!_active) {
		pm_device_runtime_get(ls_1_8);
    	_active = true;
	}

	if (!device_is_ready(bmi270)) {
		LOG_INF("BMI270 deferred init after V_LS power-on");
		ret = device_init(bmi270);
		if (ret && ret != -EALREADY) {
			LOG_ERR("BMI270 device init failed: %d", ret);
			pm_device_runtime_put(ls_1_8);
			_active = false;
			return false;
		}
	}

	if (!device_is_ready(bmi270)) {
		LOG_ERR("BMI270 device is not ready after init");
		pm_device_runtime_put(ls_1_8);
		_active = false;
		return false;
	}

	struct sensor_value full_scale;
	struct sensor_value oversampling;

	full_scale.val1 = 2;
	full_scale.val2 = 0;
	oversampling.val1 = 1;
	oversampling.val2 = 0;

	ret = sensor_attr_set(bmi270, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_FULL_SCALE,
			      &full_scale);
	if (ret) {
		LOG_ERR("BMI270 accel full scale config failed: %d", ret);
		pm_device_runtime_put(ls_1_8);
		_active = false;
		return false;
	}

	ret = sensor_attr_set(bmi270, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_OVERSAMPLING,
			      &oversampling);
	if (ret) {
		LOG_ERR("BMI270 accel oversampling config failed: %d", ret);
		pm_device_runtime_put(ls_1_8);
		_active = false;
		return false;
	}

	full_scale.val1 = 500;
	full_scale.val2 = 0;

	ret = sensor_attr_set(bmi270, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_FULL_SCALE,
			      &full_scale);
	if (ret) {
		LOG_ERR("BMI270 gyro full scale config failed: %d", ret);
		pm_device_runtime_put(ls_1_8);
		_active = false;
		return false;
	}

	ret = sensor_attr_set(bmi270, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_OVERSAMPLING,
			      &oversampling);
	if (ret) {
		LOG_ERR("BMI270 gyro oversampling config failed: %d", ret);
		pm_device_runtime_put(ls_1_8);
		_active = false;
		return false;
	}

	sensor_queue = queue;

	if (!imu_work_q_started) {
		k_work_queue_init(&imu_work_q);
		k_work_queue_start(&imu_work_q, imu_work_q_stack,
				   K_THREAD_STACK_SIZEOF(imu_work_q_stack),
				   K_PRIO_PREEMPT(IMU_WORK_QUEUE_PRIORITY), NULL);
		(void)k_thread_name_set(&imu_work_q.thread, "IMU_SAMPLE");
		imu_work_q_started = true;
	}
	
	k_work_init(&sensor.sensor_work, update_sensor);
	k_timer_init(&sensor.sensor_timer, sensor_timer_handler, NULL);

	return true;
}

void IMU::start(int sample_rate_idx) {
	if (!_active) return;

    k_timeout_t t = K_USEC(1e6 / sample_rates.true_sample_rates[sample_rate_idx]);
	struct sensor_value sampling_freq;

	sampling_freq.val1 = (int)sample_rates.sample_rates[sample_rate_idx];
	sampling_freq.val2 = 0;

	int ret = sensor_attr_set(bmi270, SENSOR_CHAN_ACCEL_XYZ,
				  SENSOR_ATTR_SAMPLING_FREQUENCY, &sampling_freq);
	if (ret) {
		LOG_ERR("BMI270 accel sampling frequency config failed: %d", ret);
		return;
	}

	ret = sensor_attr_set(bmi270, SENSOR_CHAN_GYRO_XYZ,
			      SENSOR_ATTR_SAMPLING_FREQUENCY, &sampling_freq);
	if (ret) {
		LOG_ERR("BMI270 gyro sampling frequency config failed: %d", ret);
		return;
	}

	ret = bmm150_aux_configure(sample_rates.true_sample_rates[sample_rate_idx],
				   bmi270_aux_odr_reg_for_rate(
					   sample_rates.true_sample_rates[sample_rate_idx]));
	if (ret) {
		bmm150_aux_ready = false;
		LOG_WRN("BMM150 auxiliary magnetometer config failed: %d", ret);
	}

	_running = true;

	k_timer_start(&sensor.sensor_timer, K_NO_WAIT, t);
}

void IMU::stop() {
    if (!_active) return;

	k_timer_stop(&sensor.sensor_timer);
	struct k_work_sync sync;
	(void)k_work_cancel_sync(&sensor.sensor_work, &sync);

	if (_ble_stream) {
		(void)sensor_service_flush_imu_batch();
	}

	_active = false;
	_running = false;

	struct sensor_value sampling_freq;

	sampling_freq.val1 = 0;
	sampling_freq.val2 = 0;
	(void)sensor_attr_set(bmi270, SENSOR_CHAN_ACCEL_XYZ,
			      SENSOR_ATTR_SAMPLING_FREQUENCY, &sampling_freq);
	(void)sensor_attr_set(bmi270, SENSOR_CHAN_GYRO_XYZ,
			      SENSOR_ATTR_SAMPLING_FREQUENCY, &sampling_freq);
	(void)bmi270_reg_update_byte(BMI270_REG_PWR_CTRL, BMI270_PWR_CTRL_AUX_EN, 0U);
	bmm150_aux_ready = false;

    pm_device_runtime_put(ls_1_8);
}
