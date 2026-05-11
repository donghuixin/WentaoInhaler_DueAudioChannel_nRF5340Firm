#ifndef AUDIO_WAVEFORM_SERVICE_H
#define AUDIO_WAVEFORM_SERVICE_H

#include <stddef.h>
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

#define AUDIO_WAVEFORM_SAMPLE_COUNT 96

struct audio_waveform_packet {
	uint32_t sequence;
	uint32_t sample_rate_hz;
	uint16_t frame_count;
	uint16_t peak_l;
	uint16_t peak_r;
	uint16_t mean_abs_l;
	uint16_t mean_abs_r;
	uint8_t sample_count;
	int8_t samples[AUDIO_WAVEFORM_SAMPLE_COUNT];
	int16_t min_mono;
	int16_t max_mono;
	uint16_t peak_to_peak_mono;
} __packed;

int audio_waveform_service_submit_i2s_block(const int16_t *samples, size_t frame_count);

#endif /* AUDIO_WAVEFORM_SERVICE_H */
