/*
 * MLX90642 32x24 thermal array driver implementation.
 *
 * Ported to Zephyr / TWIM from the public-domain Arduino library
 * (D. Dubins, MLX90642 v1.0.3). The single-register protocol matches
 * the third-party driver one-for-one, but it adds a block-read fast path
 * (`readRange`) so a full 1536-byte frame can be fetched in a few
 * transactions rather than 768 individual ones.
 */

#include "MLX90642.h"

#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

#include <errno.h>
#include <string.h>

LOG_MODULE_REGISTER(MLX90642, 3);

/* DTS reference. The node has to exist; failing the build here is preferable
 * to a confusing runtime probe failure if the user forgets to add it.
 */
#define MLX90642_NODE DT_NODELABEL(mlx90642)
#if !DT_NODE_EXISTS(MLX90642_NODE)
#error "DTS node `mlx90642` is missing. Add it to the OpenEarable v2 board overlay."
#endif

/* Pick TWIM bus from the DTS parent of `mlx90642` so wiring changes only need
 * DTS updates and cannot silently drift from driver configuration.
 */
#if DT_SAME_NODE(DT_BUS(MLX90642_NODE), DT_NODELABEL(i2c1))
#define MLX90642_I2C_BUS I2C1
#define MLX90642_I2C_NAME "IIC0(&i2c1)"
#elif DT_SAME_NODE(DT_BUS(MLX90642_NODE), DT_NODELABEL(i2c2))
#define MLX90642_I2C_BUS I2C2
#define MLX90642_I2C_NAME "IIC1(&i2c2)"
#elif DT_SAME_NODE(DT_BUS(MLX90642_NODE), DT_NODELABEL(i2c3))
#define MLX90642_I2C_BUS I2C3
#define MLX90642_I2C_NAME "IIC2(&i2c3)"
#else
#error "mlx90642 must be placed on &i2c1, &i2c2, or &i2c3"
#endif

/* Maximum number of bytes per Zephyr I2C transaction. Keeps the block-read
 * comfortably under `zephyr,concat-buf-size`/`zephyr,flash-buf-max-size`
 * (currently 512) so the TWIM driver can serve it from a single buffer.
 */
#define MLX90642_RANGE_CHUNK_BYTES 256U

/* Lightweight zero-length write to check if a 7-bit address ACKs.
 * Used to distinguish "device not on bus" from "EEPROM still busy" so the
 * log message tells the operator which way to debug (wiring vs timing).
 */
int MLX90642::probeAck(uint8_t addr)
{
	if (_i2c == nullptr) {
		return -ENODEV;
	}
	uint8_t dummy = 0;
	_i2c->aquire();
	int ret = i2c_write(_i2c->master, &dummy, 0, addr);
	_i2c->release();
	return ret;
}

bool MLX90642::begin()
{
	status err = SENSOR_SUCCESS;
	uint8_t addr = (uint8_t)DT_REG_ADDR(MLX90642_NODE);
	LOG_INF("MLX90642 DTS bus=%s addr=0x%02X (datasheet default 0x66, fallback 0x33)",
		MLX90642_I2C_NAME, addr);
	return begin(addr, MLX90642_I2C_BUS, err);
}

