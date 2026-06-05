#include "sensor_service.h"
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include "../SensorManager/SensorManager.h"
#include "../ParseInfo/SensorScheme.h"

#include "macros_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sensor_manager, CONFIG_MODULE_BUTTON_HANDLER_LOG_LEVEL);

#define MAX_SENSOR_REC_NAME_LENGTH 64
#define SENSOR_NOTIFY_RETRY_LIMIT 20U
#define STREAM_NOTIFY_RETRY_LIMIT 32U
#define THERMAL_STREAM_QUEUE_DEPTH (SENSOR_THERMAL_BLE_PACKETS_PER_FRAME * 3U)
#define IMU_STREAM_QUEUE_DEPTH 16U
#define SENSOR_STREAM_NOTIFY_STACK_SIZE 2048U
#define SENSOR_STREAM_NOTIFY_SPACING_MS 3U

#define SENSOR_DATA_ATTR_INDEX 4U
#define SENSOR_CONFIG_STATUS_ATTR_INDEX 7U
#define SENSOR_STREAM_DATA_ATTR_INDEX 12U

BUILD_ASSERT(sizeof(struct sensor_stream_packet) ==
	     SENSOR_STREAM_PACKET_SIZE_MAX);
BUILD_ASSERT(offsetof(struct sensor_stream_packet, payload) ==
	     SENSOR_STREAM_HEADER_SIZE);
BUILD_ASSERT(SENSOR_THERMAL_BLE_PACKETS_PER_FRAME ==
	     DIV_ROUND_UP(SENSOR_THERMAL_BLE_PIXELS_PER_FRAME,
			  SENSOR_THERMAL_BLE_PIXELS_PER_PACKET));
BUILD_ASSERT(SENSOR_STREAM_HEADER_SIZE +
	     SENSOR_IMU_BATCH_SAMPLES * SENSOR_IMU_BATCH_SAMPLE_SIZE <=
	     SENSOR_STREAM_PACKET_SIZE_MAX);

static struct k_thread thread_data_notify;
static struct k_thread thread_stream_notify;

static k_tid_t thread_id_notify;
static k_tid_t thread_id_stream_notify;

ZBUS_SUBSCRIBER_DEFINE(sensor_gatt_sub, CONFIG_BUTTON_MSG_SUB_QUEUE_SIZE);

ZBUS_CHAN_DECLARE(sensor_chan);
ZBUS_CHAN_DECLARE(bt_mgmt_chan);

static K_THREAD_STACK_DEFINE(thread_stack_notify, CONFIG_SENSOR_GATT_NOTIFY_STACK_SIZE);

K_MSGQ_DEFINE(gatt_queue, sizeof(struct sensor_data), CONFIG_SENSOR_GATT_SUB_QUEUE_SIZE, 4);

struct sensor_stream_queue_item {
	uint16_t len;
	struct sensor_stream_packet packet;
};

K_MSGQ_DEFINE(thermal_stream_queue, sizeof(struct sensor_stream_queue_item),
	      THERMAL_STREAM_QUEUE_DEPTH, 4);
K_MSGQ_DEFINE(imu_stream_queue, sizeof(struct sensor_stream_queue_item),
	      IMU_STREAM_QUEUE_DEPTH, 4);
K_SEM_DEFINE(stream_queue_sem, 0,
	     THERMAL_STREAM_QUEUE_DEPTH + IMU_STREAM_QUEUE_DEPTH);
K_MUTEX_DEFINE(imu_batch_mutex);
static K_THREAD_STACK_DEFINE(thread_stack_stream_notify,
			     SENSOR_STREAM_NOTIFY_STACK_SIZE);

struct imu_batch_sample {
	uint64_t timestamp_us;
	uint8_t payload[SENSOR_IMU_PAYLOAD_SIZE];
};

static struct imu_batch_sample imu_batch_samples[SENSOR_IMU_BATCH_SAMPLES];
static uint8_t imu_batch_count;

//static struct sensor_msg msg;
static struct sensor_data sensor_data;
static struct sensor_config config;

static bool notify_enabled = false;
static bool stream_notify_enabled = false;
static bool sensor_config_status_ntfy_enabled = false;
static uint16_t thermal_frame_sequence;
static uint16_t imu_batch_sequence;

