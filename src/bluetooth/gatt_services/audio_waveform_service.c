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

#define AUDIO_WAVEFORM_CONTROL_ENABLE BIT(0)
#define AUDIO_WAVEFORM_CONTROL_RESET BIT(1)
#define AUDIO_WAVEFORM_CONTROL_WINDOW_SHIFT 2
#define AUDIO_WAVEFORM_CONTROL_WINDOW_MASK (BIT(2) | BIT(3))

#define AUDIO_WAVEFORM_DEFAULT_SAMPLE_RATE_HZ 48000U
#define AUDIO_WAVEFORM_NOTIFY_DIVISOR 20U
#define AUDIO_WAVEFORM_MAX_PLOT_POINTS 480U
#define AUDIO_WAVEFORM_QUEUE_DEPTH 8
#define AUDIO_WAVEFORM_THREAD_STACK_SIZE 1536
#define AUDIO_WAVEFORM_THREAD_PRIORITY 7

static uint8_t control_value;
static bool capture_enabled;
static bool notify_enabled;
static uint8_t window_mode;
static uint32_t source_block_count;
static uint32_t sequence;
static uint16_t window_id;
static struct audio_waveform_packet latest_packet;
static int16_t scope_points[AUDIO_WAVEFORM_MAX_PLOT_POINTS];
static uint16_t scope_point_count;
static uint16_t scope_target_frames;
static uint16_t scope_target_points;
static uint16_t scope_decimation;
static uint16_t scope_frame_count;
static bool scope_capture_active;
static uint32_t scope_sum_abs_l;
static uint32_t scope_sum_abs_r;
static uint16_t scope_peak_l;
static uint16_t scope_peak_r;
static int16_t scope_min_mono;
static int16_t scope_max_mono;
static int32_t decimation_sum;
static uint16_t decimation_count;

static struct k_thread audio_waveform_thread_data;
static K_THREAD_STACK_DEFINE(audio_waveform_thread_stack, AUDIO_WAVEFORM_THREAD_STACK_SIZE);
K_MSGQ_DEFINE(audio_waveform_queue, sizeof(struct audio_waveform_packet),
	      AUDIO_WAVEFORM_QUEUE_DEPTH, 4);

static uint16_t abs_i16_u16(int16_t value)
{
	return value == INT16_MIN ? 32768U : (uint16_t)abs(value);
}

static uint32_t waveform_sample_rate_hz(void)
{
	return CONFIG_AUDIO_SAMPLE_RATE_HZ > 0 ? CONFIG_AUDIO_SAMPLE_RATE_HZ :
						 AUDIO_WAVEFORM_DEFAULT_SAMPLE_RATE_HZ;
}

static void waveform_configure_window(void)
{
	const uint32_t sample_rate = waveform_sample_rate_hz();
	uint32_t target_frames;
	uint32_t target_points;

	switch (window_mode) {
	case 1:
		target_frames = sample_rate / 100U; /* 10 ms */
		target_points = target_frames;
		break;
	case 2:
		target_frames = sample_rate / 20U; /* 50 ms */
		target_points = AUDIO_WAVEFORM_MAX_PLOT_POINTS;
		break;
	case 3:
		target_frames = sample_rate / 10U; /* 100 ms */
		target_points = AUDIO_WAVEFORM_MAX_PLOT_POINTS;
		break;
	case 0:
	default:
		target_frames = AUDIO_WAVEFORM_SAMPLES_PER_PACKET; /* 2 ms at 48 kHz */
		target_points = AUDIO_WAVEFORM_SAMPLES_PER_PACKET;
		break;
	}

	target_frames = CLAMP(target_frames, 1U, UINT16_MAX);
	target_points = CLAMP(target_points, 1U, AUDIO_WAVEFORM_MAX_PLOT_POINTS);
	target_points = MIN(target_points, target_frames);

	scope_decimation = MAX(1U, target_frames / target_points);
	scope_target_points = (uint16_t)(target_frames / scope_decimation);
	scope_target_points = MIN(scope_target_points, AUDIO_WAVEFORM_MAX_PLOT_POINTS);
	scope_target_frames = (uint16_t)(scope_target_points * scope_decimation);
}

static void waveform_reset_capture(bool reset_sequence)
{
	scope_capture_active = false;
	scope_point_count = 0;
	scope_frame_count = 0;
	decimation_sum = 0;
	decimation_count = 0;
	source_block_count = 0;
	k_msgq_purge(&audio_waveform_queue);

	if (reset_sequence) {
		sequence = 0;
		window_id++;
	}
}

