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

The firmware enables the BMI270 auxiliary I2C path, sets its auxiliary read cadence to the selected IMU cadence, and appends magnetometer values after accelerometer and gyroscope values in both BLE Sensor Data and SD IMU records. The BMM150 internal ODR table tops out at 30 Hz, so requests above that keep the BMI270 auxiliary slot synchronized to the IMU timer while the magnetometer reports the latest available BMM150 sample.

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
0x01 DATA_STREAMING  send live BLE sensor/audio data
0x02 DATA_STORAGE    record to SD
0x10 AUDIO_LEFT      SD audio left channel
0x20 AUDIO_RIGHT     SD audio right channel
```

When SD logging is selected from the Web console, BLE is used for control commands only; Sensor Data and Audio PCM notifications are stopped before storage configs are written.

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

### Shared Sensor Stream BLE Payload

Live BLE IR and IMU data use one shared `Sensor Stream` characteristic. The
firmware always drains pending IR packets before IMU packets. This lowers IMU
transport priority without lowering its 100 Hz acquisition rate.

```text
UUID: 34c2e3c1-34aa-11eb-adc1-0242ac120002

offset  size     common field
0       8        frame timestamp_us, little-endian
8       2        sequence, little-endian
10      1        packet_type: 1=Thermal IR, 2=IMU batch
11      1        item_count
12      2        item_offset, little-endian
14      1        packet_index
15      1        packet_count
16      variable payload
```

Thermal IR (`packet_type=1`) uses the frame capture timestamp and sequence.
`item_count` is the number of pixels and `item_offset` is their offset in the
32 x 24 image. Each packet carries at most 114 int16 pixels, so a complete
768-pixel frame uses seven notifications: six packets of 114 pixels and one
packet of 84 pixels. The maximum value is 244 bytes and fits one negotiated
251-byte Link Layer Data Length PDU.

The firmware spaces the seven notifications across the 125 ms frame period instead of injecting a 48-notification burst. At 8 Hz this is 56 IR notifications/s.

Temperature conversion:

```text
temperature_celsius = raw / 50.0
```

IMU (`packet_type=2`) batches five 9-axis samples into one notification.
`timestamp_us` is the first sample time, and each 40-byte sample record is:

```text
uint32 timestamp_delta_us
float32 accel_x, accel_y, accel_z
float32 gyro_x, gyro_y, gyro_z
float32 mag_x, mag_y, mag_z
```

At 100 Hz this produces 20 IMU notifications/s while preserving all five
individual timestamps and samples. A full IMU batch is 216 bytes.

### Thermal IR SD Payload

SD logging retains the 48 half-row records used by the IR CSV writer:

```text
offset  size  field
0       1     chunk_index, 0..47
1       1     pixel_count, normally 16
2       32    int16 raw pixels, little-endian
```

## BLE Audio PCM Packet

Audio uses the Audio Waveform Service data characteristic. IMU and Thermal IR
share the Sensor Stream characteristic described above. The streams remain
independent records with their own capture timestamps.

Characteristic value size: 244 bytes. With the 3-byte ATT notification header and 4-byte L2CAP header, the Link Layer payload is exactly 251 bytes and fits one negotiated Data Length Extension PDU.

```text
offset  size  field
0       4     first_sample_timestamp_us, little-endian uint32
4       240   120 mono PCM16 samples, little-endian
```

The timestamp is captured from the I2S clock for the first PCM sample, not from the BLE notification timer. At 16 kHz, 120 samples represent exactly 7.5 ms. The Web console appends samples sequentially and uses timestamp deltas only to detect real missing samples.

The firmware audio ring holds 64 packets:

```text
64 packets x 120 samples = 7680 samples = 480 ms at 16 kHz
```

This protects against short Windows/BLE scheduling stalls. It cannot compensate for sustained throughput below 16 kHz; a ring overflow is logged with the cumulative number of dropped PCM samples.

### BLE Connection Verification

The peripheral keeps its requested minimum and maximum connection interval at 7.5 ms, including after an audio underrun, and requests 251-byte LE data length plus 2M PHY. Windows is the central and may accept different values. Verify the final negotiated values from firmware logs, not only the request logs:

```text
Conn params updated: interval=6 units (7.500 ms) ... (target 7.500 ms)
LE data len updated: TX=251 bytes/... RX=251 bytes/... (target 251 bytes)
LE PHY updated: TX=2M RX=2M (target 2M/2M)
ATT MTU updated: TX=... RX=... bytes (target >=247)
```

The firmware emits a warning when the accepted interval is not 7.5 ms, data length is below 251 bytes, PHY is not 2M, or ATT MTU is below 247 bytes.

### BLE Logger CSV

The Web BLE logger subscribes to three characteristics when required:

- Audio: 244-byte Audio PCM notifications.
- IMU: five timestamped 9-axis samples per shared Sensor Stream notification.
- IR: shared Sensor Stream packets carrying up to 114 raw pixels, with seven packets per complete frame.

The logger reuses a sensor only when its active rate already matches the selected logger rate. If an active IMU or IR stream uses another rate, the logger temporarily applies its selected rate and restores the previous configuration when recording stops. Sensors started by the logger are tracked separately and only those sensors are stopped when recording ends. Firmware drains every queued sensor configuration command, because Zephyr `k_work` submissions can coalesce when several Web Bluetooth writes arrive close together.

For a new three-sensor BLE session, the Web defaults are 100 Hz IMU, 8 Hz IR,
and 16 kHz audio. This produces approximately 209.3 notifications/s: 133.3
audio, 20 batched IMU, and 56 IR. Visual metrics, counters, and the audio
canvas are throttled while every Audio and IMU sample and every complete IR
frame is retained.

The downloaded CSV uses `timestamp_us` as the first column and sorts all Audio, IMU, and IR records on one device-time timeline:

```text
timestamp_us,type,format,data...
```

Audio timestamps identify the first PCM sample. IMU timestamps identify the 9-axis sample, and the CSV format field includes its configured rate. An IR row contains one verified complete 32 x 24 frame with all 768 raw pixels, its frame sequence, and configured rate. The Web rejects incomplete or misordered IR frames and reports their missing packets in the dropped counter. It also estimates missing IMU samples from timestamp gaps. Starting BLE logging resets the firmware audio ring so pre-session PCM backlog is not written ahead of IMU/IR data.

## Audio Input Configuration

The Audio Config Service exposes microphone routing and noise controls so the Web console can match the three physical microphones on the board.

`micSelect` (`1410df97-5f68-4ebb-a7c7-5e0fb9ae7557`) is a one-byte bitmask:

```text
0x01 MP1 / ADAU1860 DMIC1
0x02 MP2 / DMIC23 left
0x04 MP2 / DMIC23 right
```

At least one bit must be selected. The firmware routes the first selected microphone to the I2S left slot and the second selected microphone to the I2S right slot. If only one microphone is selected, it is duplicated to both I2S slots. The current I2S and BLE preview formats carry at most two hardware slots, so selecting all three microphones powers the selected sources but only the first two are routed into the current stereo path. A true three-channel recording mode requires a wider audio transport format.

`micControl` (`1410df99-5f68-4ebb-a7c7-5e0fb9ae7557`) is a three-byte payload:

```text
offset  size  field
0       1     ADAU1860 DMIC gain register, 0x00..0x3f
1       2     PCM noise gate threshold, little-endian uint16
```

The default gain register is `0x00`. Earlier builds configured the ADAU1860 DMIC gain register to `0x20`, which raises the input level and can also raise the audible noise floor. The software noise gate defaults to `0` (off); nonzero values zero PCM samples whose absolute value is below or equal to the threshold before BLE preview, SD logging, or encoder use.

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

- Thermal IR Camera: live 32 x 24 view from seven timestamped large packets per frame.
- Sensor Stream & IMU: generic sensor control and 9-axis IMU values.
- Audio Waveform: 16 kHz PCM preview in fixed 244-byte/7.5 ms packets, microphone selection, DMIC gain, and software noise gate control.
- BLE Data logger: records Audio, IMU, and IR together and downloads a timestamp-sorted CSV.
- SD card Data logger: SD-only logging control. Sensor choices include IMU, IR at 2/4/8 Hz, and audio left/right channels with `_L` and `_R` file suffixes.
- Hardware Status: I2C/device probe status and boot/recent diagnostic log.

Hard-refresh the browser after firmware or Web UI updates so the latest `app.js` is loaded.
