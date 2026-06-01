#include "sensor_service.h"
#include <zephyr/zbus/zbus.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include "../SensorManager/SensorManager.h"
#include "macros_common.h"
#include "SDLogger.h"
#include "PowerManager.h"
#include <errno.h>
#include "audio_datapath.h"

#include "StateIndicator.h"

#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <stdarg.h>
#include <string.h>
LOG_MODULE_REGISTER(sd_logger, CONFIG_LOG_DEFAULT_LEVEL);

ZBUS_CHAN_DECLARE(sd_card_chan);

void sensor_listener_cb(const struct zbus_channel *chan);

K_MSGQ_DEFINE(sd_sensor_queue, sizeof(sensor_data), CONFIG_SENSOR_SD_SUB_QUEUE_SIZE, 4);
ZBUS_LISTENER_DEFINE(sensor_data_listener, sensor_listener_cb);

// Define thread stack
K_THREAD_STACK_DEFINE(thread_stack, CONFIG_SENSOR_SD_STACK_SIZE);

ZBUS_CHAN_DECLARE(sensor_chan);

void sd_listener_callback(const struct zbus_channel *chan);

ZBUS_LISTENER_DEFINE(sd_card_event_listener, sd_listener_callback);

static struct k_thread thread_data;
static k_tid_t thread_id;

struct ring_buf ring_buffer;
struct k_mutex ring_mutex;   // Protects ring_buffer operations
struct k_mutex file_mutex;   // Protects sd_card open/write/close
uint8_t buffer[BUFFER_SIZE];  // Ring Buffer Speicher

// Coordination flags (atomic because they are accessed from multiple threads)
static atomic_t g_stop_writing;   // 1 while end()/flush/close is in progress
static atomic_t g_sd_removed;     // 1 if SD was removed while recording

uint32_t count_max_buffer_fill = 0;

struct k_poll_signal logger_sig;
static struct k_poll_event logger_evt =
		 K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &logger_sig);

static void csv_append(char *buf, size_t cap, size_t *pos, const char *fmt, ...)
{
	if (*pos >= cap) {
		return;
	}

	va_list args;
	va_start(args, fmt);
	int written = vsnprintk(&buf[*pos], cap - *pos, fmt, args);
	va_end(args);

	if (written <= 0) {
		return;
	}

	const size_t remaining = cap - *pos;
	*pos += MIN((size_t)written, remaining - 1U);
}

static void csv_append_float(char *buf, size_t cap, size_t *pos, float value)
{
	if (value != value) {
		csv_append(buf, cap, pos, "nan");
		return;
	}

	bool negative = value < 0.0f;
	if (negative) {
		value = -value;
	}

	uint32_t whole = (uint32_t)value;
	uint32_t frac = (uint32_t)((value - (float)whole) * 1000000.0f + 0.5f);
	if (frac >= 1000000U) {
		whole++;
		frac -= 1000000U;
	}

	csv_append(buf, cap, pos, "%s%u.%06u", negative ? "-" : "",
		   (unsigned int)whole, (unsigned int)frac);
}

static const char *csv_sensor_prefix(uint8_t sensor_id)
{
	switch (sensor_id) {
	case ID_IMU:
		return "IMU";
	case ID_THERMAL:
		return "IR";
	case ID_MICRO:
		return "Audio";
	case ID_PPG:
		return "PPG";
	case ID_OPTTEMP:
		return "OptTemp";
	case ID_TEMP_BARO:
		return "TempBaro";
	case ID_BONE_CONDUCTION:
		return "Bone";
	default:
		return "SensorRaw";
	}
}

static SdLoggerCsvFileIndex csv_file_index_for_sensor(uint8_t sensor_id)
{
	switch (sensor_id) {
	case ID_IMU:
		return SD_LOGGER_CSV_FILE_IMU;
	case ID_THERMAL:
		return SD_LOGGER_CSV_FILE_THERMAL;
	case ID_MICRO:
		return SD_LOGGER_CSV_FILE_MICRO_INDEX;
	case ID_PPG:
		return SD_LOGGER_CSV_FILE_PPG;
	case ID_OPTTEMP:
		return SD_LOGGER_CSV_FILE_OPT_TEMP;
	case ID_TEMP_BARO:
		return SD_LOGGER_CSV_FILE_TEMP_BARO;
	case ID_BONE_CONDUCTION:
		return SD_LOGGER_CSV_FILE_BONE;
	default:
		return SD_LOGGER_CSV_FILE_GENERIC;
	}
}