static void waveform_begin_capture(void)
{
	waveform_configure_window();
	scope_capture_active = true;
	scope_point_count = 0;
	scope_frame_count = 0;
	scope_sum_abs_l = 0;
	scope_sum_abs_r = 0;
	scope_peak_l = 0;
	scope_peak_r = 0;
	scope_min_mono = INT16_MAX;
	scope_max_mono = INT16_MIN;
	decimation_sum = 0;
	decimation_count = 0;
	window_id++;
}

static int waveform_queue_scope_packets(void)
{
	struct audio_waveform_packet packet;
	const uint16_t mean_l = scope_frame_count > 0 ?
				(uint16_t)(scope_sum_abs_l / scope_frame_count) : 0;
	const uint16_t mean_r = scope_frame_count > 0 ?
				(uint16_t)(scope_sum_abs_r / scope_frame_count) : 0;
	const int32_t peak_to_peak = (int32_t)scope_max_mono - (int32_t)scope_min_mono;
	const uint16_t p2p = peak_to_peak > UINT16_MAX ? UINT16_MAX : (uint16_t)peak_to_peak;
	uint16_t offset = 0;
	int ret = 0;

	k_msgq_purge(&audio_waveform_queue);

	while (offset < scope_point_count) {
		const uint16_t chunk_count = MIN(AUDIO_WAVEFORM_SAMPLES_PER_PACKET,
						scope_point_count - offset);

		memset(&packet, 0, sizeof(packet));
		packet.sequence = sequence++;
		packet.sample_rate_hz = waveform_sample_rate_hz();
		packet.frame_count = scope_target_frames;
		packet.peak_l = scope_peak_l;
		packet.peak_r = scope_peak_r;
		packet.mean_abs_l = mean_l;
		packet.mean_abs_r = mean_r;
		packet.sample_count = (uint8_t)chunk_count;
		packet.sample_format = AUDIO_WAVEFORM_SAMPLE_FORMAT_PCM16;
		packet.window_id = window_id;
		packet.sample_offset = offset;
		packet.total_sample_count = scope_point_count;
		packet.decimation = scope_decimation;
		memcpy(packet.samples, &scope_points[offset], chunk_count * sizeof(packet.samples[0]));
		packet.min_mono = scope_min_mono;
		packet.max_mono = scope_max_mono;
		packet.peak_to_peak_mono = p2p;

		ret = k_msgq_put(&audio_waveform_queue, &packet, K_NO_WAIT);
		if (ret != 0) {
			return ret;
		}

		offset += chunk_count;
	}

	return ret;
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
	window_mode = (control_value & AUDIO_WAVEFORM_CONTROL_WINDOW_MASK) >>
		      AUDIO_WAVEFORM_CONTROL_WINDOW_SHIFT;
	waveform_configure_window();

	if ((control_value & AUDIO_WAVEFORM_CONTROL_RESET) != 0) {
		waveform_reset_capture(true);
	}

	LOG_INF("Audio waveform preview %s mode=%u frames=%u points=%u decim=%u",
		capture_enabled ? "enabled" : "disabled", window_mode,
		scope_target_frames, scope_target_points, scope_decimation);
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
		scope_capture_active = false;
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

		waveform_begin_capture();
	}

	for (size_t i = 0; i < frame_count; i++) {
		const int16_t left = samples[i * 2];
		const int16_t right = samples[(i * 2) + 1];
		const uint16_t abs_l = abs_i16_u16(left);
		const uint16_t abs_r = abs_i16_u16(right);
		const int32_t mono = ((int32_t)left + (int32_t)right) / 2;

		scope_min_mono = MIN(scope_min_mono, (int16_t)mono);
		scope_max_mono = MAX(scope_max_mono, (int16_t)mono);
		scope_sum_abs_l += abs_l;
		scope_sum_abs_r += abs_r;
		scope_peak_l = MAX(scope_peak_l, abs_l);
		scope_peak_r = MAX(scope_peak_r, abs_r);

		decimation_sum += mono;
		decimation_count++;
		scope_frame_count++;

		if (decimation_count >= scope_decimation &&
		    scope_point_count < scope_target_points) {
			scope_points[scope_point_count++] =
				(int16_t)(decimation_sum / decimation_count);
			decimation_sum = 0;
			decimation_count = 0;
		}

		if (scope_frame_count >= scope_target_frames ||
		    scope_point_count >= scope_target_points) {
			scope_capture_active = false;
			return waveform_queue_scope_packets();
		}
	}

	return 0;
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
