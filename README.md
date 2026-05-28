# OpenEarable 48 kHz PCM16 + Thermal IR Development Branch

This branch is a working OpenEarable firmware and Web Bluetooth setup for the current hardware bring-up. It is not the upstream OpenEarable 2 README. The focus of this branch is:

- nRF Connect SDK 3.0.1 build for `openearable_v2/nrf5340/cpuapp`
- 48 kHz PCM16 audio preview from the ADAU1860/I2S path
- Web Bluetooth connection from Chrome/Edge on Windows
- IMU streaming with the BMI270-compatible board wiring used in this project
- MLX90642 32x24 thermal IR camera streaming over BLE (IIC1: P1.00/P1.15)
- keeping the 48 kHz build usable while avoiding the previous 96 kHz experiment path

## Current Branch

Use the thermal-enabled branch:

```powershell
git checkout HuixinThermal
```

Base branch (48 kHz audio only):

```powershell
git checkout 48khz
```

Remote:

```text
https://github.com/ljqljqljq8/OpenEarable/tree/HuixinThermal
https://github.com/ljqljqljq8/OpenEarable/tree/48khz
```

Recent branch contents include:

- `CONFIG_OPENEARABLE_WEB_BLE_LEGACY_ADV=y` so Windows/Chrome Web Bluetooth can discover the device.
- BMI270 deferred initialization so the IMU is initialized after the sensor rail is powered.
- `Audio Waveform Service` for PCM16 waveform preview over BLE GATT.
- MLX90642 driver on IIC1 with chunked BLE thermal frame transport (`ID_THERMAL = 8`).
- Web UI under `tools/openearable-web-bluetooth/` for IMU, audio waveform, and thermal IR heatmap testing.

## Required Toolchain

Use the nRF Connect VS Code extension with:

```text
nRF Connect SDK: v3.0.1
nRF Connect Toolchain: v3.0.1
Board: openearable_v2/nrf5340/cpuapp
```

Do not point the nRF Connect extension at an old SDK workspace. The SDK top directory should be the current workspace that contains this repository and the matching NCS modules, for example:

```text
E:\Projects\Project_2026\Openarable_Project
```

The build should use the NCS toolchain Python, CMake, Ninja, and Zephyr SDK from the installed nRF Connect toolchain. Do not force the system Python into the build.

## Recommended VS Code Build Configuration

Create a fresh build configuration from the nRF Connect sidebar:

```text
Application: open-earable-2
Board target: openearable_v2/nrf5340/cpuapp
SDK/toolchain: v3.0.1
Build directory name: build_48khz_pcm16
Pristine build: Yes
```

Kconfig and CMake selections:

```text
Base configuration files: prj.conf
Extra Kconfig fragments: empty
Base Devicetree overlays: empty
Extra Devicetree overlays: empty
Extra CMake arguments: empty
System build: Use sysbuild
```

Do not select the FOTA file suffix for this debug branch unless you intentionally need the FOTA/sysbuild variant. The current bring-up and Web BLE tests use `prj.conf`.

## Flashing

After the build finishes, use the nRF Connect `Flash` action for the `build_48khz_pcm16` configuration.

If the device has flash protection or a previous incompatible image, use mass erase/recover through nRF Connect or `west flash --erase`. A successful flash should show both images programmed on nRF5340, including the network core image.

After a mass erase or a fresh board recovery, write the UICR values used by this hardware configuration:

```powershell
nrfutil device write --serial-number 802002436 --family nrf53 --core application --address 0x00FF80F4 --value 0
nrfutil device write --serial-number 802002436 --family nrf53 --core application --address 0x00FF8100 --value 0x02000000
nrfutil device reset --serial-number 802002436 --family nrf53
```

Replace `802002436` with the serial number of your J-Link/nRF debug probe.

## Web Bluetooth Console

The Web Bluetooth test UI is in:

```text
tools/openearable-web-bluetooth/
```

Run a local static server from that directory. This repository includes a small Node.js server:

```powershell
cd tools\openearable-web-bluetooth
node server.js --port=8766
```

Open Chrome or Edge at:

```text
http://127.0.0.1:8766/
```

Web Bluetooth requires a secure context; Chromium accepts `localhost` and `127.0.0.1`.

If the page was already open before an update, hard refresh it with `Ctrl+F5`.

## BLE Connection Notes

Expected device name format:

```text
OpenEarable-XXXX
```

If Chrome cannot find the device:

- Disconnect it from phones and other browser tabs.
- Power-cycle or reset the board.
- Confirm the flashed build contains `CONFIG_OPENEARABLE_WEB_BLE_LEGACY_ADV=y`.
- Re-write the UICR values after mass erase.
- Use `Connect Any BLE` if the browser filter misses the advertised name.

The Web BLE page connects to GATT services. Advertisement data alone is not enough for sensor streaming.

## IMU Test

The current hardware does not use a single BMX160 device. The IMU-compatible path is:

```text
BMI270 accel/gyro on the main I2C bus
BMM150 magnetometer behind the BMI270 AUX interface
```