static int write_text_locked(struct fs_file_t *file, const char *text, size_t len)
{
	ssize_t written = fs_write(file, text, len);
	if (written < 0) {
		return (int)written;
	}
	return ((size_t)written == len) ? 0 : -EIO;
}

SDLogger::SDLogger() {
    sd_card = &sdcard_manager;
    k_mutex_init(&ring_mutex);
    k_mutex_init(&file_mutex);
    atomic_clear(&g_stop_writing);
    atomic_clear(&g_sd_removed);

    for (size_t i = 0; i < SD_LOGGER_CSV_FILE_COUNT; i++) {
        fs_file_t_init(&csv_files[i].file);
        csv_files[i].is_open = false;
    }
    fs_file_t_init(&audio_left_file.file);
    fs_file_t_init(&audio_right_file.file);
    audio_left_file.is_open = false;
    audio_right_file.is_open = false;
}

SDLogger::~SDLogger() {

}

//static bool _prio_boost = false;

void sensor_listener_cb(const struct zbus_channel *chan) {
    const sensor_msg* msg = (sensor_msg*)zbus_chan_const_msg(chan);

	if (msg->sd) {
        int ret = sdlogger.write_sensor_data(msg->data);
        if (ret < 0) {
            LOG_WRN("Failed to enqueue sensor data for SD: %d", ret);
        }
	}
}


void sd_listener_callback(const struct zbus_channel *chan)
{
    const struct sd_msg * sd_msg_event = (sd_msg *)zbus_chan_const_msg(&sd_card_chan);

    if (sdlogger.is_open && sd_msg_event->removed) {
        // Signal SD thread to stop writing immediately.
        atomic_set(&g_sd_removed, 1);
        state_indicator.set_sd_state(SD_FAULT);
        LOG_ERR("SD card removed mid recording. Stop recording.");

        // Wake the SD thread so it can react quickly.
        k_poll_signal_raise(&logger_sig, 0);
    }
}


void SDLogger::sensor_sd_task() {
    int ret;

    while (1) {
        ret = k_poll(&logger_evt, 1, K_FOREVER);

        if (ret < 0) {
            LOG_ERR("k_poll failed: %d", ret);
            continue;
        }

        unsigned int signaled;
        int result;
        k_poll_signal_check(&logger_sig, &signaled, &result);

        if (signaled == 0) {
            LOG_DBG("Poll woke up without signal");
            continue;
        }

        // If a close/flush is in progress, do not write concurrently.
        if (atomic_get(&g_stop_writing)) {
            k_poll_signal_reset(&logger_sig);
            continue;
        }

        // If SD was removed, stop writing and drop buffered data.
        if (atomic_get(&g_sd_removed)) {
            k_mutex_lock(&ring_mutex, K_FOREVER);
            ring_buf_reset(&ring_buffer);
            k_mutex_unlock(&ring_mutex);
            for (size_t i = 0; i < SD_LOGGER_CSV_FILE_COUNT; i++) {
                sdlogger.csv_files[i].is_open = false;
            }
            sdlogger.audio_left_file.is_open = false;
            sdlogger.audio_right_file.is_open = false;
            sdlogger.is_open = false;
            k_poll_signal_reset(&logger_sig);
            continue;
        }

        if (!sdcard_manager.is_mounted()) {
            state_indicator.set_sd_state(SD_FAULT);
            LOG_ERR("SD Card not mounted!");
            return;
        }

        uint32_t fill = ring_buf_size_get(&ring_buffer);

        if (fill >= SD_BLOCK_SIZE) {
            if (fill > count_max_buffer_fill) {
                count_max_buffer_fill = fill;
            }

            uint8_t *data = nullptr;

            // Claim up to one SD block from the ring buffer under lock.
            k_mutex_lock(&ring_mutex, K_FOREVER);
            uint32_t claimed = ring_buf_get_claim(&ring_buffer, &data, SD_BLOCK_SIZE);
            k_mutex_unlock(&ring_mutex);

            if (claimed == 0 || data == nullptr) {
                // Nothing to write right now.
                k_poll_signal_reset(&logger_sig);
                continue;
            }

            // Write the claimed bytes under file lock.
            size_t write_size = claimed;
            int written;
            k_mutex_lock(&file_mutex, K_FOREVER);
            written = sdlogger.sd_card->write((char*)data, &write_size, false);
            k_mutex_unlock(&file_mutex);

            if (written < 0) {
                state_indicator.set_sd_state(SD_FAULT);
                LOG_ERR("SD write failed: %d", written);

                // Do not advance the ring buffer on error.
                // Wakeups will continue; user can call end().
                k_poll_signal_reset(&logger_sig);
                continue;
            }

            // Advance ring buffer by the number of bytes actually written.
            k_mutex_lock(&ring_mutex, K_FOREVER);
            ring_buf_get_finish(&ring_buffer, (uint32_t)written);
            k_mutex_unlock(&ring_mutex);
        } else {
            k_yield();
        }

        k_poll_signal_reset(&logger_sig);

        STACK_USAGE_PRINT("sensor_msg_thread", &sdlogger.thread_data);
    }
}

