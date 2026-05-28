/*
 * Thermal IR sensor wrapper implementation.
 * See Thermal.h for the BLE wire format.
 */

#include "Thermal.h"
#include "SensorManager.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include <string.h>

LOG_MODULE_DECLARE(MLX90642);

Thermal Thermal::sensor;
MLX90642 Thermal::cam;

static struct sensor_msg msg_thermal;

/* Keep the 1.5 KB raw pixel buffer off the work-queue stack
 * (CONFIG_SENSOR_WORK_QUEUE_STACK_SIZE is only ~1 KB by default).
 * update_sensor() runs on a single work queue, so this is non-reentrant.
 */
static int16_t s_frame_pixels[MLX90642_NUM_PIXELS];

/* Refresh rate options. The reg_vals column holds MLX90642 refresh-rate codes
 * (see MLX90642_REFRESH_* in the driver). The other two columns describe the
 * achievable frame rate so SensorManager can plan the work-queue period.
 */
const SampleRateSetting<4> Thermal::sample_rates = {
	{ MLX90642_REFRESH_2HZ, MLX90642_REFRESH_4HZ,
	  MLX90642_REFRESH_8HZ, MLX90642_REFRESH_16HZ },
	{ 2.0f, 4.0f, 8.0f, 16.0f },
	{ 2.0f, 4.0f, 8.0f, 16.0f },
};

bool Thermal::init(struct k_msgq *queue)
{
	if (!_active) {
		/* Bring up both rails: IIC pull-ups live on 1.8V while the
		 * MLX90642 itself requires 3-3.6V (typ 3.3V, datasheet section 2.2).
		 * The 3V3 rail is driven by the BQ25120A LDO/LS chain
		 * (see Battery/BQ25120a setup). The PM runtime callbacks only
		 * wait the load-switch t_on (~300-600us); the MLX90642 POR /
		 * EEPROM-bootstrap is handled inside `MLX90642::begin()`.
		 */
		int r1 = pm_device_runtime_get(ls_1_8);
		int r2 = pm_device_runtime_get(ls_3_3);
		LOG_INF("Thermal rails: ls_1_8 get=%d, ls_3_3 get=%d", r1, r2);
		_active = true;
	}

	if (!cam.begin()) {
		LOG_ERR("MLX90642 not detected on IIC1 (i2c2, SCL=P1.00, SDA=P1.15). "
			"Re-check: (1) 3.3V on pin2, (2) GND on pin3, (3) SDA/SCL not "
			"swapped, (4) 4.7k pull-ups to 1V8 present, (5) DTS reg=0x66.");
		pm_device_runtime_put(ls_1_8);
		pm_device_runtime_put(ls_3_3);
		_active = false;
		return false;
	}

	sensor_queue = queue;

	k_work_init(&sensor.sensor_work, update_sensor);
	k_timer_init(&sensor.sensor_timer, sensor_timer_handler, NULL);

	return true;
}

void Thermal::update_sensor(struct k_work *work)
{
	ARG_UNUSED(work);

	if (!cam.isNewDataAvailable()) {
		return;
	}

	MLX90642::status ret = cam.readPixelsRaw(s_frame_pixels);
	if (ret != MLX90642::SENSOR_SUCCESS) {
		LOG_WRN("MLX90642 frame read failed: %d", (int)ret);
		return;
	}

	const uint64_t frame_time = micros();

	for (uint8_t chunk = 0; chunk < THERMAL_TOTAL_CHUNKS; chunk++) {
		const uint16_t offset = (uint16_t)chunk * THERMAL_PIXELS_PER_CHUNK;
		const uint8_t count = (uint8_t)MIN(
			(uint16_t)THERMAL_PIXELS_PER_CHUNK,
			(uint16_t)(MLX90642_NUM_PIXELS - offset));

		msg_thermal.sd = sensor._sd_logging;
		msg_thermal.stream = sensor._ble_stream;

		msg_thermal.data.id = ID_THERMAL;
		/* Payload = chunk header (2 bytes) + N int16 raw pixels. */
		msg_thermal.data.size = (uint8_t)(2U + (uint16_t)count * 2U);
		msg_thermal.data.time = frame_time;

		msg_thermal.data.data[0] = chunk;
		msg_thermal.data.data[1] = count;
		memcpy(&msg_thermal.data.data[2], &s_frame_pixels[offset],
		       (size_t)count * sizeof(int16_t));

		int put_ret = k_msgq_put(sensor_queue, &msg_thermal, K_NO_WAIT);
		if (put_ret) {
			/* Best-effort: drop the rest of the frame rather than
			 * block the work-queue. The web UI handles dropped chunks.
			 */
			LOG_WRN("Thermal queue full at chunk %u/%u (ret=%d)",
				(unsigned int)chunk,
				(unsigned int)THERMAL_TOTAL_CHUNKS, put_ret);
			break;
		}
	}
}

void Thermal::sensor_timer_handler(struct k_timer *dummy)
{
	ARG_UNUSED(dummy);
	k_work_submit_to_queue(&sensor_work_q, &sensor.sensor_work);
}

void Thermal::start(int sample_rate_idx)
{
	if (!_active) {
		return;
	}

	if (sample_rate_idx < 0 ||
	    sample_rate_idx >= (int)ARRAY_SIZE(sample_rates.reg_vals)) {
		LOG_WRN("Thermal start: invalid sample rate idx %d", sample_rate_idx);
		return;
	}

	const uint8_t rate_code = sample_rates.reg_vals[sample_rate_idx];
	(void)cam.setRefreshRate(rate_code);

	/* Poll twice as fast as the configured refresh rate so we can detect a
	 * fresh frame quickly. The probe inside `update_sensor()` skips when
	 * no new frame is available.
	 */
	const float rate_hz = sample_rates.true_sample_rates[sample_rate_idx];
	const float poll_hz = rate_hz * 2.0f;
	k_timeout_t t = K_USEC((uint32_t)(1e6f / poll_hz));

	k_timer_start(&sensor.sensor_timer, K_NO_WAIT, t);

	_running = true;
}

void Thermal::stop()
{
	if (!_active) {
		return;
	}
	_active = false;
	_running = false;

	k_timer_stop(&sensor.sensor_timer);

	pm_device_runtime_put(ls_1_8);
	pm_device_runtime_put(ls_3_3);
}
