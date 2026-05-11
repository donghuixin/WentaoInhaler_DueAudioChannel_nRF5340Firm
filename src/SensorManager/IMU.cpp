#include "IMU.h"

#include "SensorManager.h"

#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#include <zephyr/logging/log.h>
#include <errno.h>
LOG_MODULE_DECLARE(sensor_manager);

static struct sensor_msg msg_imu;

#define BMI270_NODE DT_NODELABEL(bmi270)
static const struct device *const bmi270 = DEVICE_DT_GET(BMI270_NODE);

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

	if (sample_log_count < 5) {
		LOG_INF("BMI270 sample ax=%d.%06d ay=%d.%06d az=%d.%06d gx=%d.%06d gy=%d.%06d gz=%d.%06d",
			accel[0].val1, accel[0].val2, accel[1].val1, accel[1].val2,
			accel[2].val1, accel[2].val2, gyro[0].val1, gyro[0].val2,
			gyro[1].val1, gyro[1].val2, gyro[2].val1, gyro[2].val2);
		sample_log_count++;
	}

	size_t size = 3 * sizeof(float);
	
	msg_imu.sd = sensor._sd_logging;
	msg_imu.stream = sensor._ble_stream;

	msg_imu.data.id = ID_IMU;
	msg_imu.data.size = 3 * size;
	msg_imu.data.time = micros();

	memcpy(msg_imu.data.data, &accel_data,size);
	memcpy(msg_imu.data.data + size, &gyro_data, size);
	memcpy(msg_imu.data.data + 2 * size, &magno_data, size);

	ret = k_msgq_put(sensor_queue, &msg_imu, K_NO_WAIT);
	if (ret) {
		LOG_WRN("sensor msg queue full");
	}
}

/**
* @brief Submit a k_work on timer expiry.
*/
void IMU::sensor_timer_handler(struct k_timer *dummy)
{
	k_work_submit_to_queue(&sensor_work_q, &sensor.sensor_work);
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

	_running = true;

	k_timer_start(&sensor.sensor_timer, K_NO_WAIT, t);
}

void IMU::stop() {
    if (!_active) return;
    _active = false;

	_running = false;

	k_timer_stop(&sensor.sensor_timer);

	struct sensor_value sampling_freq;

	sampling_freq.val1 = 0;
	sampling_freq.val2 = 0;
	(void)sensor_attr_set(bmi270, SENSOR_CHAN_ACCEL_XYZ,
			      SENSOR_ATTR_SAMPLING_FREQUENCY, &sampling_freq);
	(void)sensor_attr_set(bmi270, SENSOR_CHAN_GYRO_XYZ,
			      SENSOR_ATTR_SAMPLING_FREQUENCY, &sampling_freq);

    pm_device_runtime_put(ls_1_8);
}