int SDLogger::init() {
    int ret;

    sd_card->init();

    ring_buf_init(&ring_buffer, BUFFER_SIZE, buffer);

    atomic_clear(&g_stop_writing);
    atomic_clear(&g_sd_removed);

    //set_ring_buffer(&ring_buffer);

    k_poll_signal_init(&logger_sig);

	thread_id = k_thread_create(
		&thread_data, thread_stack,
		CONFIG_SENSOR_SD_STACK_SIZE, (k_thread_entry_t)sensor_sd_task, NULL,
		NULL, NULL, K_PRIO_PREEMPT(CONFIG_SENSOR_SD_THREAD_PRIO), 0, K_NO_WAIT);
	
	ret = k_thread_name_set(thread_id, "SENSOR_SD_SUB");
	if (ret) {
		LOG_ERR("Failed to create sensor_msg thread");
		return ret;
	}

    ret = zbus_chan_add_obs(&sensor_chan, &sensor_data_listener, ZBUS_ADD_OBS_TIMEOUT_MS);
    if (ret) {
        LOG_ERR("Failed to add sensor sub");
        return ret;
    }

    ret = zbus_chan_add_obs(&sd_card_chan, &sd_card_event_listener, ZBUS_ADD_OBS_TIMEOUT_MS);
	if (ret) {
		LOG_ERR("Failed to add sd sub");
		return ret;
	}

    return 0;
}

/**
 * @brief Begin logging to a file
 * @param filename Base filename without extension
 * @return 0 on success, negative error code on failure
 * 
 * Opens a file for logging with .oe extension appended to the filename.
 * Returns -EBUSY if logger is already open or -ENODEV if SD card not initialized.
 */
int SDLogger::begin(const std::string& filename) {
    int ret;

    if (is_open) {
        LOG_ERR("Logger already open");
        return -EBUSY;
    }

    if (!sd_card->is_mounted()) {
        ret = sd_card->mount();
        if (ret < 0) {
            state_indicator.set_sd_state(SD_FAULT);
            LOG_ERR("Failed to mount sd card: %d", ret);
            return ret;
        }
    }

    LOG_INF("OPEN FILE: %s", filename.c_str());

    session_timestamp = std::to_string(micros());
    for (size_t i = 0; i < SD_LOGGER_CSV_FILE_COUNT; i++) {
        fs_file_t_init(&csv_files[i].file);
        csv_files[i].is_open = false;
    }
    fs_file_t_init(&audio_left_file.file);
    fs_file_t_init(&audio_right_file.file);
    audio_left_file.is_open = false;
    audio_right_file.is_open = false;

    std::string full_filename = filename + ".oe";
    k_mutex_lock(&file_mutex, K_FOREVER);
    ret = sd_card->open_file(full_filename, true, false, true);
    k_mutex_unlock(&file_mutex);
    if (ret < 0) {
        state_indicator.set_sd_state(SD_FAULT);
        LOG_ERR("Failed to open file: %d", ret);
        return ret;
    }

    // Ensure no concurrent end()/flush is running
    atomic_clear(&g_stop_writing);
    atomic_clear(&g_sd_removed);

    current_file = full_filename;
    is_open = true;

    k_mutex_lock(&ring_mutex, K_FOREVER);
    ring_buf_reset(&ring_buffer);
    k_mutex_unlock(&ring_mutex);

    ret = write_header();
    if (ret < 0) {
        state_indicator.set_sd_state(SD_FAULT);
        LOG_ERR("Failed to write header: %d", ret);
        return ret;
    }

    k_poll_signal_raise(&logger_sig, 0);

    return 0;
}

