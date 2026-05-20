/*
 * MLX90642 32x24 thermal array driver for OpenEarable.
 *
 * Ported from the Arduino MLX90642 v1.0.3 library by D. Dubins (Feb-Mar 2026)
 * to fit the Zephyr / TWIM driver structure used by the rest of this project
 * (see MLX90632.cpp for the reference style).
 *
 * The sensor is wired to I2C1 on OpenEarable v2 (see DTS node `mlx90642`).
 * 7-bit I2C address is taken from the DTS node so it can be changed
 * without touching firmware code.
 */

#pragma once

#include <TWIM.h>

/* MLX90642 register / memory layout */
#define MLX90642_NUM_PIXELS        768U      /* 32 columns x 24 rows */
#define MLX90642_NUM_COLS          32U
#define MLX90642_NUM_ROWS          24U

/* RAM addresses */
#define MLX90642_FRAME_ADDR        0x342C   /* Start of pixel temperature data */
#define MLX90642_TA_ADDR           0x3A2C   /* Sensor / ambient temperature */
#define MLX90642_PROGRESS_ADDR     0x3C10   /* Conversion progress (used to detect new frame) */

/* EEPROM addresses */
#define MLX90642_CONFIG_ADDR       0x11F0   /* bits 0:2 hold refresh rate code */

/* Refresh rate codes (bits 0:2 of 0x11F0) */
#define MLX90642_REFRESH_2HZ       2
#define MLX90642_REFRESH_4HZ       3
#define MLX90642_REFRESH_8HZ       4
#define MLX90642_REFRESH_16HZ      5

/* EEPROM write opcode */
#define MLX90642_EEPROM_WRITE_OP   0x3A
#define MLX90642_EEPROM_WRITE_SUB  0x2E

/* Pixel temperature is stored as int16, value = temperature_celsius * 50.
 * raw / 50.0f -> deg C (per the third-party driver). The web UI also uses /50.
 */
#define MLX90642_TEMP_SCALE        50.0f
#define MLX90642_TA_SCALE          100.0f

class MLX90642 {
public:
	typedef enum {
		SENSOR_SUCCESS,
		SENSOR_ID_ERROR,
		SENSOR_I2C_ERROR,
		SENSOR_INTERNAL_ERROR,
		SENSOR_GENERIC_ERROR,
		SENSOR_TIMEOUT_ERROR,
	} status;

	/* By default the driver picks up the address and the bus from DTS
	 * (`mlx90642` node). Override for tests / alternate wiring.
	 */
	bool begin();
	bool begin(uint8_t deviceAddress, TWIM &i2c, status &returnError);

	/* True when the sensor has finished a fresh frame since last check.
	 * Internally tracks the conversion progress register.
	 */
	bool isNewDataAvailable();

	/* Single 16-bit register access. addr is the MLX90642 memory address. */
	status readAddr_unsigned(uint16_t addr, uint16_t &out);
	status readAddr_signed(uint16_t addr, int16_t &out);

	/* Block read `count` consecutive 16-bit registers starting at `addr`.
	 * `out` must point to a buffer of at least `count` int16_t elements.
	 * The MLX9064x family supports auto-incrementing reads, so this is
	 * the fast path used to grab a full thermal frame.
	 */
	status readRange(uint16_t addr, int16_t *out, uint16_t count);

	/* Convenience: read all 768 pixels in raw int16 form (value / 50 = degC).
	 * Note: pass a buffer large enough to hold MLX90642_NUM_PIXELS int16s.
	 * (1536 bytes; allocate as static/heap if calling from a small stack.)
	 */
	status readPixelsRaw(int16_t *raw_pixels);

	/* Sensor body temperature in degC (typically 8-10 degC above ambient). */
	float readTa();

	/* Write a 16-bit EEPROM word. Returns true on success. */
	bool writeEEPROM(uint16_t eepromAddr, uint16_t newValue);

	/* Pick refresh rate code (see MLX90642_REFRESH_* macros).
	 * EEPROM is only written when the new code differs from the existing one
	 * to avoid unnecessary wear.
	 */
	bool setRefreshRate(uint8_t rate_code);
	uint8_t getRefreshRateCode();

	uint16_t pix_addr(uint16_t pxl) {
		return MLX90642_FRAME_ADDR + (pxl * 2U);
	}

private:
	TWIM *_i2c = nullptr;
	uint8_t _deviceAddress = 0;
	uint16_t _last_progress = 0xFFFF;
};