The branch keeps BMI270 initialization deferred until after the sensor power rail is available. This avoids the earlier boot-time `device not ready` failure.

Web test steps:

1. Connect to `OpenEarable-XXXX` from the Web Bluetooth page.
2. In `Sensor Stream`, select `IMU`.
3. Use `Rate Index = 1`.
4. Click `Enable Stream`.
5. Click `Start IMU`.
6. `Packets` should increase and accelerometer/gyro values should update when the board moves.

If packets stay at zero, verify that the firmware is the current `48khz` branch build and that the page is connected to the GATT data notification characteristic.

## 48 kHz PCM16 Audio Preview

The audio preview path is:

```text
SPH0641LU4H-1 digital microphone -> ADAU1860 PDM/DMIC input -> ADAU1860 I2S output -> nRF5340 I2S RX -> BLE Audio Waveform Service -> Web UI
```

The BLE waveform service is a diagnostic preview channel. It is not a full continuous audio recorder.

Current packet format:

```text
PCM format: signed PCM16 mono preview
PCM source rate: 48,000 Hz
BLE characteristic: Audio Waveform Data
Service UUID: 1410dfa0-5f68-4ebb-a7c7-5e0fb9ae7557
```

The Web UI supports four windows:

| Mode | Source PCM frames | Display points | Effective plot rate | Intended use |
| --- | ---: | ---: | ---: | --- |
| `2 ms raw` | 96 | 96 | 48 kHz | high-frequency local shape |
| `10 ms mid` | 480 | 480 | 48 kHz | 300 Hz to several kHz, raw point display |
| `50 ms low` | 2400 | 480 | 9.6 kHz | low-frequency trend and stable frequency estimate |
| `100 ms low` | 4800 | 480 | 4.8 kHz | very low-frequency trend |

Important display behavior:

- `2 ms raw` and `10 ms mid` do not decimate; displayed samples are raw PCM16 points from the preview window.
- `50 ms low` averages every 5 PCM points into one display point.
- `100 ms low` averages every 10 PCM points into one display point.
- The canvas currently shows an AC-coupled, auto-scaled waveform. This is useful for seeing shape and frequency, but it is not fixed-scale amplitude display.
- Use the numeric `Peak`, `Mean Abs`, and `Peak-to-Peak` values to judge actual raw amplitude.

Typical interpretation:

```text
Silence / room noise:
Peak around 400-600 raw
Peak-to-Peak around 800-1200 raw

Reliable external tone:
Peak should preferably exceed 1500 raw
Peak-to-Peak should preferably exceed 3000 raw
```

For 400 Hz to 2 kHz tones, start with `10 ms mid`. For 100 Hz to 300 Hz tones, use `50 ms low` or `100 ms low`, but remember those modes are decimated previews.

## MLX90642 Thermal IR Camera

The thermal path is:

```text
MLX90642 (32x24, 768 pixels) -> IIC1 (TWIM alias) -> Thermal sensor wrapper -> BLE Sensor Service -> Web UI heatmap
```

IIC bus mapping in this project (strict pin definition):

- `IIC0`: SCL `P0.24`, SDA `P0.21` -> Zephyr `&i2c1`
- `IIC1`: SCL `P1.00`, SDA `P1.15` -> Zephyr `&i2c2`
- `IIC2`: SCL `P1.02`, SDA `P1.03` -> Zephyr `&i2c3`

Device assignment:

- `IIC0`: `bq27220` (0x55), `bq25120a` (0x6A), `bmi270` (0x68), `adau1860` (0x64)
- `IIC1`: `mlx90642` (0x66), `maxm86161` (0x62), `mlx90632` (0x3A)
- `IIC2`: reserved/other bring-up devices

Thermal hardware notes:

- Sensor is on **IIC1** (Zephyr `&i2c2`) in `openearable_v2_nrf5340_cpuapp_common.dts`.
- DTS 7-bit address is `0x66`.
- Do not use `0x33` unless your module is really strapped to that address.
- `Thermal::init()` enables both `ls_1_8` and `ls_3_3` (MLX90642 needs 3.0–3.6 V).
- Allow up to ~3 minutes after power-on for thermal stabilization before expecting full accuracy.

**CRITICAL 1.8V I2C REQUIREMENTS (nRF5340 Compatibility):**
1. **EEPROM 0x11FC (Analog Config)**: The nRF5340 GPIOs run at 1.8V. The MLX90642 factory default for I2C VIH is VDD-referenced (0.7 × 3.3V = 2.31V), which causes the sensor to NACK 1.8V signals.
   - You MUST use a 3.3V I2C master (e.g., an Arduino) to set **bit 2 of EEPROM 0x11FC to 1** before connecting it to the nRF5340.
   - This changes the MLX90642 I2C threshold to 1.8V-reference mode (VIH ≈ 1.26V), allowing the nRF5340 to communicate successfully.
2. **Boot-Up Sequence**: The MLX90642 requires a minimum of **600ms POR (Power-On Reset) delay** after `ls_3_3` (3.3V power rail) is enabled before it will ACK any I2C commands. The driver implements this wait.

