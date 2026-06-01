# OpenEarable Inhaler Firmware and Web Console

This repository contains the nRF5340 firmware and the Web Bluetooth dashboard for the OpenEarable-based inhaler sensing platform.

## Build and Flash

Target:

```text
openearable_v2/nrf5340/cpuapp
```

Recommended SDK:

```text
nRF Connect SDK v3.0.1
```

Typical build from this workspace:

```powershell
C:\ncs\toolchains\0b393f9e1b\opt\bin\python.exe -m west build E:\InhalernRF5340Firm -b openearable_v2/nrf5340/cpuapp -d E:\InhalernRF5340Firm\build --sysbuild -p always -- -DBOARD_ROOT=E:/InhalernRF5340Firm
```

If the board was mass erased, write the UICR values before normal testing:

```powershell
nrfutil device write --serial-number <JLINK_SERIAL> --family nrf53 --core application --address 0x00FF80F4 --value 0
nrfutil device write --serial-number <JLINK_SERIAL> --family nrf53 --core application --address 0x00FF8100 --value 0x02000000
nrfutil device reset --serial-number <JLINK_SERIAL> --family nrf53
```

## Verified Hardware Map

The SD card wiring is already represented in the board DTS and pinctrl files:

| Signal | nRF pin | Source |
| --- | --- | --- |
| SD load switch enable | P1.12 | `load_switch_sd.enable-gpios = <&gpio1 12 GPIO_ACTIVE_HIGH>` |
| SD card detect / state | P0.15 | `sd_state.gpios = <&gpio0 15 ...>` |
| SPI CS | P0.11 | `&spi4 cs-gpios = <&gpio0 11 GPIO_ACTIVE_LOW>` |
| SPI CLK | P0.08 | `spi4_default SPIM_SCK` |
| SPI MOSI | P0.09 | `spi4_default SPIM_MOSI` |
| SPI MISO | P0.10 | `spi4_default SPIM_MISO` |

The SD card is exposed as a Zephyr `sdmmc-disk` named `SD` mounted at `/SD:`.

The BMM150 magnetometer is connected behind the BMI270 auxiliary interface:

| BMM150 pin | BMI270 pin |
| --- | --- |
| SDI | ASDx |
| SCK | ASCx |

The firmware enables the BMI270 auxiliary I2C path, sets its auxiliary read cadence to the selected IMU cadence, and appends magnetometer values after accelerometer and gyroscope values in both BLE TDM and SD IMU records. The BMM150 internal ODR table tops out at 30 Hz, so requests above that keep the BMI270 auxiliary slot synchronized to the IMU timer while the magnetometer reports the latest available BMM150 sample.

## Sensor Configuration Packet

The Web console controls sensors through the Sensor Service config characteristic.

```text
offset  size  field
0       1     sensorId
1       1     sampleRateIndex
2       1     storageOptions
```

`storageOptions` bits:

```text
0x01 DATA_STREAMING  send live BLE sensor/TDM data
0x02 DATA_STORAGE    record to SD
0x10 AUDIO_LEFT      SD audio left channel
0x20 AUDIO_RIGHT     SD audio right channel
```

When SD logging is selected from the Web console, BLE is used for control commands only; sensor data notifications and audio TDM notifications are stopped before storage configs are written.

## Generic Sensor Data Packet

Most sensors publish a packed `sensor_data` record:

```text
offset  size  field
0       1     id
1       1     payload size in bytes
2       8     timestamp_us, little-endian
10      38    payload buffer
```

Only the first `payload size` bytes are valid. The SD binary `.oe` stream writes the packet header plus only the valid payload bytes.

### IMU Payload

Sensor ID: `0`

```text
float32 accel_x_mps2
float32 accel_y_mps2
float32 accel_z_mps2
float32 gyro_x_dps
float32 gyro_y_dps
float32 gyro_z_dps
float32 mag_x_uT
float32 mag_y_uT
float32 mag_z_uT
```