int SDLogger::write_header() {
    size_t header_size = sizeof(FileHeader);
    uint8_t header_buffer[header_size];
    FileHeader* header = reinterpret_cast<FileHeader*>(header_buffer);

    header->version = SENSOR_LOG_VERSION;
    header->timestamp = micros();

    int ret;
    k_mutex_lock(&file_mutex, K_FOREVER);
    ret = sd_card->write((char *)header_buffer, &header_size, false);
    k_mutex_unlock(&file_mutex);
    return ret;
}

int SDLogger::ensure_csv_file(uint8_t sensor_id, SdLoggerCsvFileIndex *file_idx) {
    if (!is_open || session_timestamp.empty()) {
        return -ENODEV;
    }

    const SdLoggerCsvFileIndex idx = csv_file_index_for_sensor(sensor_id);
    if (file_idx != nullptr) {
        *file_idx = idx;
    }

    CsvFile &entry = csv_files[idx];
    if (entry.is_open) {
        return 0;
    }

    const char *prefix = csv_sensor_prefix(sensor_id);
    std::string path = std::string("/SD:/") + prefix + "_" + session_timestamp + ".csv";

    fs_file_t_init(&entry.file);

    k_mutex_lock(&file_mutex, K_FOREVER);
    int ret = fs_open(&entry.file, path.c_str(),
                      FS_O_CREATE | FS_O_WRITE | FS_O_APPEND);
    if (ret) {
        k_mutex_unlock(&file_mutex);
        LOG_WRN("Failed to open CSV log %s: %d", path.c_str(), ret);
        return ret;
    }
    entry.is_open = true;

    char header[640];
    size_t pos = 0;

    switch (sensor_id) {
    case ID_IMU:
        csv_append(header, sizeof(header), &pos,
                   "timestamp_us,accel_x_mps2,accel_y_mps2,accel_z_mps2,"
                   "gyro_x_dps,gyro_y_dps,gyro_z_dps,"
                   "mag_x_uT,mag_y_uT,mag_z_uT,payload_size");
        break;
    case ID_THERMAL:
        csv_append(header, sizeof(header), &pos,
                   "timestamp_us,chunk_index,pixel_count");
        for (int i = 0; i < 16; i++) {
            csv_append(header, sizeof(header), &pos, ",pixel_%02d_raw", i);
        }
        csv_append(header, sizeof(header), &pos, ",payload_size");
        break;
    case ID_MICRO:
        csv_append(header, sizeof(header), &pos,
                   "timestamp_us,channel,byte_count,sample_rate_hz");
        break;
    default:
        csv_append(header, sizeof(header), &pos,
                   "timestamp_us,sensor_id,payload_size");
        break;
    }

    if (sensor_id != ID_MICRO) {
        for (int i = 0; i < SENSOR_DATA_FIXED_LENGTH; i++) {
            csv_append(header, sizeof(header), &pos, ",payload_%02d", i);
        }
    }
    csv_append(header, sizeof(header), &pos, "\n");

    ret = write_text_locked(&entry.file, header, pos);
    if (ret) {
        LOG_WRN("Failed to write CSV header for sensor %u: %d",
                (unsigned int)sensor_id, ret);
    }

    k_mutex_unlock(&file_mutex);
    return ret;
}

