#include "audio_waveform_service.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(audio_waveform_service, CONFIG_BLE_LOG_LEVEL);

#ifndef CONFIG_AUDIO_SAMPLE_RATE_HZ
#define CONFIG_AUDIO_SAMPLE_RATE_HZ 0
#endif

#define AUDIO_WAVEFORM_NOTIFY_DIVISOR 20
#define AUDIO_WAVEFORM_QUEUE_DEPTH 4
#define AUDIO_WAVEFORM_THREAD_STACK_SIZE 1536
#define AUDIO_WAVEFORM_THREAD_PRIORITY 7

static uint8_t control_value;
static bool capture_enabled;
static bool notify_enabled;
static uint32_t source_block_count;
static uint32_t sequence;
static struct audio_waveform_packet latest_packet;
static int16_t scope_window[AUDIO_WAVEFORM_SAMPLE_COUNT];
static size_t scope_frame_count;
static bool scope_capture_active;
static uint32_t scope_sum_abs_l;
static uint32_t scope_sum_abs_r;
static uint16_t scope_peak_l;
static uint16_t scope_peak_r;
static int16_t scope_min_mono;
static int16_t scope_max_mono;

static struct k_thread audio_waveform_thread_data;
static K_THREAD_STACK_DEFINE(audio_waveform_thread_stack, AUDIO_WAVEFORM_THREAD_STACK_SIZE);
K_MSGQ_DEFINE(audio_waveform_queue, sizeof(struct audio_waveform_packet),
	      AUDIO_WAVEFORM_QUEUE_DEPTH, 4);

static uint16_t abs_i16_u16(int16_t value)
{
	return value == INT16_MIN ? 32768U : (uint16_t)abs(value);
}

static int8_t clamp_i8(int32_t value)
{
	if (value > INT8_MAX) {
		return INT8_MAX;
	}

	if (value < INT8_MIN) {
		return INT8_MIN;
	}

	return (int8_t)value;
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
	capture_enabled = (control_value & BIT(0)) != 0;

	if ((control_value & BIT(1)) != 0) {
		sequence = 0;
		source_block_count = 0;
		k_msgq_purge(&audio_waveform_queue);
	}

	LOG_INF("Audio waveform preview %s", capture_enabled ? "enabled" : "disabled");
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
	LOG_INF("Audio waveform notifications %s", notify_enabled ? "enabled" : "disabled");

	if (!notify_enabled) {
		k_msgq_purge(&audio_waveform_queue);
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
	struct audio_waveform_packet packet = {0};

	if (!capture_enabled || !notify_enabled) {
		return -EACCES;
	}

	if ((samples == NULL) || (frame_count == 0)) {
		return -EINVAL;
	}

	source_block_count++;

	if (!scope_capture_active) {
		if ((source_block_count % AUDIO_WAVEFORM_NOTIFY_DIVISOR) != 0) {
			return 0;
		}

		scope_capture_active = true;
		scope_frame_count = 0;
		scope_sum_abs_l = 0;
		scope_sum_abs_r = 0;
		scope_peak_l = 0;
		scope_peak_r = 0;
		scope_min_mono = INT16_MAX;
		scope_max_mono = INT16_MIN;
	}

	const size_t frames_to_copy = MIN(frame_count,
					 AUDIO_WAVEFORM_SAMPLE_COUNT - scope_frame_count);

	for (size_t i = 0; i < frames_to_copy; i++) {
		const int16_t left = samples[i * 2];
		const int16_t right = samples[(i * 2) + 1];
		const uint16_t abs_l = abs_i16_u16(left);
		const uint16_t abs_r = abs_i16_u16(right);
		const int32_t mono = ((int32_t)left + (int32_t)right) / 2;

		scope_window[scope_frame_count + i] = (int16_t)mono;
		scope_min_mono = MIN(scope_min_mono, (int16_t)mono);
		scope_max_mono = MAX(scope_max_mono, (int16_t)mono);
		scope_sum_abs_l += abs_l;
		scope_sum_abs_r += abs_r;
		scope_peak_l = MAX(scope_peak_l, abs_l);
		scope_peak_r = MAX(scope_peak_r, abs_r);
	}

	scope_frame_count += frames_to_copy;

	if (scope_frame_count < AUDIO_WAVEFORM_SAMPLE_COUNT) {
		return 0;
	}

	scope_capture_active = false;
	packet.sequence = sequence++;
	packet.sample_rate_hz = CONFIG_AUDIO_SAMPLE_RATE_HZ;
	packet.frame_count = scope_frame_count > UINT16_MAX ? UINT16_MAX : (uint16_t)scope_frame_count;
	packet.sample_count = AUDIO_WAVEFORM_SAMPLE_COUNT;
	packet.peak_l = scope_peak_l;
	packet.peak_r = scope_peak_r;
	packet.mean_abs_l = (uint16_t)(scope_sum_abs_l / scope_frame_count);
	packet.mean_abs_r = (uint16_t)(scope_sum_abs_r / scope_frame_count);
	packet.min_mono = scope_min_mono;
	packet.max_mono = scope_max_mono;
	const int32_t peak_to_peak = (int32_t)scope_max_mono - (int32_t)scope_min_mono;
	packet.peak_to_peak_mono = peak_to_peak > UINT16_MAX ? UINT16_MAX : (uint16_t)peak_to_peak;

	const uint16_t preview_peak = MAX(scope_peak_l, scope_peak_r);

	for (size_t i = 0; i < AUDIO_WAVEFORM_SAMPLE_COUNT; i++) {
		const int32_t mono = scope_window[i];
		const int32_t scaled = preview_peak > 0 ? (mono * 120) / preview_peak : 0;

		packet.samples[i] = clamp_i8(scaled);
	}

	return k_msgq_put(&audio_waveform_queue, &packet, K_NO_WAIT);
}

static void audio_waveform_notify_thread(void *arg1, void *arg2, void *arg3)
{
	int ret;
	static int last_notify_error;
	struct audio_waveform_packet packet;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (1) {
		ret = k_msgq_get(&audio_waveform_queue, &packet, K_FOREVER);
		if (ret != 0) {
			continue;
		}

		memcpy(&latest_packet, &packet, sizeof(latest_packet));

		if (!notify_enabled) {
			continue;
		}

		ret = bt_gatt_notify(NULL, &audio_waveform_service.attrs[4], &packet,
				     sizeof(packet));
		if (ret == 0) {
			last_notify_error = 0;
		} else if ((ret != -ENOTCONN) && (ret != last_notify_error)) {
			last_notify_error = ret;
			LOG_WRN("Audio waveform notify failed: %d", ret);
		}
	}
}

static int audio_waveform_service_init(void)
{
	k_tid_t thread_id = k_thread_create(&audio_waveform_thread_data,
					    audio_waveform_thread_stack,
					    AUDIO_WAVEFORM_THREAD_STACK_SIZE,
					    audio_waveform_notify_thread,
					    NULL, NULL, NULL,
					    K_PRIO_PREEMPT(AUDIO_WAVEFORM_THREAD_PRIORITY),
					    0, K_NO_WAIT);

	(void)k_thread_name_set(thread_id, "AUDIO_WAVE_BLE");
	return 0;
}

SYS_INIT(audio_waveform_service_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