Total payload: 36 bytes. Magnetometer data is placed immediately after gyro data.

### Thermal IR SD Payload

Sensor ID: `8`

The MLX90642 32 x 24 frame is stored in 48 half-row chunks. Each chunk stays inside the fixed 38-byte payload:

```text
offset  size  field
0       1     chunk_index, 0..47
1       1     pixel_count, normally 16
2       32    int16 raw pixels, little-endian
```

Temperature conversion:

```text
temperature_celsius = raw / 50.0
```

## BLE TDM Composite Packet

Live audio, thermal rows, and IMU preview values are multiplexed through the Audio Waveform Service data characteristic.

Packet size: 251 bytes.

```text
offset  size  field
0       1     seq
1       4     uptime_ms, big-endian
5       1     mic_valid_len
6       160   mono PCM16 little-endian, left channel, up to 80 samples
166     1     thermal_valid_len
167     1     thermal_row_index, 0..23, or 0xff when empty
168     64    thermal row, 32 x int16 big-endian raw pixels
232     1     imu_valid_len
233     18    IMU preview, 9 x int16 little-endian
```

IMU preview scaling:

```text
accel_mps2 = raw * 9.80665 / 1000
gyro_dps   = raw * 0.1
mag_uT     = raw * 0.1
```

Thermal row stitching is strict. The Web console starts a frame only when `thermal_row_index == 0`, then requires rows `1..23` in order. If any row is missing, duplicated, or out of order, the partial frame is dropped and the UI waits for the next row 0. This prevents the severe row-shift artifacts caused by guessing row order.

## SD Card Data Logger

Starting any sensor with `DATA_STORAGE` mounts `/SD:`, opens the legacy binary session file, and also creates typed sidecar files.

File naming rule:

```text
<SensorType>_<storage_start_timestamp><suffix>
```

Examples:

```text
IMU_123456789.csv
IR_123456789.csv
Audio_123456789.csv
Audio_123456789_L.pcm
Audio_123456789_R.pcm
sensor_log_123456700.oe
```

CSV files always use the first column for the packet/block timestamp.

### IMU CSV

```text
timestamp_us,accel_x_mps2,accel_y_mps2,accel_z_mps2,gyro_x_dps,gyro_y_dps,gyro_z_dps,mag_x_uT,mag_y_uT,mag_z_uT,payload_size,payload_00...
```

### IR CSV

```text
timestamp_us,chunk_index,pixel_count,pixel_00_raw...pixel_15_raw,payload_size,payload_00...
```

### Audio Files

Audio is deinterleaved into raw signed 16-bit PCM files:

```text
Audio_<timestamp>_L.pcm
Audio_<timestamp>_R.pcm
```

The companion `Audio_<timestamp>.csv` indexes each written block:

```text
timestamp_us,channel,byte_count,sample_rate_hz
```

The Web console exposes 16, 24, 32, 48, and 96 kHz choices. The current audio hardware path is still governed by the compiled `CONFIG_AUDIO_SAMPLE_RATE_HZ`; the requested index is passed through the normal sensor config and recorded in control state, while the SD block metadata records the actual firmware sample rate used by the I2S path.

## Web Bluetooth Console

Run the local console:

```powershell
cd tools/openearable-web-bluetooth
node server.js --port=8766
```

Open:

```text
http://127.0.0.1:8766/
```

Main panels:

- Thermal IR Camera: live 32 x 24 view, 2/4/8 Hz BLE preview, strict row stitching.
- Sensor Stream & IMU: generic sensor control and 9-axis IMU values.
- Audio Waveform: 16 kHz TDM preview.
- SD card Data logger: SD-only logging control. Sensor choices include IMU, IR at 2/4/8 Hz, and audio left/right channels with `_L` and `_R` file suffixes.
- Hardware Status: I2C/device probe status and boot/recent diagnostic log.

Hard-refresh the browser after firmware or Web UI updates so the latest `app.js` is loaded.