Why thermal search could fail before this change:

- **1.8V I2C Voltage Mismatch**: As mentioned above, missing the 0x11FC EEPROM configuration caused a silent NACK because the 1.8V logic level couldn't trigger the sensor's 3.3V-referenced threshold.
- Bus mismatch: board wiring used `IIC1(P1.00/P1.15)` while previous DTS/driver path still pointed to a different bus.
- Address mismatch: mixed use of `0x33` and `0x66` caused probe to miss the device on some builds.
- Power domain timing: if `ls_1_8`/`ls_3_3` is not up before probe, I2C scan reports `NACK`.

Current fix status in this branch:

- `mlx90642` DTS node moved to Zephyr `&i2c2` (project `IIC1`).
- MLX90642 driver now selects TWIM bus from DTS parent at compile time (no hard-coded bus).
- MLX90642 driver now reads and verifies `0x11FC` during boot and warns if the 1.8V threshold bit is missing.
- HW status JSON now reports buses as `IIC0/IIC1/IIC2` with matching probe targets.

Driver layout (ported from the third-party Arduino MLX90642 v1.0.3 library):

```text
src/SensorManager/MLX90642/MLX90642.{h,cpp}   # Zephyr/TWIM register access + block read
src/SensorManager/Thermal.{h,cpp}             # EdgeMlSensor wrapper, chunking, timer
```

Refresh rates (EEPROM `0x11F0` bits 0:2):

| Rate index | MLX90642 code | Frame rate |
| ---: | ---: | ---: |
| 0 | 2 | 2 Hz |
| 1 | 3 | 4 Hz (default) |
| 2 | 4 | 8 Hz |
| 3 | 5 | 16 Hz |

BLE transport (sensor ID `8`, `ID_THERMAL`):

Each thermal frame is split into **43 chunks** (18 pixels per chunk, last chunk 12 pixels). All chunks of one frame share the same `sensor_data.time` timestamp.

Per-packet payload (after the 10-byte `sensor_data` header):

```text
[chunk_idx : u8][pixel_count : u8][int16 raw_pixel × pixel_count]
```

Raw pixel value: `temperature_celsius = raw / 50.0`

Web test steps:

1. Flash the `HuixinThermal` branch build.
2. Connect from `http://127.0.0.1:8766/` (hard refresh with `Ctrl+F5` after updates).
3. Scroll to the **Thermal IR Camera** panel at the bottom of the page.
4. Select refresh rate (default **4 Hz**).
5. Click **Start Thermal**.
6. The 32×24 heatmap should update; metrics show FPS, Min/Max/Avg °C, and chunk progress (`43/43`).

Alternatively, use the generic **Sensor Stream** controls: select `Thermal IR`, set rate index, **Enable Stream**, and **Subscribe Data**.

## Known Limitations

- The Web Audio Waveform panel is a BLE diagnostic preview, not a lossless audio capture tool.
- The canvas uses AC removal and auto gain, so quiet noise can look visually large.
- The frequency estimator is unreliable when the signal is close to the noise floor.
- The 96 kHz experiment branch did not solve low-frequency display quality; this branch stays on 48 kHz and improves the preview windowing instead.
- True raw long-window audio capture should use SD card, USB, RTT, or a dedicated streaming protocol instead of this BLE preview characteristic.
- Thermal IR at 8 Hz sends ~344 BLE packets/s (43 chunks × 8 frames); use 4 Hz for a safer margin on congested links.
- The existing `MLX90632` skin-temperature sensor on IIC1 / Zephyr `&i2c2` (`ID_OPTTEMP`) is unchanged; thermal IR is a separate sensor (`ID_THERMAL`).

## Useful Files

```text
prj.conf
boards/teco/openearable_v2/openearable_v2_nrf5340_cpuapp_common.dts
include/openearable_common.h
src/SensorManager/IMU.cpp
src/SensorManager/MLX90642/MLX90642.{h,cpp}
src/SensorManager/Thermal.{h,cpp}
src/SensorManager/SensorManager.cpp
src/ParseInfo/DefaultSensors.h
src/bluetooth/gatt_services/audio_waveform_service.c
src/bluetooth/gatt_services/audio_waveform_service.h
src/audio/audio_datapath.c
tools/openearable-web-bluetooth/index.html
tools/openearable-web-bluetooth/app.js
tools/openearable-web-bluetooth/style.css
tools/openearable-web-bluetooth/server.js
```

## Development Rule

For this branch, prefer small, testable changes:

1. Keep `prj.conf` as the default debug build configuration.
2. Keep Web Bluetooth legacy advertising enabled unless deliberately testing phone-only behavior.
3. Do not reintroduce the 96 kHz configuration into the `48khz` / `HuixinThermal` branches.
4. Validate IMU, audio preview, and thermal IR separately after each firmware change.
5. When changing waveform transport, keep the Web UI packet parser in sync with `audio_waveform_packet`.
6. When changing thermal chunk layout, keep `Thermal.cpp`, `app.js`, and this README in sync.
