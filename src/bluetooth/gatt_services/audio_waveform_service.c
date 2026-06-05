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

#define AUDIO_TDM_MIC_RING_PACKETS 64U
#define AUDIO_TDM_MIC_RING_SAMPLES \
	(AUDIO_WAVEFORM_MIC_SAMPLES_PER_PACKET * AUDIO_TDM_MIC_RING_PACKETS)
#define AUDIO_TDM_THREAD_STACK_SIZE 1536
#define AUDIO_TDM_THREAD_PRIORITY 7
#define AUDIO_SAMPLE_PERIOD_HALF_US 125U
#define AUDIO_NOTIFY_RETRY_LIMIT 64U

BUILD_ASSERT(sizeof(struct audio_waveform_packet) == AUDIO_WAVEFORM_PACKET_SIZE);
BUILD_ASSERT(offsetof(struct audio_waveform_packet, first_sample_timestamp_us_le) == 0U);
BUILD_ASSERT(offsetof(struct audio_waveform_packet, mic_payload) == 4U);
BUILD_ASSERT(AUDIO_WAVEFORM_MIC_PAYLOAD_SIZE ==
	     AUDIO_WAVEFORM_MIC_SAMPLES_PER_PACKET * sizeof(int16_t));
BUILD_ASSERT(AUDIO_WAVEFORM_PACKET_SIZE <= 244U);

static uint8_t control_value;
static bool capture_enabled;
static bool notify_enabled;
static bool mic_stream_enabled;
static struct audio_waveform_packet latest_packet;

static struct k_spinlock stream_lock;

static int16_t mic_ring[AUDIO_TDM_MIC_RING_SAMPLES];
static uint16_t mic_read_idx;
static uint16_t mic_write_idx;
static uint16_t mic_count;
static uint64_t mic_read_timestamp_half_us;
static uint32_t dropped_mic_samples;
static uint32_t reported_dropped_mic_samples;

static struct k_thread audio_tdm_thread_data;
static K_THREAD_STACK_DEFINE(audio_tdm_thread_stack, AUDIO_TDM_THREAD_STACK_SIZE);

static struct k_sem tdm_tick_sem;
static void tdm_tick_handler(struct k_timer *timer);
K_TIMER_DEFINE(tdm_tick_timer, tdm_tick_handler, NULL);

static void tdm_tick_handler(struct k_timer *timer)
{
	k_sem_give(&tdm_tick_sem);
}

static void stream_cache_reset(void)
{
	k_spinlock_key_t key = k_spin_lock(&stream_lock);

	mic_read_idx = 0;
	mic_write_idx = 0;
	mic_count = 0;
	mic_read_timestamp_half_us = 0;
	dropped_mic_samples = 0;
	reported_dropped_mic_samples = 0;

	memset(&latest_packet, 0, sizeof(latest_packet));

	k_spin_unlock(&stream_lock, key);
}

static void mic_ring_push(int16_t sample)
{
	if (mic_count == AUDIO_TDM_MIC_RING_SAMPLES) {
		mic_read_idx = (mic_read_idx + 1U) % AUDIO_TDM_MIC_RING_SAMPLES;
		mic_count--;
		mic_read_timestamp_half_us += AUDIO_SAMPLE_PERIOD_HALF_US;
		dropped_mic_samples++;
	}

	mic_ring[mic_write_idx] = sample;
	mic_write_idx = (mic_write_idx + 1U) % AUDIO_TDM_MIC_RING_SAMPLES;
	mic_count++;
}

static bool mic_ring_take_packet(struct audio_waveform_packet *packet)
{
	if (mic_count < AUDIO_WAVEFORM_MIC_SAMPLES_PER_PACKET) {
		return false;
	}

	sys_put_le32((uint32_t)(mic_read_timestamp_half_us / 2U),
		     packet->first_sample_timestamp_us_le);

	for (uint16_t i = 0; i < AUDIO_WAVEFORM_MIC_SAMPLES_PER_PACKET; i++) {
		const uint16_t idx =
			(mic_read_idx + i) % AUDIO_TDM_MIC_RING_SAMPLES;
		const int16_t sample = mic_ring[idx];

		sys_put_le16((uint16_t)sample, &packet->mic_payload[i * sizeof(int16_t)]);
	}

	for (uint16_t i = 0; i < AUDIO_WAVEFORM_MIC_SAMPLES_PER_PACKET; i++) {
		mic_read_idx = (mic_read_idx + 1U) % AUDIO_TDM_MIC_RING_SAMPLES;
		mic_count--;
	}
	mic_read_timestamp_half_us +=
		(uint64_t)AUDIO_WAVEFORM_MIC_SAMPLES_PER_PACKET * AUDIO_SAMPLE_PERIOD_HALF_US;
	return true;
}

