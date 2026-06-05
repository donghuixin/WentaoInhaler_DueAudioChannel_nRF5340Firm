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

#define AUDIO_WAVEFORM_PACKET_SIZE 244U
#define AUDIO_WAVEFORM_SAMPLE_RATE_HZ 16000U
#define AUDIO_WAVEFORM_PERIOD_US 7500U

#define AUDIO_WAVEFORM_MIC_PAYLOAD_SIZE 240U
#define AUDIO_WAVEFORM_MIC_SAMPLES_PER_PACKET 120U

struct audio_waveform_packet {
	uint8_t first_sample_timestamp_us_le[4];
	uint8_t mic_payload[AUDIO_WAVEFORM_MIC_PAYLOAD_SIZE];
} __packed;

#ifdef __cplusplus
extern "C" {
#endif

int audio_waveform_service_submit_i2s_block(const int16_t *samples, size_t frame_count,
					    uint32_t first_sample_timestamp_us);
void audio_waveform_service_set_mic_enabled(bool enabled);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_WAVEFORM_SERVICE_H */