bool MLX90642::begin(uint8_t deviceAddress, TWIM &i2c, status &returnError)
{
	returnError = SENSOR_SUCCESS;
	_deviceAddress = deviceAddress;
	_i2c = &i2c;
	_last_progress = 0xFFFF;

	_i2c->begin();
	if (!device_is_ready(_i2c->master)) {
		LOG_ERR("MLX90642 I2C controller %s is NOT ready - check DTS / pinctrl",
			MLX90642_I2C_NAME);
		returnError = SENSOR_I2C_ERROR;
		return false;
	}

	/* Datasheet 3.2.2: first valid data after POR needs at most
	 * 10ms (init) + 70ms (max) + RT (up to 500ms @ 2Hz). The supply
	 * itself may have been off until ls_3_3 was switched on a moment
	 * ago, so we wait long enough for the EEPROM->RAM copy to settle
	 * before bothering the device with the first command.
	 */
	LOG_INF("MLX90642 waiting %u ms for POR / EEPROM bootstrap...",
		MLX90642_POR_DELAY_MS);
	k_msleep(MLX90642_POR_DELAY_MS);

	/* Step 1: simple I2C ACK probe at the configured address. This isolates
	 * a wiring/power problem from a register-read failure.
	 */
	int ack = -EIO;
	for (uint32_t i = 0; i < MLX90642_PROBE_RETRIES; i++) {
		ack = probeAck(_deviceAddress);
		if (ack == 0) {
			break;
		}
		LOG_WRN("MLX90642 NACK on 0x%02X (try %u/%u, errno=%d)",
			_deviceAddress, (unsigned int)(i + 1U),
			(unsigned int)MLX90642_PROBE_RETRIES, ack);
		k_msleep(MLX90642_PROBE_RETRY_MS);
	}

	if (ack != 0) {
		/* Per datasheet NOTE 1 (table 5 / section 3.1.5.4): if the
		 * EEPROM slave-address byte is 0x00 the device responds on
		 * 0x33 instead. Try it once before giving up so the operator
		 * doesn't have to guess.
		 */
		int ack_fb = probeAck(MLX90642_FALLBACK_ADDR);
		if (ack_fb == 0) {
			LOG_WRN("MLX90642 not at 0x%02X but ACKs on fallback 0x%02X "
				"(EEPROM SA byte == 0x00). Switching driver to 0x%02X.",
				_deviceAddress, MLX90642_FALLBACK_ADDR,
				MLX90642_FALLBACK_ADDR);
			_deviceAddress = MLX90642_FALLBACK_ADDR;
		} else {
			LOG_ERR("MLX90642 no ACK on 0x%02X or 0x%02X (errno=%d/%d). "
				"Check IIC1 wiring (SCL=P1.00, SDA=P1.15), pull-ups, "
				"VDD=3.0..3.6V on pin2, and that ls_3_3 is enabled.",
				_deviceAddress, MLX90642_FALLBACK_ADDR, ack, ack_fb);
			returnError = SENSOR_ID_ERROR;
			return false;
		}
	}

	/* Step 2: read the Progress bar with retries. 0x3C10 is a small RAM
	 * field that should never read back 0xFFFF; if it does, we either
	 * have noise or the EEPROM->RAM copy isn't done yet.
	 */
	uint16_t probe = 0;
	status ret = SENSOR_I2C_ERROR;
	for (uint32_t i = 0; i < MLX90642_PROBE_RETRIES; i++) {
		ret = readAddr_unsigned(MLX90642_PROGRESS_ADDR, probe);
		if (ret == SENSOR_SUCCESS && probe != 0xFFFF) {
			break;
		}
		LOG_WRN("MLX90642 progress read try %u/%u: ret=%d raw=0x%04X",
			(unsigned int)(i + 1U),
			(unsigned int)MLX90642_PROBE_RETRIES, (int)ret, probe);
		k_msleep(MLX90642_PROBE_RETRY_MS);
	}

	if (ret != SENSOR_SUCCESS || probe == 0xFFFF) {
		LOG_ERR("MLX90642 ACKs on 0x%02X but progress read keeps failing "
			"(ret=%d, raw=0x%04X). Possible causes: long wires/noise, "
			"missing/weak pull-ups, FM+ disabled in EEPROM, or a real "
			"defective device.",
			_deviceAddress, (int)ret, probe);
		returnError = SENSOR_ID_ERROR;
		return false;
	}

	/* Step 3: read FW version + part of device ID for traceability. */
	uint16_t fw_lo = 0, fw_hi = 0;
	uint16_t id0 = 0, id1 = 0;
	uint16_t sa_eep = 0;
	(void)readAddr_unsigned(MLX90642_FW_VERSION_LO, fw_lo);
	(void)readAddr_unsigned(MLX90642_FW_VERSION_HI, fw_hi);
	(void)readAddr_unsigned(MLX90642_DEVICE_ID_0, id0);
	(void)readAddr_unsigned(MLX90642_DEVICE_ID_1, id1);
	(void)readAddr_unsigned(MLX90642_SA_ADDR, sa_eep);

	/* Step 3.5: verify 0x11FC analog config — critical for 1.8V I2C.
	 *
	 * The nRF5340 GPIO/TWIM runs at 1.8V. If bit 2 of 0x11FC is NOT set
	 * the MLX90642 uses VDD-referenced thresholds: VIH = 0.7 × 3.3V =
	 * 2.31V, which 1.8V cannot reach → the sensor will NACK all traffic.
	 *
	 * If we can read this register, it means the sensor was already
	 * configured for 1.8V mode (otherwise we would have failed at the
	 * ACK probe above). Log the value for diagnostics anyway.
	 */
	uint16_t analog_cfg = 0;
	if (readAddr_unsigned(MLX90642_ANALOG_CONFIG_ADDR, analog_cfg) == SENSOR_SUCCESS) {
		LOG_INF("MLX90642 EEPROM 0x11FC = 0x%04X (bit2=%u -> %s I2C threshold)",
			analog_cfg, (unsigned int)((analog_cfg >> 2) & 1U),
			(analog_cfg & 0x0004) ? "1.8V" : "VDD-ref");
		if (!(analog_cfg & 0x0004)) {
			LOG_WRN("MLX90642 0x11FC bit2=0: VDD-referenced threshold active. "
				"1.8V I2C (nRF5340) will be unreliable! "
				"Use a 3.3V I2C master to set bit 2 first.");
		}
	} else {
		LOG_WRN("MLX90642 could not read 0x11FC (analog config)");
	}

	int16_t ta_raw = 0;
	(void)readAddr_signed(MLX90642_TA_ADDR, ta_raw);

	LOG_INF("MLX90642 online at 0x%02X, Ta=%.2f degC, progress=%u%%, refresh=%u",
		_deviceAddress, (double)ta_raw / (double)MLX90642_TA_SCALE,
		(unsigned int)probe, getRefreshRateCode());
	LOG_INF("MLX90642 FW=%u.%u.%u, devID[0..1]=0x%04X%04X, EEPROM SA=0x%02X",
		(unsigned int)(fw_lo >> 8) & 0xFFU,
		(unsigned int)(fw_hi & 0xFFU),
		(unsigned int)((fw_hi >> 8) & 0xFFU),
		id0, id1, (unsigned int)(sa_eep & 0x7FU));
	return true;
}

