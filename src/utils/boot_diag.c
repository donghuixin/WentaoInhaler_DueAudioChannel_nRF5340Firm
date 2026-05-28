#include "boot_diag.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>

/* Two-stage log:
 *   - `boot_buf` captures the very first events after reset (PMIC safe
 *     init, sensor power up, etc.) and is never overwritten once filled.
 *     This guarantees the web UI can always show how the device booted.
 *   - `recent_buf` is a rolling ring for runtime events (bus scans, sensor
 *     bring-up retries, etc.). Old entries get dropped when the ring wraps.
 */
#define BOOT_DIAG_BOOT_CAP    32
#define BOOT_DIAG_RECENT_CAP  24
/* Bumped from 88 -> 144 (2026-05-27) so PCA9306 VERDICT lines and other
 * detailed hardware diagnostics don't get truncated mid-sentence. The static
 * footprint grows from (32+24)*88 = 4928 B to (32+24)*144 = 8064 B which is
 * still well within the device-info hw_status buffer (8192 B). */
#define BOOT_DIAG_MAX_MSG_LEN 144

struct boot_diag_entry {
	uint32_t boot_ms;
	char msg[BOOT_DIAG_MAX_MSG_LEN];
};

static struct boot_diag_entry boot_buf[BOOT_DIAG_BOOT_CAP];
static struct boot_diag_entry recent_buf[BOOT_DIAG_RECENT_CAP];

static uint16_t boot_count;
static uint16_t recent_head;       /* next write position in recent ring */
static uint16_t recent_count;      /* current valid entries (<= CAP)     */
static uint32_t dropped_boot;      /* boot entries we had to drop        */
static uint32_t dropped_recent;    /* recent entries we had to drop      */

static K_MUTEX_DEFINE(diag_mutex);

static void escape_json(const char *in, char *out, size_t out_cap)
{
	if (out_cap == 0) {
		return;
	}

	size_t o = 0;
	const size_t last = out_cap - 1;
	for (size_t i = 0; in[i] != '\0' && o < last; i++) {
		char c = in[i];
		if (c == '"' || c == '\\') {
			if (o + 1 >= last) {
				break;
			}
			out[o++] = '\\';
			out[o++] = c;
		} else if ((unsigned char)c < 0x20) {
			/* Drop control bytes; they only confuse JSON parsers. */
			continue;
		} else {
			out[o++] = c;
		}
	}
	out[o] = '\0';
}

void boot_diag_log(const char *fmt, ...)
{
	if (fmt == NULL) {
		return;
	}

	char tmp[BOOT_DIAG_MAX_MSG_LEN];
	va_list ap;
	va_start(ap, fmt);
	int written = vsnprintf(tmp, sizeof(tmp), fmt, ap);
	va_end(ap);
	if (written < 0) {
		return;
	}

	const uint32_t now_ms = (uint32_t)k_uptime_get_32();

	k_mutex_lock(&diag_mutex, K_FOREVER);

	if (boot_count < BOOT_DIAG_BOOT_CAP) {
		struct boot_diag_entry *slot = &boot_buf[boot_count++];
		slot->boot_ms = now_ms;
		escape_json(tmp, slot->msg, sizeof(slot->msg));
		k_mutex_unlock(&diag_mutex);
		return;
	}

	struct boot_diag_entry *slot = &recent_buf[recent_head];
	slot->boot_ms = now_ms;
	escape_json(tmp, slot->msg, sizeof(slot->msg));

	recent_head = (uint16_t)((recent_head + 1U) % BOOT_DIAG_RECENT_CAP);
	if (recent_count < BOOT_DIAG_RECENT_CAP) {
		recent_count++;
	} else {
		dropped_recent++;
	}

	k_mutex_unlock(&diag_mutex);
}

static size_t append(char *out, size_t cap, size_t pos, const char *fmt, ...)
{
	if (pos >= cap) {
		return cap == 0 ? 0 : cap - 1;
	}

	va_list ap;
	va_start(ap, fmt);
	int written = vsnprintf(&out[pos], cap - pos, fmt, ap);
	va_end(ap);
	if (written < 0) {
		return pos;
	}

	size_t next = pos + (size_t)written;
	if (next >= cap) {
		return cap - 1;
	}
	return next;
}

static size_t emit_array(char *out, size_t cap, size_t pos,
			 const char *key,
			 const struct boot_diag_entry *base,
			 uint16_t start, uint16_t count, uint16_t mod)
{
	pos = append(out, cap, pos, "\"%s\":[", key);
	for (uint16_t i = 0; i < count; i++) {
		uint16_t idx = (uint16_t)((start + i) % (mod > 0 ? mod : 1));
		const struct boot_diag_entry *e = &base[idx];
		pos = append(out, cap, pos, "%s{\"t\":%u,\"m\":\"%s\"}",
			     i == 0 ? "" : ",",
			     (unsigned int)e->boot_ms, e->msg);
	}
	pos = append(out, cap, pos, "]");
	return pos;
}

size_t boot_diag_emit_json(char *out, size_t cap, size_t pos)
{
	if (out == NULL || cap == 0) {
		return pos;
	}

	k_mutex_lock(&diag_mutex, K_FOREVER);

	pos = append(out, cap, pos,
		     "\"dropped_boot\":%u,\"dropped_recent\":%u,",
		     dropped_boot, dropped_recent);
	pos = emit_array(out, cap, pos, "boot", boot_buf, 0,
			 boot_count, BOOT_DIAG_BOOT_CAP);
	pos = append(out, cap, pos, ",");

	uint16_t start = (recent_count < BOOT_DIAG_RECENT_CAP)
				? 0U
				: recent_head;
	pos = emit_array(out, cap, pos, "recent", recent_buf, start,
			 recent_count, BOOT_DIAG_RECENT_CAP);

	k_mutex_unlock(&diag_mutex);
	return pos;
}

void boot_diag_reset(void)
{
	k_mutex_lock(&diag_mutex, K_FOREVER);
	memset(boot_buf, 0, sizeof(boot_buf));
	memset(recent_buf, 0, sizeof(recent_buf));
	boot_count = 0;
	recent_head = 0;
	recent_count = 0;
	dropped_boot = 0;
	dropped_recent = 0;
	k_mutex_unlock(&diag_mutex);
}
