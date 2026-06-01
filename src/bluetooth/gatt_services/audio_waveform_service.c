#include "audio_waveform_service.h"

#include <errno.h>
#include <string.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(audio_waveform_service, CONFIG_BLE_LOG_LEVEL);

#define AUDIO_WAVEFORM_CONTROL_ENABLE BIT(0)
#define AUDIO_WAVEFORM_CONTROL_RESET BIT(1)

#define AUDIO_TDM_MIC_RING_PACKETS 16U
#define AUDIO_TDM_MIC_RING_SAMPLES \
	(AUDIO_WAVEFORM_MIC_SAMPLES_PER_PACKET * AUDIO_TDM_MIC_RING_PACKETS)
#define AUDIO_TDM_THERMAL_QUEUE_DEPTH 24U
#define AUDIO_TDM_THREAD_STACK_SIZE 1536
#define AUDIO_TDM_THREAD_PRIORITY 7

BUILD_ASSERT(sizeof(struct audio_waveform_packet) == AUDIO_WAVEFORM_PACKET_SIZE);
BUILD_ASSERT(offsetof(struct audio_waveform_packet, seq) == 0U);
BUILD_ASSERT(offsetof(struct audio_waveform_packet, timestamp_be) == 1U);
BUILD_ASSERT(offsetof(struct audio_waveform_packet, mic_valid_len) == 5U);
BUILD_ASSERT(offsetof(struct audio_waveform_packet, mic_payload) == 6U);
BUILD_ASSERT(offsetof(struct audio_waveform_packet, thermal_valid_len) == 166U);
BUILD_ASSERT(offsetof(struct audio_waveform_packet, thermal_row_index) == 167U);
BUILD_ASSERT(offsetof(struct audio_waveform_packet, thermal_payload) == 168U);
BUILD_ASSERT(offsetof(struct audio_waveform_packet, imu_valid_len) == 232U);
BUILD_ASSERT(offsetof(struct audio_waveform_packet, imu_payload) == 233U);
BUILD_ASSERT(AUDIO_WAVEFORM_MIC_PAYLOAD_SIZE ==
	     AUDIO_WAVEFORM_MIC_SAMPLES_PER_PACKET * sizeof(int16_t));
BUILD_ASSERT(AUDIO_WAVEFORM_THERMAL_PAYLOAD_SIZE ==
	     AUDIO_WAVEFORM_THERMAL_PIXELS_PER_ROW * sizeof(int16_t));
BUILD_ASSERT(AUDIO_WAVEFORM_IMU_PAYLOAD_SIZE == 9U * sizeof(int16_t));

static uint8_t control_value;
static bool capture_enabled;
static bool notify_enabled;
static bool mic_stream_enabled;
static uint8_t sequence;
static struct audio_waveform_packet latest_packet;

static struct k_spinlock stream_lock;

static int16_t mic_ring[AUDIO_TDM_MIC_RING_SAMPLES];
static uint16_t mic_read_idx;
static uint16_t mic_write_idx;
static uint16_t mic_count;

static uint8_t thermal_rows[AUDIO_TDM_THERMAL_QUEUE_DEPTH]
			   [AUDIO_WAVEFORM_THERMAL_PAYLOAD_SIZE];
static uint8_t thermal_row_indices[AUDIO_TDM_THERMAL_QUEUE_DEPTH];
static uint8_t thermal_read_idx;
static uint8_t thermal_write_idx;
static uint8_t thermal_count;

static uint8_t imu_payload[AUDIO_WAVEFORM_IMU_PAYLOAD_SIZE];
static bool imu_pending;

static uint16_t dropped_mic_samples = 0;

static struct k_thread audio_tdm_thread_data;
static K_THREAD_STACK_DEFINE(audio_tdm_thread_stack, AUDIO_TDM_THREAD_STACK_SIZE);

static struct k_sem tdm_tick_sem;
static void tdm_tick_handler(struct k_timer *timer);
K_TIMER_DEFINE(tdm_tick_timer, tdm_tick_handler, NULL);

static void tdm_tick_handler(struct k_timer *timer)
{
	k_sem_give(&tdm_tick_sem);
}