int SDLogger::ensure_audio_file(bool left_channel) {
    if (!is_open || session_timestamp.empty()) {
        return -ENODEV;
    }

    AudioFile &entry = left_channel ? audio_left_file : audio_right_file;
    if (entry.is_open) {
        return 0;
    }

    const char *suffix = left_channel ? "_L.pcm" : "_R.pcm";
    std::string path = std::string("/SD:/Audio_") + session_timestamp + suffix;

    fs_file_t_init(&entry.file);

    k_mutex_lock(&file_mutex, K_FOREVER);
    int ret = fs_open(&entry.file, path.c_str(),
                      FS_O_CREATE | FS_O_WRITE | FS_O_APPEND);
    if (!ret) {
        entry.is_open = true;
    } else {
        LOG_WRN("Failed to open audio log %s: %d", path.c_str(), ret);
    }
    k_mutex_unlock(&file_mutex);

    return ret;
}

int SDLogger::write_sensor_csv(const sensor_data& msg) {
    if (!is_open || atomic_get(&g_stop_writing) || atomic_get(&g_sd_removed)) {
        return -ENODEV;
    }

    SdLoggerCsvFileIndex file_idx;
    int ret = ensure_csv_file(msg.id, &file_idx);
    if (ret) {
        return ret;
    }

    char line[768];
    size_t pos = 0;
    const uint8_t payload_size = MIN(msg.size, (uint8_t)SENSOR_DATA_FIXED_LENGTH);

    csv_append(line, sizeof(line), &pos, "%llu",
               (unsigned long long)msg.time);

    if (msg.id == ID_IMU && payload_size >= 36U) {
        float values[9];
        memcpy(values, msg.data, sizeof(values));
        for (size_t i = 0; i < ARRAY_SIZE(values); i++) {
            csv_append(line, sizeof(line), &pos, ",");
            csv_append_float(line, sizeof(line), &pos, values[i]);
        }
        csv_append(line, sizeof(line), &pos, ",%u", (unsigned int)payload_size);
    } else if (msg.id == ID_THERMAL && payload_size >= 2U) {
        const uint8_t chunk = msg.data[0];
        const uint8_t count = MIN(msg.data[1], (uint8_t)16U);
        csv_append(line, sizeof(line), &pos, ",%u,%u",
                   (unsigned int)chunk, (unsigned int)count);
        for (uint8_t i = 0; i < 16U; i++) {
            csv_append(line, sizeof(line), &pos, ",");
            if ((i < count) && ((2U + i * 2U + 1U) < payload_size)) {
                int16_t pixel;
                memcpy(&pixel, &msg.data[2U + i * 2U], sizeof(pixel));
                csv_append(line, sizeof(line), &pos, "%d", (int)pixel);
            }
        }
        csv_append(line, sizeof(line), &pos, ",%u", (unsigned int)payload_size);
    } else {
        csv_append(line, sizeof(line), &pos, ",%u,%u",
                   (unsigned int)msg.id, (unsigned int)payload_size);
    }

    if (msg.id != ID_MICRO) {
        for (uint8_t i = 0; i < SENSOR_DATA_FIXED_LENGTH; i++) {
            csv_append(line, sizeof(line), &pos, ",");
            if (i < payload_size) {
                csv_append(line, sizeof(line), &pos, "%u", (unsigned int)msg.data[i]);
            }
        }
    }
    csv_append(line, sizeof(line), &pos, "\n");

    k_mutex_lock(&file_mutex, K_FOREVER);
    ret = write_text_locked(&csv_files[file_idx].file, line, pos);
    k_mutex_unlock(&file_mutex);

    return ret;
}