bool MLX90642::isNewDataAvailable()
{
	uint16_t progress = 0;
	if (readAddr_unsigned(MLX90642_PROGRESS_ADDR, progress) != SENSOR_SUCCESS) {
		return false;
	}

	bool new_frame = (progress < _last_progress);
	_last_progress = progress;
	return new_frame;
}

MLX90642::status MLX90642::readAddr_unsigned(uint16_t addr, uint16_t &out)
{
	if (_i2c == nullptr) {
		out = 0xFFFF;
		return SENSOR_GENERIC_ERROR;
	}

	uint8_t addr_buf[2] = {
		(uint8_t)(addr >> 8),
		(uint8_t)(addr & 0xFF),
	};
	uint8_t buffer[2] = { 0, 0 };

	_i2c->aquire();
	int ret = i2c_write_read(_i2c->master, _deviceAddress, addr_buf, sizeof(addr_buf),
				 buffer, sizeof(buffer));
	_i2c->release();

	if (ret) {
		LOG_WRN("MLX90642 read16 addr=0x%04X failed: %d", addr, ret);
		out = 0xFFFF;
		return SENSOR_I2C_ERROR;
	}

	out = ((uint16_t)buffer[0] << 8) | buffer[1];
	return SENSOR_SUCCESS;
}

MLX90642::status MLX90642::readAddr_signed(uint16_t addr, int16_t &out)
{
	uint16_t tmp = 0;
	status ret = readAddr_unsigned(addr, tmp);
	out = (int16_t)tmp;
	return ret;
}