static int16_t clamp_i16_from_float(float value)
{
	if (value > 32767.0f) {
		return INT16_MAX;
	}

	if (value < -32768.0f) {
		return INT16_MIN;
	}

	return (int16_t)(value >= 0.0f ? value + 0.5f : value - 0.5f);
}

static void stream_cache_reset(bool reset_sequence)
{
	k_spinlock_key_t key = k_spin_lock(&stream_lock);

	mic_read_idx = 0;
	mic_write_idx = 0;
	mic_count = 0;
	thermal_read_idx = 0;
	thermal_write_idx = 0;
	thermal_count = 0;
	imu_pending = false;

	memset(&latest_packet, 0, sizeof(latest_packet));

	if (reset_sequence) {
		sequence = 0;
	}

	k_spin_unlock(&stream_lock, key);
}

static void mic_ring_push(int16_t sample)
{
	if (mic_count == AUDIO_TDM_MIC_RING_SAMPLES) {
		mic_read_idx = (mic_read_idx + 1U) % AUDIO_TDM_MIC_RING_SAMPLES;
		mic_count--;
		dropped_mic_samples++;
		if (dropped_mic_samples >= AUDIO_WAVEFORM_MIC_SAMPLES_PER_PACKET) {
			sequence++;
			dropped_mic_samples -= AUDIO_WAVEFORM_MIC_SAMPLES_PER_PACKET;
		}
	}

	mic_ring[mic_write_idx] = sample;
	mic_write_idx = (mic_write_idx + 1U) % AUDIO_TDM_MIC_RING_SAMPLES;
	mic_count++;
}

static uint8_t mic_ring_copy_payload(uint8_t *payload)
{
	const uint16_t samples_to_copy =
		MIN((uint16_t)AUDIO_WAVEFORM_MIC_SAMPLES_PER_PACKET, mic_count);

	for (uint16_t i = 0; i < samples_to_copy; i++) {
		const uint16_t idx =
			(mic_read_idx + i) % AUDIO_TDM_MIC_RING_SAMPLES;
		const int16_t sample = mic_ring[idx];

		sys_put_le16((uint16_t)sample, &payload[i * sizeof(int16_t)]);
	}

	return (uint8_t)(samples_to_copy * sizeof(int16_t));
}

static void mic_ring_advance_payload(uint8_t valid_len)
{
	const uint16_t samples_consumed = valid_len / sizeof(int16_t);

	for (uint16_t i = 0; i < samples_consumed; i++) {
		mic_read_idx = (mic_read_idx + 1U) % AUDIO_TDM_MIC_RING_SAMPLES;
		mic_count--;
	}
}

static bool thermal_row_pop(uint8_t *payload, uint8_t *row_index)
{
	if (thermal_count == 0U) {
		return false;
	}

	memcpy(payload, thermal_rows[thermal_read_idx], AUDIO_WAVEFORM_THERMAL_PAYLOAD_SIZE);
	if (row_index != NULL) {
		*row_index = thermal_row_indices[thermal_read_idx];
	}
	thermal_read_idx = (thermal_read_idx + 1U) % AUDIO_TDM_THERMAL_QUEUE_DEPTH;
	thermal_count--;

	return true;
}

static void packet_build(struct audio_waveform_packet *packet)
{
	k_spinlock_key_t key;

	memset(packet, 0, sizeof(*packet));

	key = k_spin_lock(&stream_lock);

	packet->seq = sequence++;
	sys_put_be32(k_uptime_get_32(), packet->timestamp_be);

	packet->mic_valid_len = mic_ring_copy_payload(packet->mic_payload);

	packet->thermal_row_index = 0xffU;
	if (thermal_row_pop(packet->thermal_payload, &packet->thermal_row_index)) {
		packet->thermal_valid_len = AUDIO_WAVEFORM_THERMAL_PAYLOAD_SIZE;
	}

	if (imu_pending) {
		memcpy(packet->imu_payload, imu_payload, sizeof(packet->imu_payload));
		packet->imu_valid_len = AUDIO_WAVEFORM_IMU_PAYLOAD_SIZE;
		imu_pending = false;
	}

	memcpy(&latest_packet, packet, sizeof(latest_packet));

	k_spin_unlock(&stream_lock, key);
}