int SDLogger::write_audio_block(uint64_t timestamp_us,
                                const void *interleaved_pcm,
                                size_t byte_count,
                                uint8_t channel_mask,
                                uint32_t sample_rate_hz) {
    if (!is_open || interleaved_pcm == nullptr || byte_count == 0U ||
        atomic_get(&g_stop_writing) || atomic_get(&g_sd_removed)) {
        return -ENODEV;
    }

    if ((channel_mask & 0x03U) == 0U) {
        channel_mask = 0x03U;
    }

    int ret = ensure_csv_file(ID_MICRO, nullptr);
    if (ret) {
        return ret;
    }

    if ((channel_mask & 0x01U) != 0U) {
        ret = ensure_audio_file(true);
        if (ret) {
            return ret;
        }
    }
    if ((channel_mask & 0x02U) != 0U) {
        ret = ensure_audio_file(false);
        if (ret) {
            return ret;
        }
    }

    const int16_t *samples = static_cast<const int16_t *>(interleaved_pcm);
    const size_t frame_count = byte_count / (2U * sizeof(int16_t));
    int16_t mono_chunk[64];

    k_mutex_lock(&file_mutex, K_FOREVER);

    for (uint8_t channel = 0; channel < 2U; channel++) {
        const bool write_channel = (channel == 0U) ?
            ((channel_mask & 0x01U) != 0U) : ((channel_mask & 0x02U) != 0U);
        if (!write_channel) {
            continue;
        }

        AudioFile &entry = (channel == 0U) ? audio_left_file : audio_right_file;
        size_t offset = 0;
        while (offset < frame_count) {
            const size_t chunk_frames = MIN((size_t)ARRAY_SIZE(mono_chunk),
                                            frame_count - offset);
            for (size_t i = 0; i < chunk_frames; i++) {
                mono_chunk[i] = samples[(offset + i) * 2U + channel];
            }

            ret = write_text_locked(&entry.file,
                                    reinterpret_cast<const char *>(mono_chunk),
                                    chunk_frames * sizeof(int16_t));
            if (ret) {
                k_mutex_unlock(&file_mutex);
                return ret;
            }
            offset += chunk_frames;
        }

        char line[128];
        size_t pos = 0;
        csv_append(line, sizeof(line), &pos, "%llu,%c,%u,%u\n",
                   (unsigned long long)timestamp_us,
                   channel == 0U ? 'L' : 'R',
                   (unsigned int)(frame_count * sizeof(int16_t)),
                   (unsigned int)sample_rate_hz);
        ret = write_text_locked(&csv_files[SD_LOGGER_CSV_FILE_MICRO_INDEX].file,
                                line, pos);
        if (ret) {
            k_mutex_unlock(&file_mutex);
            return ret;
        }
    }

    k_mutex_unlock(&file_mutex);
    return 0;
}

int SDLogger::write_sensor_data(const void* const* data_blocks, const size_t* lengths, size_t block_count) {
    if (!is_open || data_blocks == nullptr || lengths == nullptr || block_count == 0) {
        return -ENODEV;
    }

    // Calculate total length needed
    size_t total_length = 0;
    for (size_t i = 0; i < block_count; i++) {
        total_length += lengths[i];
    }

    // Single message larger than buffer -> cannot ever fit
    if (total_length > BUFFER_SIZE) {
        LOG_WRN("Dropping oversize record: %zu > BUFFER_SIZE=%u", total_length, (unsigned)BUFFER_SIZE);
        return -EMSGSIZE;
    }

    // If a close/flush is in progress or SD was removed, drop quickly
    if (atomic_get(&g_stop_writing) || atomic_get(&g_sd_removed)) {
        return -ENODEV;
    }

    // Do not block producers; if mutex is contended, drop quickly
    if (k_mutex_lock(&ring_mutex, K_NO_WAIT) != 0) {
        return -EAGAIN;
    }

    // Ensure there is enough space; if not, free up room by discarding oldest bytes
    // in SD_BLOCK_SIZE chunks to keep SD writer alignment and minimize partial writes.
    uint32_t space = ring_buf_space_get(&ring_buffer);
    if (space < total_length) {
        LOG_ERR("Ring buffer low on space: have %u, need %zu. Skipping data",
            space, total_length);
        k_mutex_unlock(&ring_mutex);
        return -ENOSPC;
    }

    // Try to write all blocks
    for (size_t i = 0; i < block_count; ++i) {
        const uint8_t* src = (const uint8_t*)data_blocks[i];
        size_t len = lengths[i];
        while (len > 0) {
            int wrote = ring_buf_put(&ring_buffer, src, len);
            if (wrote <= 0) {
                // Buffer still tight -> give up quickly; do not block the producer
                k_mutex_unlock(&ring_mutex);
                LOG_DBG("Ring buffer tight; partial enqueue. Dropping remainder=%zu", len);
                return -ENOSPC;
            }
            src += wrote;
            len -= wrote;
        }
    }

    k_mutex_unlock(&ring_mutex);

    k_poll_signal_raise(&logger_sig, 0);
    return 0;
}

