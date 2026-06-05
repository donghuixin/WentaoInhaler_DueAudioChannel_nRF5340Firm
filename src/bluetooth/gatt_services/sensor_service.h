//#pragma once

#ifndef SENSOR_SERVICE_H
#define SENSOR_SERVICE_H

#include <zephyr/bluetooth/gatt.h>
#include "openearable_common.h"
#include "zbus_common.h"

#define BT_UUID_SENSOR_VAL \
	BT_UUID_128_ENCODE(0x34c2e3bb, 0x34aa, 0x11eb, 0xadc1, 0x0242ac120002)

/** @brief Sensor Characteristic UUID. */
#define BT_UUID_SENSOR_CONFIG_VAL \
    BT_UUID_128_ENCODE(0x34c2e3be, 0x34aa, 0x11eb, 0xadc1, 0x0242ac120002)
#define BT_UUID_SENSOR_CONFIG_STATUS_VAL \
    BT_UUID_128_ENCODE(0x34c2e3bf, 0x34aa, 0x11eb, 0xadc1, 0x0242ac120002)
#define BT_UUID_SENSOR_RECORDING_NAME_VAL \
    BT_UUID_128_ENCODE(0x34c2e3c0, 0x34aa, 0x11eb, 0xadc1, 0x0242ac120002)
#define BT_UUID_SENSOR_THERMAL_DATA_VAL \
    BT_UUID_128_ENCODE(0x34c2e3c1, 0x34aa, 0x11eb, 0xadc1, 0x0242ac120002)

#define BT_UUID_SENSOR_DATA_VAL \
    BT_UUID_128_ENCODE(0x34c2e3bc, 0x34aa, 0x11eb, 0xadc1, 0x0242ac120002)

#define BT_UUID_SENSOR             BT_UUID_DECLARE_128(BT_UUID_SENSOR_VAL)
#define BT_UUID_SENSOR_CONFIG      BT_UUID_DECLARE_128(BT_UUID_SENSOR_CONFIG_VAL)
#define BT_UUID_SENSOR_CONFIG_STATUS BT_UUID_DECLARE_128(BT_UUID_SENSOR_CONFIG_STATUS_VAL)
#define BT_UUID_SENSOR_RECORDING_NAME BT_UUID_DECLARE_128(BT_UUID_SENSOR_RECORDING_NAME_VAL)
#define BT_UUID_SENSOR_DATA        BT_UUID_DECLARE_128(BT_UUID_SENSOR_DATA_VAL)
#define BT_UUID_SENSOR_THERMAL_DATA BT_UUID_DECLARE_128(BT_UUID_SENSOR_THERMAL_DATA_VAL)

/*
 * Shared high-rate Sensor Stream packet. IR and batched IMU data use the same
 * characteristic and scheduler so IR can be prioritized without starving IMU.
 */
#define SENSOR_STREAM_PACKET_SIZE_MAX 244U
#define SENSOR_STREAM_HEADER_SIZE 16U
#define SENSOR_STREAM_TYPE_THERMAL 1U
#define SENSOR_STREAM_TYPE_IMU_BATCH 2U

#define SENSOR_THERMAL_BLE_PIXELS_PER_PACKET 114U
#define SENSOR_THERMAL_BLE_PIXELS_PER_FRAME 768U
#define SENSOR_THERMAL_BLE_PACKETS_PER_FRAME 7U
#define SENSOR_IMU_PAYLOAD_SIZE 36U
#define SENSOR_IMU_BATCH_SAMPLES 5U
#define SENSOR_IMU_BATCH_SAMPLE_SIZE (sizeof(uint32_t) + SENSOR_IMU_PAYLOAD_SIZE)

struct sensor_stream_packet {
	uint8_t timestamp_us_le[8];
	uint8_t sequence_le[2];
	uint8_t packet_type;
	uint8_t item_count;
	uint8_t item_offset_le[2];
	uint8_t packet_index;
	uint8_t packet_count;
	uint8_t payload[SENSOR_STREAM_PACKET_SIZE_MAX - SENSOR_STREAM_HEADER_SIZE];
} __attribute__((packed));

#ifdef __cplusplus
extern "C" {
#endif

int init_sensor_service();
const char *get_sensor_recording_name();
//int send_sensor_data(); //struct sensor_data * data);

int set_sensor_config_status(struct sensor_config config);

void temp_disable_notifies(bool disable);
int sensor_service_submit_thermal_frame(const int16_t *pixels,
					size_t pixel_count,
					uint64_t timestamp_us);
int sensor_service_submit_imu_sample(const struct sensor_data *sample);
int sensor_service_flush_imu_batch(void);

#ifdef __cplusplus
}
#endif

#endif