void set_sensor_recording_name(const char *name);
static char sensor_recording_name[MAX_SENSOR_REC_NAME_LENGTH] = "sensor_log_";

static struct sensor_config *active_sensor_configs;
static size_t active_sensor_configs_size = 0;

static void connect_evt_handler(const struct zbus_channel *chan);
ZBUS_LISTENER_DEFINE(bt_mgmt_evt_listen2, connect_evt_handler); //static

void sensor_queue_listener_cb(const struct zbus_channel *chan);
ZBUS_LISTENER_DEFINE(sensor_queue_listener, sensor_queue_listener_cb);

static bool connection_complete = false;

static void reset_stream_queues(void)
{
	k_msgq_purge(&thermal_stream_queue);
	k_msgq_purge(&imu_stream_queue);
	k_sem_reset(&stream_queue_sem);

	k_mutex_lock(&imu_batch_mutex, K_FOREVER);
	imu_batch_count = 0U;
	k_mutex_unlock(&imu_batch_mutex);
}

static void connect_evt_handler(const struct zbus_channel *chan)
{
	const struct bt_mgmt_msg *msg;

	msg = zbus_chan_const_msg(chan);

	switch (msg->event) {
	case BT_MGMT_CONNECTED:
		connection_complete = true;
		break;

	case BT_MGMT_DISCONNECTED:
		connection_complete = false;
		notify_enabled = false;
		stream_notify_enabled = false;
		k_msgq_purge(&gatt_queue);
		reset_stream_queues();
		break;

	default:
		break;
	}
}

static void sensor_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				  uint16_t value)
{
	notify_enabled = (value == BT_GATT_CCC_NOTIFY);

	LOG_INF("Sensor data notifications %s", notify_enabled ? "enabled" : "disabled");

	k_msgq_purge(&gatt_queue);
}

static void stream_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				   uint16_t value)
{
	ARG_UNUSED(attr);

	stream_notify_enabled = (value == BT_GATT_CCC_NOTIFY);
	LOG_INF("Sensor Stream notifications %s",
		stream_notify_enabled ? "enabled" : "disabled");
	reset_stream_queues();
}

static void sensor_config_status_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				  uint16_t value)
{
	sensor_config_status_ntfy_enabled = (value == BT_GATT_CCC_NOTIFY);
}

