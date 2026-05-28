/*
 * MLX90642 32x24 thermal array driver for OpenEarable.
 *
 * Ported from the Arduino MLX90642 v1.0.3 library by D. Dubins (Feb-Mar 2026)
 * to fit the Zephyr / TWIM driver structure used by the rest of this project
 * (see MLX90632.cpp for the reference style).
 *
 * The sensor bus is selected from DTS parent node (`mlx90642`), so project
 * IIC0/IIC1/IIC2 remapping only needs a DTS change.
 * 7-bit I2C address is taken from the DTS node so it can be changed
 * without touching firmware code.
 */

#pragma once

#include <TWIM.h>

/* MLX90642 register / memory layout */
#define MLX90642_NUM_PIXELS        768U      /* 32 columns x 24 rows */
#define MLX90642_NUM_COLS          32U
#define MLX90642_NUM_ROWS          24U

/* RAM addresses (datasheet 3.1.5.3) */
#define MLX90642_FRAME_ADDR        0x342C   /* Start of To pixel data (768 words) */
#define MLX90642_RAW_IR_ADDR       0x2E2A   /* Raw IR data, datasheet 3.1.4.1.4 #2 */
#define MLX90642_TA_ADDR           0x3A2C   /* T sensor (8..10 degC above ambient) */
#define MLX90642_PROGRESS_ADDR     0x3C10   /* Conversion progress 0..100 */

/* EEPROM addresses (datasheet table 13) */
#define MLX90642_CONFIG_ADDR       0x11F0   /* bits 0:2 hold refresh rate code */
#define MLX90642_ANALOG_CONFIG_ADDR 0x11FC  /* bit 2: I2C threshold ref (0=VDD, 1=1.8V) */
#define MLX90642_SA_ADDR           0x11FE   /* I2C device slave address (bits 0..6) */

/* FW version / device ID (datasheet 3.1.4.3 / 3.1.4.4) */
#define MLX90642_FW_VERSION_LO     0xFFF8   /* Major version in MSB */
#define MLX90642_FW_VERSION_HI     0xFFFA   /* Minor in LSB, Patch in MSB */
#define MLX90642_DEVICE_ID_0       0x1230
#define MLX90642_DEVICE_ID_1       0x1232
#define MLX90642_DEVICE_ID_2       0x1234
#define MLX90642_DEVICE_ID_3       0x1236

/* Refresh rate codes (bits 0:2 of 0x11F0) */
#define MLX90642_REFRESH_2HZ       2
#define MLX90642_REFRESH_4HZ       3
#define MLX90642_REFRESH_8HZ       4
#define MLX90642_REFRESH_16HZ      5

/* EEPROM write opcode 0x3A2E sent MSB-first per datasheet figure 12 */
#define MLX90642_EEPROM_WRITE_OP   0x3A
#define MLX90642_EEPROM_WRITE_SUB  0x2E

/* Datasheet note: if the EEPROM slave address is 0x00 the device
 * answers on SA=0x33 instead of 0x66.
 */
#define MLX90642_FALLBACK_ADDR     0x33

/* POR timing per datasheet 3.2.2 (Rev 003, 19-Mar-25):
 *   Tvalid_data = 10ms (Init) + 70ms (max) + RT, ms
 *   where RT is the Refresh Time at the configured RR setting:
 *     RR=2Hz  -> RT = 500 ms  -> Tvalid_data = 580 ms (worst case)
 *     RR=4Hz  -> RT = 250 ms  -> Tvalid_data = 330 ms
 *     RR=8Hz  -> RT = 125 ms  -> Tvalid_data = 205 ms (factory default)
 *     RR=16Hz -> RT =  62 ms  -> Tvalid_data = 142 ms
 *
 * We pick 600 ms so we cover the 2 Hz worst case with margin, regardless of
 * whatever the EEPROM was programmed to in a previous session. The penalty
 * is paid only once at boot, so the extra ~350 ms over 8 Hz default is fine.
 *
 * Note: this only delays the FIRST register read; the bare I2C ACK probe
 * (zero-length write) does not need POR completion -- the I2C interface
 * is up almost immediately after VDD crosses VPOR_UP (2.8 V).
 */
#define MLX90642_POR_DELAY_MS      600U
#define MLX90642_PROBE_RETRIES     5U
#define MLX90642_PROBE_RETRY_MS    50U

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

	/* Zero-length write to test whether the configured address ACKs on the
	 * bus. Returns 0 on ACK, negative errno on NACK / bus error.
	 */
	int probeAck(uint8_t addr);

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