static ssize_t read_control(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr,
			    void *buf,
			    uint16_t len,
			    uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &control_value,
				 sizeof(control_value));
}

static ssize_t write_control(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     const void *buf,
			     uint16_t len,
			     uint16_t offset,
			     uint8_t flags)
{
	if (len != sizeof(uint8_t)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	control_value = *(const uint8_t *)buf;
	capture_enabled = (control_value & AUDIO_WAVEFORM_CONTROL_ENABLE) != 0;

	if ((control_value & AUDIO_WAVEFORM_CONTROL_RESET) != 0) {
		stream_cache_reset(true);
	}

	if (capture_enabled && notify_enabled) {
		k_timer_start(&tdm_tick_timer, K_MSEC(AUDIO_WAVEFORM_PERIOD_MS),
			      K_MSEC(AUDIO_WAVEFORM_PERIOD_MS));
	} else {
		k_timer_stop(&tdm_tick_timer);
	}

	LOG_INF("Audio TDM stream %s period=%ums packet=%uB mic=%uB thermal=%uB imu=%uB",
		capture_enabled ? "enabled" : "disabled",
		AUDIO_WAVEFORM_PERIOD_MS, AUDIO_WAVEFORM_PACKET_SIZE,
		AUDIO_WAVEFORM_MIC_PAYLOAD_SIZE,
		AUDIO_WAVEFORM_THERMAL_PAYLOAD_SIZE,
		AUDIO_WAVEFORM_IMU_PAYLOAD_SIZE);

	return len;
}

static ssize_t read_waveform(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr,
			     void *buf,
			     uint16_t len,
			     uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &latest_packet,
				 sizeof(latest_packet));
}

static void waveform_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	notify_enabled = (value == BT_GATT_CCC_NOTIFY);
	LOG_INF("Audio TDM notifications %s", notify_enabled ? "enabled" : "disabled");

	if (notify_enabled && capture_enabled) {
		k_timer_start(&tdm_tick_timer, K_MSEC(AUDIO_WAVEFORM_PERIOD_MS),
			      K_MSEC(AUDIO_WAVEFORM_PERIOD_MS));
	} else {
		k_timer_stop(&tdm_tick_timer);
	}

	if (!notify_enabled) {
		stream_cache_reset(false);
	}
}

