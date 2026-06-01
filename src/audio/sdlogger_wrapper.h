#ifndef SDLOGGER_WRAPPER_H
#define SDLOGGER_WRAPPER_H

#include "openearable_common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Wrapper function to call sdlogger.write_data from C code
 * 
 * @param data Pointer to sensor data header
 * @param size Size of the audio data
 * @return int Return code (0 for success)
 */
int sdlogger_write_data(const void* const* data_blocks, const size_t* lengths, size_t block_count);
int sdlogger_write_audio_block(uint64_t timestamp_us,
                               const void *interleaved_pcm,
                               size_t byte_count,
                               uint8_t channel_mask,
                               uint32_t sample_rate_hz);

#ifdef __cplusplus
}
#endif

#endif /* SDLOGGER_WRAPPER_H */
