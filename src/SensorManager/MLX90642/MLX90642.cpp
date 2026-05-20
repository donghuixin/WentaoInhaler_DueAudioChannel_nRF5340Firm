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

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

#include <string.h>

LOG_MODULE_REGISTER(MLX90642, 3);

/* DTS reference. The node has to exist; failing the build here is preferable
 * to a confusing runtime probe failure if the user forgets to add it.
 */
#define MLX90642_NODE DT_NODELABEL(mlx90642)
#if !DT_NODE_EXISTS(MLX90642_NODE)
#error "DTS node `mlx90642` is missing. Add it to the OpenEarable v2 board overlay."
#endif

/* I2C bus that hosts the MLX90642. Hard-wired to I2C1 because that is what the
 * OpenEarable v2 board uses (see DTS overlay).
 */
#define MLX90642_I2C_BUS I2C1

/* Maximum number of bytes per Zephyr I2C transaction. Keeps the block-read
 * comfortably under `zephyr,concat-buf-size`/`zephyr,flash-buf-max-size`
 * (currently 512) so the TWIM driver can serve it from a single buffer.
 */
#define MLX90642_RANGE_CHUNK_BYTES 256U

bool MLX90642::begin()
{
	status err = SENSOR_SUCCESS;
	uint8_t addr = (uint8_t)DT_REG_ADDR(MLX90642_NODE);
	return begin(addr, MLX90642_I2C_BUS, err);
}

bool MLX90642::begin(uint8_t deviceAddress, TWIM &i2c, status &returnError)
{
	returnError = SENSOR_SUCCESS;
	_deviceAddress = deviceAddress;
	_i2c = &i2c;
	_last_progress = 0xFFFF;

	_i2c->begin();

	/* Probe: a healthy device will return a plausible Ta reading.
	 * The probe register is a status word, so reading it should always
	 * succeed once the bus + 1V8 supply are up.
	 */
	uint16_t probe = 0;
	status ret = readAddr_unsigned(MLX90642_PROGRESS_ADDR, probe);
	if (ret != SENSOR_SUCCESS || probe == 0xFFFF) {
		LOG_WRN("MLX90642 probe failed at 0x%02X (ret=%d, raw=0x%04X)",
			_deviceAddress, (int)ret, probe);
		returnError = SENSOR_ID_ERROR;
		return false;
	}

	int16_t ta_raw = 0;
	(void)readAddr_signed(MLX90642_TA_ADDR, ta_raw);
	LOG_INF("MLX90642 online at 0x%02X, Ta=%.2f degC, refresh code=%u",
		_deviceAddress, (double)ta_raw / (double)MLX90642_TA_SCALE,
		getRefreshRateCode());
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