BT_GATT_SERVICE_DEFINE(audio_waveform_service,
BT_GATT_PRIMARY_SERVICE(BT_UUID_AUDIO_WAVEFORM_SERVICE),
BT_GATT_CHARACTERISTIC(BT_UUID_AUDIO_WAVEFORM_CONTROL,
		       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
		       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		       read_control, write_control, &control_value),
BT_GATT_CHARACTERISTIC(BT_UUID_AUDIO_WAVEFORM_DATA,
		       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
		       BT_GATT_PERM_READ,
		       read_waveform, NULL, &latest_packet),
BT_GATT_CCC(waveform_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

int audio_waveform_service_submit_i2s_block(const int16_t *samples, size_t frame_count)
{
	if (!capture_enabled || !notify_enabled || !mic_stream_enabled) {
		return -EACCES;
	}

	if ((samples == NULL) || (frame_count == 0U)) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&stream_lock);

	for (size_t i = 0; i < frame_count; i++) {
		/* The fixed BLE TDM packet has one mono PCM slot. Use the left
		 * I2S channel here so the 16 kHz payload remains continuous.
		 */
		mic_ring_push(samples[i * 2U]);
	}

	k_spin_unlock(&stream_lock, key);

	return 0;
}

void audio_waveform_service_set_mic_enabled(bool enabled)
{
	k_spinlock_key_t key = k_spin_lock(&stream_lock);

	mic_stream_enabled = enabled;
	if (!enabled) {
		mic_read_idx = 0;
		mic_write_idx = 0;
		mic_count = 0;
	}

	k_spin_unlock(&stream_lock, key);
}

void audio_waveform_service_submit_imu_sample(const float accel_mps2[3],
					      const float gyro_dps[3],
					      const float mag_ut[3])
{
	if ((accel_mps2 == NULL) || (gyro_dps == NULL) || (mag_ut == NULL)) {
		return;
	}

	int16_t scaled[9];

	for (size_t i = 0; i < 3U; i++) {
		scaled[i] = clamp_i16_from_float(accel_mps2[i] * (1000.0f / 9.80665f));
		scaled[i + 3U] = clamp_i16_from_float(gyro_dps[i] * 10.0f);
		scaled[i + 6U] = clamp_i16_from_float(mag_ut[i] * 10.0f);
	}

	k_spinlock_key_t key = k_spin_lock(&stream_lock);

	for (size_t i = 0; i < ARRAY_SIZE(scaled); i++) {
		sys_put_le16((uint16_t)scaled[i], &imu_payload[i * sizeof(int16_t)]);
	}

	imu_pending = true;

	k_spin_unlock(&stream_lock, key);
}

void audio_waveform_service_submit_thermal_row(const int16_t *row_pixels,
					       size_t pixel_count,
					       uint8_t row_index)
{
	if ((row_pixels == NULL) ||
	    (pixel_count < AUDIO_WAVEFORM_THERMAL_PIXELS_PER_ROW)) {
		return;
	}

	k_spinlock_key_t key = k_spin_lock(&stream_lock);

	if (row_index == 0U) {
		thermal_read_idx = 0;
		thermal_write_idx = 0;
		thermal_count = 0;
	}

	if (thermal_count == AUDIO_TDM_THERMAL_QUEUE_DEPTH) {
		thermal_read_idx = (thermal_read_idx + 1U) % AUDIO_TDM_THERMAL_QUEUE_DEPTH;
		thermal_count--;
	}

	for (size_t i = 0; i < AUDIO_WAVEFORM_THERMAL_PIXELS_PER_ROW; i++) {
		sys_put_be16((uint16_t)row_pixels[i],
			     &thermal_rows[thermal_write_idx][i * sizeof(int16_t)]);
	}

	thermal_row_indices[thermal_write_idx] = row_index;
	thermal_write_idx = (thermal_write_idx + 1U) % AUDIO_TDM_THERMAL_QUEUE_DEPTH;
	thermal_count++;

	k_spin_unlock(&stream_lock, key);
}

static void audio_tdm_notify_thread(void *arg1, void *arg2, void *arg3)
{
	static int last_notify_error;
	struct audio_waveform_packet packet;
	uint8_t mic_valid_len;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (1) {
		k_sem_take(&tdm_tick_sem, K_FOREVER);

		if (!capture_enabled || !notify_enabled) {
			continue;
		}

		packet_build(&packet);
		mic_valid_len = packet.mic_valid_len;

		if (mic_valid_len == 0U) {
			continue;
		}

retry_notify:
		int ret = bt_gatt_notify(NULL, &audio_waveform_service.attrs[4], &packet,
					 sizeof(packet));
		if (ret == -ENOMEM) {
			k_sleep(K_MSEC(5));
			goto retry_notify;
		}

		if (ret == 0) {
			last_notify_error = 0;
			k_spinlock_key_t key = k_spin_lock(&stream_lock);
			mic_ring_advance_payload(mic_valid_len);
			k_spin_unlock(&stream_lock, key);
		} else if ((ret != -ENOTCONN) && (ret != last_notify_error)) {
			last_notify_error = ret;
			LOG_WRN("Audio TDM notify failed: %d", ret);
		}
	}
}

static int audio_waveform_service_init(void)
{
	k_sem_init(&tdm_tick_sem, 0, 100);

	k_tid_t thread_id = k_thread_create(&audio_tdm_thread_data,
					    audio_tdm_thread_stack,
					    AUDIO_TDM_THREAD_STACK_SIZE,
					    audio_tdm_notify_thread,
					    NULL, NULL, NULL,
					    K_PRIO_PREEMPT(AUDIO_TDM_THREAD_PRIORITY),
					    0, K_NO_WAIT);

	(void)k_thread_name_set(thread_id, "AUDIO_TDM_BLE");
	return 0;
}

SYS_INIT(audio_waveform_service_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
