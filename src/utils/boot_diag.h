/*
 * Boot / runtime diagnostic ring buffer.
 *
 * Collects short status lines (boot-time PMIC safe-init, runtime I2C bus
 * scans, etc.) into a small in-RAM ring so they can be surfaced to the web
 * UI through the Hardware Status GATT characteristic in addition to the
 * normal RTT/UART logger.
 *
 * Design choices:
 *   - Fixed size, statically allocated (no heap).
 *   - Producer-side append is mutex protected so the BLE thread that emits
 *     the JSON snapshot does not race with the boot or scan threads.
 *   - JSON emission walks the ring oldest-first; entries are timestamped
 *     in milliseconds since boot.
 *   - Newest entries always win; on overflow the oldest is dropped and a
 *     monotonic `dropped` counter is exposed in the JSON header.
 */

#ifndef BOOT_DIAG_H_
#define BOOT_DIAG_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Append a single line. Format string is printf-style. */
void boot_diag_log(const char *fmt, ...);

/* Convenience macro mirroring LOG_INF callsites; the standard logger still
 * fires from the original LOG_* macro so RTT output is unchanged.
 */
#define BOOT_DIAG(fmt, ...) boot_diag_log(fmt, ##__VA_ARGS__)

/* Append the diagnostic ring as a JSON fragment of the form:
 *   "dropped":N,"log":[{"t":12,"m":"..."}, ...]
 * The caller is responsible for placing the enclosing braces / commas.
 * Returns the new buffer position (clamped to `cap - 1`).
 */
size_t boot_diag_emit_json(char *out, size_t cap, size_t pos);

/* Mostly for unit tests. */
void boot_diag_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* BOOT_DIAG_H_ */