static ssize_t write_config(struct bt_conn *conn,
			 const struct bt_gatt_attr *attr,
			 const void *buf,
			 uint16_t len, uint16_t offset, uint8_t flags)
{
	LOG_DBG("Attribute write, handle: %u, conn: %p", attr->handle, (void *)conn);

	if (len != sizeof(struct sensor_config)) {
		LOG_WRN("Write sensor config: Incorrect data length: Expected %i but got %i", sizeof(struct sensor_config), len);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	if (offset != 0) {
		LOG_WRN("Write sensor config: Incorrect data offset");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	struct sensor_config * config = (struct sensor_config *)buf;

	if (config->storageOptions == 0) {
		LOG_INF("Setup sensor ID %i (turned off)", config->sensorId);
	} else {
		LOG_INF("Setup sensor ID %i with samplerateIndex %i", config->sensorId, config->sampleRateIndex);
	}

	//stop_sensor_manager();
	config_sensor((struct sensor_config *) buf);

	return len;
}

static ssize_t read_sensor_rec_name(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr,
			  void *buf,
			  uint16_t len,
			  uint16_t offset)
{
	const char *name = get_sensor_recording_name();
	size_t name_len = strlen(name);

	if (offset > name_len) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, name, name_len);
}

static ssize_t write_sensor_rec_name(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr,
			  const void *buf,
			  uint16_t len, uint16_t offset, uint8_t flags)
{
	LOG_DBG("Attribute write, len: %u, handle: %u, conn: %p", len, attr->handle, (void *)conn);
	if (len > MAX_SENSOR_REC_NAME_LENGTH - 1) {
		LOG_WRN("Write sensor recording name: Data length exceeds maximum allowed length of %i", MAX_SENSOR_REC_NAME_LENGTH - 1);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	// Print buffer contents as a string (ensure null-termination for safety)
	char temp_buf[MAX_SENSOR_REC_NAME_LENGTH];
	strncpy(temp_buf, (const char *)buf, len);
	temp_buf[len] = '\0';

	LOG_DBG("Write sensor recording name: %s", temp_buf);

	set_sensor_recording_name(temp_buf);

	return len;
}

static ssize_t read_sensor_config_status(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr,
			  void *buf,
			  uint16_t len,
			  uint16_t offset)
{
	const uint16_t size = sizeof(struct sensor_config) * active_sensor_configs_size;
	LOG_DBG("Reading sensor config status");

	if (len < size) {
		LOG_WRN("Read sensor config status: Buffer too small: %u < %u", len, size);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset, active_sensor_configs, size);
}

BT_GATT_SERVICE_DEFINE(sensor_service,
BT_GATT_PRIMARY_SERVICE(BT_UUID_SENSOR),
BT_GATT_CHARACTERISTIC(BT_UUID_SENSOR_CONFIG,
            BT_GATT_CHRC_WRITE,
            BT_GATT_PERM_WRITE,
            NULL, write_config, &config),
BT_GATT_CHARACTERISTIC(BT_UUID_SENSOR_DATA,
			BT_GATT_CHRC_NOTIFY,
			BT_GATT_PERM_NONE,
			NULL, NULL, &sensor_data),
BT_GATT_CCC(sensor_ccc_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
BT_GATT_CHARACTERISTIC(BT_UUID_SENSOR_CONFIG_STATUS,
			BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			BT_GATT_PERM_READ,
			read_sensor_config_status, NULL, &active_sensor_configs),
BT_GATT_CCC(sensor_config_status_ccc_cfg_changed,
			BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
BT_GATT_CHARACTERISTIC(BT_UUID_SENSOR_RECORDING_NAME,
			BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			read_sensor_rec_name, write_sensor_rec_name, NULL),
BT_GATT_CHARACTERISTIC(BT_UUID_SENSOR_THERMAL_DATA,
			BT_GATT_CHRC_NOTIFY,
			BT_GATT_PERM_NONE,
			NULL, NULL, NULL),
BT_GATT_CCC(stream_ccc_cfg_changed,
			BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

static int notify_with_retry(const struct bt_gatt_attr *attr,
			     const void *data,
			     uint16_t len,
			     uint8_t retry_limit)
{
	int ret;

	for (uint8_t retry_count = 0U;; retry_count++) {
		ret = bt_gatt_notify(NULL, attr, data, len);
		if ((ret != -ENOMEM) || !connection_complete ||
		    (retry_count >= retry_limit)) {
			return ret;
		}
		k_sleep(K_MSEC(1));
	}
}

static void notification_task(void *arg1, void *arg2, void *arg3)
{
	int ret;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (1) {
		ret = k_msgq_get(&gatt_queue, &sensor_data, K_FOREVER);

		if (ret != 0) {
			LOG_WRN("No data to process");
			continue;
		}

		if (connection_complete && notify_enabled) {
			const uint16_t size = sizeof(sensor_data.id) + sizeof(sensor_data.size) + sizeof(sensor_data.time) + sensor_data.size;
			ret = notify_with_retry(
				&sensor_service.attrs[SENSOR_DATA_ATTR_INDEX],
				&sensor_data, size, SENSOR_NOTIFY_RETRY_LIMIT);
			if ((ret != 0) && (ret != -ENOTCONN)) {
				LOG_WRN("Failed to send sensor data: %d", ret);
			}
		}
	}
}

static void sensor_stream_notification_task(void *arg1, void *arg2, void *arg3)
{
	struct sensor_stream_queue_item item;
	static int last_notify_error;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (1) {
		k_sem_take(&stream_queue_sem, K_FOREVER);

		/*
		 * Thermal packets have transport priority. IMU sampling is not
		 * deprioritized; only its batched BLE packets wait behind a pending
		 * IR packet.
		 */
		int ret = k_msgq_get(&thermal_stream_queue, &item, K_NO_WAIT);
		if (ret != 0) {
			ret = k_msgq_get(&imu_stream_queue, &item, K_NO_WAIT);
		}
		if (ret != 0) {
			continue;
		}

		if (!connection_complete || !stream_notify_enabled) {
			continue;
		}

		ret = notify_with_retry(
			&sensor_service.attrs[SENSOR_STREAM_DATA_ATTR_INDEX],
			&item.packet, item.len, STREAM_NOTIFY_RETRY_LIMIT);

		if (ret == 0) {
			last_notify_error = 0;
			k_sleep(K_MSEC(SENSOR_STREAM_NOTIFY_SPACING_MS));
		} else if ((ret != -ENOTCONN) && (ret != last_notify_error)) {
			last_notify_error = ret;
			LOG_WRN("Sensor Stream notify failed: %d", ret);
		}
	}
}

static int enqueue_stream_packet(struct k_msgq *queue,
				 const struct sensor_stream_queue_item *item)
{
	int ret = k_msgq_put(queue, item, K_NO_WAIT);
	if (ret == 0) {
		k_sem_give(&stream_queue_sem);
	}
	return ret;
}

int sensor_service_submit_thermal_frame(const int16_t *pixels,
					size_t pixel_count,
					uint64_t timestamp_us)
{
	if ((pixels == NULL) ||
	    (pixel_count != SENSOR_THERMAL_BLE_PIXELS_PER_FRAME)) {
		return -EINVAL;
	}

	if (!connection_complete || !stream_notify_enabled) {
		return -EACCES;
	}

	/*
	 * Increment at capture time, before checking transport capacity. A queue
	 * overflow therefore leaves a visible frame-sequence gap in the Web log.
	 */
	const uint16_t frame_sequence = thermal_frame_sequence++;

	/* Queue complete frames only; never enqueue a truncated frame prefix. */
	if (k_msgq_num_free_get(&thermal_stream_queue) <
	    SENSOR_THERMAL_BLE_PACKETS_PER_FRAME) {
		return -ENOMEM;
	}

	for (uint8_t packet_index = 0;
	     packet_index < SENSOR_THERMAL_BLE_PACKETS_PER_FRAME;
	     packet_index++) {
		struct sensor_stream_queue_item item = { 0 };
		const uint16_t pixel_offset =
			(uint16_t)packet_index * SENSOR_THERMAL_BLE_PIXELS_PER_PACKET;
		const uint8_t packet_pixels = (uint8_t)MIN(
			(size_t)SENSOR_THERMAL_BLE_PIXELS_PER_PACKET,
			pixel_count - pixel_offset);

		sys_put_le64(timestamp_us, item.packet.timestamp_us_le);
		sys_put_le16(frame_sequence, item.packet.sequence_le);
		item.packet.packet_type = SENSOR_STREAM_TYPE_THERMAL;
		item.packet.item_count = packet_pixels;
		sys_put_le16(pixel_offset, item.packet.item_offset_le);
		item.packet.packet_index = packet_index;
		item.packet.packet_count = SENSOR_THERMAL_BLE_PACKETS_PER_FRAME;

		for (uint8_t i = 0; i < packet_pixels; i++) {
			sys_put_le16((uint16_t)pixels[pixel_offset + i],
				     &item.packet.payload[i * sizeof(int16_t)]);
		}

		item.len = SENSOR_STREAM_HEADER_SIZE +
			   (uint16_t)packet_pixels * sizeof(int16_t);

		int ret = enqueue_stream_packet(&thermal_stream_queue, &item);
		if (ret != 0) {
			LOG_WRN("Thermal IR queue failed at packet %u/%u: %d",
				(unsigned int)packet_index,
				(unsigned int)SENSOR_THERMAL_BLE_PACKETS_PER_FRAME,
				ret);
			return ret;
		}
	}

	return 0;
}

static int enqueue_imu_batch_locked(void)
{
	if (imu_batch_count == 0U) {
		return 0;
	}

	struct sensor_stream_queue_item item = { 0 };
	const uint64_t base_timestamp_us = imu_batch_samples[0].timestamp_us;
	const uint16_t sequence = imu_batch_sequence++;

	sys_put_le64(base_timestamp_us, item.packet.timestamp_us_le);
	sys_put_le16(sequence, item.packet.sequence_le);
	item.packet.packet_type = SENSOR_STREAM_TYPE_IMU_BATCH;
	item.packet.item_count = imu_batch_count;
	sys_put_le16(0U, item.packet.item_offset_le);
	item.packet.packet_index = 0U;
	item.packet.packet_count = 1U;

	for (uint8_t i = 0U; i < imu_batch_count; i++) {
		const size_t offset = (size_t)i * SENSOR_IMU_BATCH_SAMPLE_SIZE;
		const uint64_t delta_us =
			imu_batch_samples[i].timestamp_us - base_timestamp_us;

		sys_put_le32((uint32_t)MIN(delta_us, (uint64_t)UINT32_MAX),
			     &item.packet.payload[offset]);
		memcpy(&item.packet.payload[offset + sizeof(uint32_t)],
		       imu_batch_samples[i].payload, SENSOR_IMU_PAYLOAD_SIZE);
	}

	item.len = SENSOR_STREAM_HEADER_SIZE +
		   (uint16_t)imu_batch_count * SENSOR_IMU_BATCH_SAMPLE_SIZE;
	imu_batch_count = 0U;

	int ret = enqueue_stream_packet(&imu_stream_queue, &item);
	if (ret != 0) {
		LOG_WRN("IMU batch queue full, sequence=%u", sequence);
	}
	return ret;
}

int sensor_service_submit_imu_sample(const struct sensor_data *sample)
{
	if ((sample == NULL) || (sample->id != ID_IMU) ||
	    (sample->size != SENSOR_IMU_PAYLOAD_SIZE)) {
		return -EINVAL;
	}

	if (!connection_complete || !stream_notify_enabled) {
		return -EACCES;
	}

	k_mutex_lock(&imu_batch_mutex, K_FOREVER);

	struct imu_batch_sample *dst = &imu_batch_samples[imu_batch_count++];
	dst->timestamp_us = sample->time;
	memcpy(dst->payload, sample->data, SENSOR_IMU_PAYLOAD_SIZE);

	int ret = 0;
	if (imu_batch_count == SENSOR_IMU_BATCH_SAMPLES) {
		ret = enqueue_imu_batch_locked();
	}

	k_mutex_unlock(&imu_batch_mutex);
	return ret;
}

int sensor_service_flush_imu_batch(void)
{
	k_mutex_lock(&imu_batch_mutex, K_FOREVER);
	int ret;

	if (!connection_complete || !stream_notify_enabled) {
		imu_batch_count = 0U;
		ret = -EACCES;
	} else {
		ret = enqueue_imu_batch_locked();
	}

	k_mutex_unlock(&imu_batch_mutex);
	return ret;
}

void sensor_queue_listener_cb(const struct zbus_channel *chan) {
	int ret;
	const struct sensor_msg * msg;
    
    msg = (struct sensor_msg *)zbus_chan_const_msg(&sensor_chan);

	if (msg->stream) {
		ret = k_msgq_put(&gatt_queue, &msg->data, K_NO_WAIT);

		if (ret) {
			LOG_WRN("ble sensor stream queue full");
		}
	}
}

int init_sensor_config_status() {
	struct ParseInfoScheme *parse_info_scheme = getParseInfoScheme();

	// Initialize the active sensor configs list
	active_sensor_configs_size = parse_info_scheme->sensorCount;
	active_sensor_configs = k_malloc(sizeof(struct sensor_config) * active_sensor_configs_size);
	if (active_sensor_configs == NULL) {
		LOG_ERR("Failed to allocate memory for active sensor configs");
		return -1;
	}

	for (size_t i = 0; i < active_sensor_configs_size; i++) {
		struct SensorScheme *sensor_scheme = getSensorSchemeForId(parse_info_scheme->sensorIds[i]);
		LOG_DBG("Initializing sensor config state for sensor with id %d", sensor_scheme->id);

		active_sensor_configs[i].sensorId = sensor_scheme->id;
		if (sensor_scheme->configOptions.availableOptions & FREQUENCIES_DEFINED) {
			active_sensor_configs[i].sampleRateIndex = sensor_scheme->configOptions.frequencyOptions.defaultFrequencyIndex;
		} else {
			active_sensor_configs[i].sampleRateIndex = 0; // Default to 0 if frequencies are not defined
		}
		active_sensor_configs[i].storageOptions = 0; // Default storage options
	}

	LOG_DBG("Sensor config status initialized");
	return 0;
}

int set_sensor_config_status(struct sensor_config config) {
	LOG_DBG("Setting sensor config status for sensorId: %i", config.sensorId);

	ssize_t sensor_config_index = -1;
	for (size_t i = 0; i < active_sensor_configs_size; i++) {
		if (active_sensor_configs[i].sensorId == config.sensorId) {
			sensor_config_index = i;
			break;
		}
	}

	if (sensor_config_index >= 0) {
		active_sensor_configs[sensor_config_index] = config;
		LOG_DBG("Found sensor config");
	} else {
		LOG_DBG("Sensor config not found, adding new sensor config");
		// allocate more space for the new sensor config list
		active_sensor_configs_size++;
		struct sensor_config *new_active_sensor_configs = k_realloc(active_sensor_configs, active_sensor_configs_size);
		if (new_active_sensor_configs == NULL) {
			LOG_ERR("Failed to allocate memory for new sensor config");
			return -1;
		}
		active_sensor_configs = new_active_sensor_configs;
		active_sensor_configs[active_sensor_configs_size - 1] = config;
	}

	if (sensor_config_status_ntfy_enabled) {
		LOG_DBG("Sensor config status notification, notifying %zu active sensor configs", active_sensor_configs_size);
        struct bt_gatt_notify_params params = {
            .attr   = &sensor_service.attrs[SENSOR_CONFIG_STATUS_ATTR_INDEX],
            .data   = active_sensor_configs,
            .len    = sizeof(struct sensor_config) * active_sensor_configs_size,
        };
        int ret = bt_gatt_notify_cb(NULL, &params);

		if (ret) {
			LOG_ERR("Failed to notify sensor config status, error code: %d", ret);
			return ret;
		}
	}

	return 0;
}

int init_sensor_service() {
	int ret;

	thread_id_notify = k_thread_create(
		&thread_data_notify, thread_stack_notify,
		CONFIG_SENSOR_GATT_NOTIFY_STACK_SIZE, notification_task, NULL,
		NULL, NULL, K_PRIO_PREEMPT(CONFIG_SENSOR_GATT_NOTIFY_THREAD_PRIO), 0, K_NO_WAIT);
	
	ret = k_thread_name_set(thread_id_notify, "SENSOR_GATT_NOTIFY");
	if (ret) {
		LOG_ERR("Failed to create sensor_msg thread");
		return ret;
	}

	thread_id_stream_notify = k_thread_create(
		&thread_stream_notify, thread_stack_stream_notify,
		SENSOR_STREAM_NOTIFY_STACK_SIZE,
		sensor_stream_notification_task,
		NULL, NULL, NULL,
		K_PRIO_PREEMPT(CONFIG_SENSOR_GATT_NOTIFY_THREAD_PRIO + 1),
		0, K_NO_WAIT);

	ret = k_thread_name_set(thread_id_stream_notify, "SENSOR_STREAM_BLE");
	if (ret) {
		LOG_ERR("Failed to create Sensor Stream notify thread");
		return ret;
	}

    ret = zbus_chan_add_obs(&sensor_chan, &sensor_queue_listener, ZBUS_ADD_OBS_TIMEOUT_MS);
	if (ret) {
		LOG_ERR("Failed to add sensor sub");
		return ret;
	}

	ret = zbus_chan_add_obs(&bt_mgmt_chan, &bt_mgmt_evt_listen2, ZBUS_ADD_OBS_TIMEOUT_MS);
	if (ret) {
		LOG_ERR("Failed to add bt_mgmt listener");
		return ret;
	}

	init_sensor_config_status();

    return 0;
}

const char *get_sensor_recording_name() {
	return sensor_recording_name;
}

/**
 * @brief Set the sensor recording name object.
 * 
 * @param name A pointer to the name string.
 * Has to be a valid string with a length greater than 0
 * and 0 terminated.
 */
void set_sensor_recording_name(const char *name) {
	if (name == NULL || strlen(name) == 0) {
		LOG_WRN("Invalid sensor recording name");
		return;
	}

	strncpy(sensor_recording_name, name, sizeof(sensor_recording_name) - 1);
	sensor_recording_name[sizeof(sensor_recording_name) - 1] = '\0';
}
