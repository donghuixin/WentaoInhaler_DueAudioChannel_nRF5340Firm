#ifndef AUDIO_WAVEFORM_SERVICE_H
#define AUDIO_WAVEFORM_SERVICE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/sys/util.h>

#define BT_UUID_AUDIO_WAVEFORM_SERVICE_VAL \
	BT_UUID_128_ENCODE(0x1410dfa0, 0x5f68, 0x4ebb, 0xa7c7, 0x5e0fb9ae7557)

#define BT_UUID_AUDIO_WAVEFORM_SERVICE \
	BT_UUID_DECLARE_128(BT_UUID_AUDIO_WAVEFORM_SERVICE_VAL)

#define BT_UUID_AUDIO_WAVEFORM_CONTROL_VAL \
	BT_UUID_128_ENCODE(0x1410dfa1, 0x5f68, 0x4ebb, 0xa7c7, 0x5e0fb9ae7557)

#define BT_UUID_AUDIO_WAVEFORM_CONTROL \
	BT_UUID_DECLARE_128(BT_UUID_AUDIO_WAVEFORM_CONTROL_VAL)

#define BT_UUID_AUDIO_WAVEFORM_DATA_VAL \
	BT_UUID_128_ENCODE(0x1410dfa2, 0x5f68, 0x4ebb, 0xa7c7, 0x5e0fb9ae7557)

#define BT_UUID_AUDIO_WAVEFORM_DATA \
	BT_UUID_DECLARE_128(BT_UUID_AUDIO_WAVEFORM_DATA_VAL)

#define AUDIO_WAVEFORM_PACKET_SIZE 251U
#define AUDIO_WAVEFORM_SAMPLE_RATE_HZ 16000U
#define AUDIO_WAVEFORM_PERIOD_MS 5U

#define AUDIO_WAVEFORM_MIC_PAYLOAD_SIZE 160U
#define AUDIO_WAVEFORM_MIC_SAMPLES_PER_PACKET 80U
#define AUDIO_WAVEFORM_THERMAL_PAYLOAD_SIZE 64U
#define AUDIO_WAVEFORM_THERMAL_PIXELS_PER_ROW 32U
#define AUDIO_WAVEFORM_IMU_PAYLOAD_SIZE 18U

struct audio_waveform_packet {
	uint8_t seq;
	uint8_t timestamp_be[4];
	uint8_t mic_valid_len;
	uint8_t mic_payload[AUDIO_WAVEFORM_MIC_PAYLOAD_SIZE];
	uint8_t thermal_valid_len;
	uint8_t thermal_row_index;
	uint8_t thermal_payload[AUDIO_WAVEFORM_THERMAL_PAYLOAD_SIZE];
	uint8_t imu_valid_len;
	uint8_t imu_payload[AUDIO_WAVEFORM_IMU_PAYLOAD_SIZE];
} __packed;

#ifdef __cplusplus
extern "C" {
#endif

int audio_waveform_service_submit_i2s_block(const int16_t *samples, size_t frame_count);
void audio_waveform_service_set_mic_enabled(bool enabled);
void audio_waveform_service_submit_imu_sample(const float accel_mps2[3],
					      const float gyro_dps[3],
					      const float mag_ut[3]);
void audio_waveform_service_submit_thermal_row(const int16_t *row_pixels,
					       size_t pixel_count,
					       uint8_t row_index);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_WAVEFORM_SERVICE_H */