MLX90642::status MLX90642::readRange(uint16_t addr, int16_t *out, uint16_t count)
{
	if (_i2c == nullptr || out == nullptr || count == 0) {
		return SENSOR_GENERIC_ERROR;
	}

	uint16_t remaining = count;
	uint16_t cur_addr = addr;
	int16_t *cursor = out;

	while (remaining > 0) {
		uint16_t take_words =
			(remaining * 2U > MLX90642_RANGE_CHUNK_BYTES)
				? (uint16_t)(MLX90642_RANGE_CHUNK_BYTES / 2U)
				: remaining;
		uint16_t take_bytes = take_words * 2U;

		uint8_t addr_buf[2] = {
			(uint8_t)(cur_addr >> 8),
			(uint8_t)(cur_addr & 0xFF),
		};
		uint8_t buf[MLX90642_RANGE_CHUNK_BYTES];

		_i2c->aquire();
		int ret = i2c_write_read(_i2c->master, _deviceAddress, addr_buf,
					 sizeof(addr_buf), buf, take_bytes);
		_i2c->release();

		if (ret) {
			LOG_WRN("MLX90642 readRange addr=0x%04X len=%u failed: %d",
				cur_addr, take_bytes, ret);
			return SENSOR_I2C_ERROR;
		}

		for (uint16_t i = 0; i < take_words; i++) {
			uint16_t hi = buf[i * 2];
			uint16_t lo = buf[i * 2 + 1];
			cursor[i] = (int16_t)((hi << 8) | lo);
		}

		cursor += take_words;
		cur_addr = (uint16_t)(cur_addr + take_bytes);
		remaining = (uint16_t)(remaining - take_words);
	}

	return SENSOR_SUCCESS;
}

MLX90642::status MLX90642::readPixelsRaw(int16_t *raw_pixels)
{
	if (raw_pixels == nullptr) {
		return SENSOR_GENERIC_ERROR;
	}
	return readRange(MLX90642_FRAME_ADDR, raw_pixels, MLX90642_NUM_PIXELS);
}

float MLX90642::readTa()
{
	int16_t ta_raw = 0;
	if (readAddr_signed(MLX90642_TA_ADDR, ta_raw) != SENSOR_SUCCESS) {
		return 0.0f;
	}
	return (float)ta_raw / MLX90642_TA_SCALE;
}

bool MLX90642::writeEEPROM(uint16_t eepromAddr, uint16_t newValue)
{
	if (_i2c == nullptr) {
		return false;
	}

	/* MLX90642 EEPROM configuration command:
	 *  0x3A | 0x2E | addrMSB | addrLSB | dataMSB | dataLSB
	 */
	uint8_t payload[6] = {
		MLX90642_EEPROM_WRITE_OP,
		MLX90642_EEPROM_WRITE_SUB,
		(uint8_t)(eepromAddr >> 8),
		(uint8_t)(eepromAddr & 0xFF),
		(uint8_t)(newValue >> 8),
		(uint8_t)(newValue & 0xFF),
	};

	_i2c->aquire();
	int ret = i2c_write(_i2c->master, payload, sizeof(payload), _deviceAddress);
	_i2c->release();

	if (ret) {
		LOG_WRN("MLX90642 EEPROM write addr=0x%04X failed: %d", eepromAddr, ret);
		return false;
	}

	/* Per datasheet behaviour: allow the internal write cycle to finish. */
	k_msleep(10);
	return true;
}

uint8_t MLX90642::getRefreshRateCode()
{
	uint16_t ctrl = 0;
	if (readAddr_unsigned(MLX90642_CONFIG_ADDR, ctrl) != SENSOR_SUCCESS) {
		return 0xFF;
	}
	return (uint8_t)(ctrl & 0x07);
}

bool MLX90642::setRefreshRate(uint8_t rate_code)
{
	if (rate_code < MLX90642_REFRESH_2HZ || rate_code > MLX90642_REFRESH_16HZ) {
		LOG_WRN("MLX90642 invalid refresh rate code %u", rate_code);
		return false;
	}

	uint16_t ctrl = 0;
	if (readAddr_unsigned(MLX90642_CONFIG_ADDR, ctrl) != SENSOR_SUCCESS) {
		return false;
	}

	/* Skip the write (and the EEPROM wear) when the rate is already set. */
	if ((ctrl & 0x07) == rate_code) {
		return true;
	}

	uint16_t updated = (uint16_t)((ctrl & ~0x07U) | rate_code);
	if (!writeEEPROM(MLX90642_CONFIG_ADDR, updated)) {
		return false;
	}

	/* Verify. */
	uint16_t verify = 0;
	k_msleep(20);
	if (readAddr_unsigned(MLX90642_CONFIG_ADDR, verify) != SENSOR_SUCCESS) {
		return false;
	}
	if ((verify & 0x07) != rate_code) {
		LOG_WRN("MLX90642 refresh rate verify mismatch: wrote %u got %u",
			rate_code, (unsigned int)(verify & 0x07));
		return false;
	}
	LOG_INF("MLX90642 refresh rate set to code %u", rate_code);
	return true;
}
