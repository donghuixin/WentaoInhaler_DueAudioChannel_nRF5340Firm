#include "sdlogger_wrapper.h"
#include "openearable_common.h"
#include "SDLogger.h"

// Assuming sdlogger is an accessible global object
extern SDLogger sdlogger;

extern "C" {
    int sdlogger_write_data(const void* const* data_blocks, const size_t* lengths, size_t block_count) {
        return sdlogger.write_sensor_data(data_blocks, lengths, block_count);
    }

    int sdlogger_write_audio_block(uint64_t timestamp_us,
                                   const void *interleaved_pcm,
                                   size_t byte_count,
                                   uint8_t channel_mask,
                                   uint32_t sample_rate_hz) {
        return sdlogger.write_audio_block(timestamp_us, interleaved_pcm, byte_count,
                                          channel_mask, sample_rate_hz);
    }
}
