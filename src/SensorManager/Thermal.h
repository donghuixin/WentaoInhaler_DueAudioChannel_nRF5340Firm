/*
 * Thermal IR sensor wrapper for the MLX90642 32x24 thermal array.
 *
 * Mirrors the structure of Temp / IMU / Baro so the SensorManager dispatcher
 * can drive it the same way. The wrapper polls a Zephyr timer; on each tick
 * it checks whether the MLX90642 has finished a new frame and, if so, reads
 * the full 768-pixel frame and pushes it onto `sensor_queue` as a sequence
 * of `sensor_msg` packets that the BLE / SD pipelines already understand.
 *
 * Each thermal BLE packet payload is:
 *   [chunk_idx : u8][count : u8][int16 raw_pixel x count]
 * All chunks belonging to the same frame share the same `sensor_data.time`
 * value (microseconds since boot at the moment the frame was captured).
 */

#ifndef _THERMAL_H
#define _THERMAL_H

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include "MLX90642/MLX90642.h"
#include "EdgeMLSensor.h"

#include "openearable_common.h"
#include "zbus_common.h"

/* Wire format for chunked thermal frames. 
 * 16 pixels (32 bytes) + 2 bytes header = 34 bytes payload.
 * This perfectly aligns with half-rows (width 32) so missing chunks
 * don't cause pixel stitching misalignments across row boundaries.
 * 768 pixels / 16 pixels per chunk = 48 chunks exactly.
 */
#define THERMAL_PIXELS_PER_CHUNK 16U
#define THERMAL_TOTAL_CHUNKS     48U

class Thermal : public EdgeMlSensor {
public:
	static Thermal sensor;

	bool init(struct k_msgq *queue) override;
	void start(int sample_rate_idx) override;
	void stop() override;

	/* Refresh-rate options exposed to ParseInfo / the phone:
	 *   index 0 -> 2 Hz, 1 -> 4 Hz, 2 -> 8 Hz, 3 -> 16 Hz
	 * The reg_vals here are MLX90642 refresh codes (2..5).
	 */
	const static SampleRateSetting<4> sample_rates;

private:
	static MLX90642 cam;

	static void sensor_timer_handler(struct k_timer *dummy);
	static void update_sensor(struct k_work *work);

	bool _active = false;
};

#endif /* _THERMAL_H */