static bool packet_build(struct audio_waveform_packet *packet)
{
	k_spinlock_key_t key;
	bool packet_ready;
	bool report_overflow = false;
	uint32_t dropped_snapshot = 0;

	memset(packet, 0, sizeof(*packet));

	key = k_spin_lock(&stream_lock);
	packet_ready = mic_ring_take_packet(packet);
	if (packet_ready) {
		memcpy(&latest_packet, packet, sizeof(latest_packet));
	}
	if (dropped_mic_samples >=
	    reported_dropped_mic_samples + AUDIO_WAVEFORM_MIC_SAMPLES_PER_PACKET) {
		reported_dropped_mic_samples = dropped_mic_samples;
		dropped_snapshot = dropped_mic_samples;
		report_overflow = true;
	}
	k_spin_unlock(&stream_lock, key);

	if (report_overflow) {
		LOG_WRN("Audio PCM ring overflow: dropped=%u samples ring=%u packets",
			dropped_snapshot, AUDIO_TDM_MIC_RING_PACKETS);
	}

	return packet_ready;
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
		stream_cache_reset();
	}

	if (capture_enabled && notify_enabled) {
		k_timer_start(&tdm_tick_timer, K_USEC(AUDIO_WAVEFORM_PERIOD_US),
			      K_USEC(AUDIO_WAVEFORM_PERIOD_US));
	} else {
		k_timer_stop(&tdm_tick_timer);
	}

	LOG_INF("Audio PCM stream %s period=%uus packet=%uB samples=%u ring=%u packets",
		capture_enabled ? "enabled" : "disabled",
		AUDIO_WAVEFORM_PERIOD_US, AUDIO_WAVEFORM_PACKET_SIZE,
		AUDIO_WAVEFORM_MIC_SAMPLES_PER_PACKET, AUDIO_TDM_MIC_RING_PACKETS);

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
	LOG_INF("Audio PCM notifications %s", notify_enabled ? "enabled" : "disabled");

	if (notify_enabled && capture_enabled) {
		k_timer_start(&tdm_tick_timer, K_USEC(AUDIO_WAVEFORM_PERIOD_US),
			      K_USEC(AUDIO_WAVEFORM_PERIOD_US));
	} else {
		k_timer_stop(&tdm_tick_timer);
	}

	if (!notify_enabled) {
		stream_cache_reset();
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

int audio_waveform_service_submit_i2s_block(const int16_t *samples, size_t frame_count,
					    uint32_t first_sample_timestamp_us)
{
	if (!capture_enabled || !notify_enabled || !mic_stream_enabled) {
		return -EACCES;
	}

	if ((samples == NULL) || (frame_count == 0U)) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&stream_lock);

	if (mic_count == 0U) {
		mic_read_timestamp_half_us = (uint64_t)first_sample_timestamp_us * 2U;
	}

	for (size_t i = 0; i < frame_count; i++) {
		/* The fixed BLE PCM packet has one mono slot. Use the left I2S
		 * channel so each 244-byte notification carries 120 continuous
		 * samples and fits in one 251-byte Link Layer data PDU.
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
		mic_read_timestamp_half_us = 0;
	}

	k_spin_unlock(&stream_lock, key);
}

static void audio_tdm_notify_thread(void *arg1, void *arg2, void *arg3)
{
	static int last_notify_error;
	struct audio_waveform_packet packet;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (1) {
		k_sem_take(&tdm_tick_sem, K_FOREVER);

		if (!capture_enabled || !notify_enabled) {
			continue;
		}

		if (!packet_build(&packet)) {
			continue;
		}

		uint8_t retry_count = 0U;
retry_notify:
		int ret = bt_gatt_notify(NULL, &audio_waveform_service.attrs[4], &packet,
					 sizeof(packet));
		if ((ret == -ENOMEM) &&
		    capture_enabled && notify_enabled &&
		    (retry_count++ < AUDIO_NOTIFY_RETRY_LIMIT)) {
			k_sleep(K_MSEC(1));
			goto retry_notify;
		}

		if (ret == 0) {
			last_notify_error = 0;
		} else if ((ret != -ENOTCONN) && (ret != last_notify_error)) {
			last_notify_error = ret;
			LOG_WRN("Audio PCM notify failed: %d", ret);
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

	(void)k_thread_name_set(thread_id, "AUDIO_PCM_BLE");
	return 0;
}

SYS_INIT(audio_waveform_service_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