int SDLogger::write_sensor_data(const sensor_data& msg) {
    int csv_ret = write_sensor_csv(msg);
    if (csv_ret < 0) {
        LOG_DBG("CSV sidecar write skipped/failed for sensor %u: %d",
                (unsigned int)msg.id, csv_ret);
    }

    const size_t data_size = sizeof(sensor_data) - sizeof(msg.data) + msg.size;
    const void* msg_ptr = &msg;
    return write_sensor_data(&msg_ptr, &data_size, 1);
}

int SDLogger::flush() {
    // Prevent SD thread from writing concurrently
    atomic_set(&g_stop_writing, 1);
    k_poll_signal_raise(&logger_sig, 0);

    uint32_t total_written = 0;
    for (;;) {
        uint8_t *data = nullptr;
        uint32_t fill;

        k_mutex_lock(&ring_mutex, K_FOREVER);
        fill = ring_buf_size_get(&ring_buffer);
        if (fill == 0) {
            k_mutex_unlock(&ring_mutex);
            break;
        }

        uint32_t claimed = ring_buf_get_claim(&ring_buffer, &data, fill);
        k_mutex_unlock(&ring_mutex);

        if (claimed == 0 || data == nullptr) {
            break;
        }

        size_t req = claimed;
        int written;
        k_mutex_lock(&file_mutex, K_FOREVER);
        written = sd_card->write((char*)data, &req, false);
        k_mutex_unlock(&file_mutex);

        if (written < 0) {
            state_indicator.set_sd_state(SD_FAULT);
            LOG_ERR("Failed to flush SD buffer: %d", written);
            break;
        }

        k_mutex_lock(&ring_mutex, K_FOREVER);
        ring_buf_get_finish(&ring_buffer, (uint32_t)written);
        k_mutex_unlock(&ring_mutex);

        total_written += (uint32_t)written;

        if ((uint32_t)written < claimed) {
            k_yield();
        }
    }

    return (int)total_written;
}

int SDLogger::close_auxiliary_files() {
    int first_error = 0;

    k_mutex_lock(&file_mutex, K_FOREVER);

    for (size_t i = 0; i < SD_LOGGER_CSV_FILE_COUNT; i++) {
        if (!csv_files[i].is_open) {
            continue;
        }

        int ret = fs_sync(&csv_files[i].file);
        if (ret && first_error == 0) {
            first_error = ret;
        }

        ret = fs_close(&csv_files[i].file);
        if (ret && first_error == 0) {
            first_error = ret;
        }
        csv_files[i].is_open = false;
    }

    AudioFile *audio_files[] = { &audio_left_file, &audio_right_file };
    for (AudioFile *entry : audio_files) {
        if (!entry->is_open) {
            continue;
        }

        int ret = fs_sync(&entry->file);
        if (ret && first_error == 0) {
            first_error = ret;
        }

        ret = fs_close(&entry->file);
        if (ret && first_error == 0) {
            first_error = ret;
        }
        entry->is_open = false;
    }

    k_mutex_unlock(&file_mutex);
    return first_error;
}

int SDLogger::end() {
    int ret;
    
    if (!is_open) {
        return -ENODEV;
    }

    if (!sd_card->is_mounted()) {
        //k_poll_signal_reset(&logger_sig);
        is_open = false;
        return -ENODEV;
    }

    // Prevent SD thread/producers from writing while we flush/close
    atomic_set(&g_stop_writing, 1);
    k_poll_signal_raise(&logger_sig, 0);

    ret = flush();
    if (ret < 0) {
        LOG_ERR("Failed to flush file buffer.");
        return ret;
    }

    LOG_INF("Close File ....");

    LOG_DBG("Max buffer fill: %d bytes", count_max_buffer_fill);

    k_mutex_lock(&file_mutex, K_FOREVER);
    ret = sd_card->close_file();
    k_mutex_unlock(&file_mutex);
    if (ret < 0) {
        k_poll_signal_reset(&logger_sig);
        return ret;
    }

    ret = close_auxiliary_files();
    if (ret < 0) {
        LOG_WRN("Failed to close auxiliary SD log files: %d", ret);
    }

    is_open = false;

    k_poll_signal_reset(&logger_sig);
    atomic_clear(&g_stop_writing);
    atomic_clear(&g_sd_removed);

    return 0;
}

bool SDLogger::is_active() {
    return is_open;
}

SDLogger sdlogger;
