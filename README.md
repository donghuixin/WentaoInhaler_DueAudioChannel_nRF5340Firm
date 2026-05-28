# OpenEarable Inhaler Console & Firmware - Developer Hand-off Guide

This repository contains the nRF5340 firmware and the Web Bluetooth (Inhaler Console) dashboard for the OpenEarable-based thermal and audio project.

This README is designed to help new developers **onboard quickly**, understand the hardware/software architecture, and start contributing immediately.

---

## 1. Quick Start: Build & Flash

### Prerequisites
- **nRF Connect SDK**: v3.0.1
- **Toolchain**: v3.0.1
- **VS Code** with the nRF Connect Extension.

### VS Code Build Configuration
Create a build configuration in the nRF Connect extension:
- **Board Target**: `openearable_v2/nrf5340/cpuapp`
- **Base configuration**: `prj.conf`
- **Build Directory**: `build`

### Flashing & UICR Initialization
After building, flash the board using your J-Link/nRF debug probe.
If the board is fresh or was mass-erased, you **must** write the UICR values:

```powershell
nrfutil device write --serial-number <YOUR_JLINK_SERIAL> --family nrf53 --core application --address 0x00FF80F4 --value 0
nrfutil device write --serial-number <YOUR_JLINK_SERIAL> --family nrf53 --core application --address 0x00FF8100 --value 0x02000000
nrfutil device reset --serial-number <YOUR_JLINK_SERIAL> --family nrf53
```

---

## 2. Hardware Architecture & I2C Map

The system utilizes three I2C buses (IIC0, IIC1, IIC2). Understanding this mapping is critical for adding new sensors.

| Bus  | Device        | Address | Purpose / Notes |
|------|--------------|---------|-----------------|
| IIC0 | **LSM6DS3**  | `0x6A`  | Main IMU (Accel & Gyro) |
| IIC1 | **BQ25120A** | `0x6A`  | PMIC / Battery Charger. *Crucial: shares address with LSM6DS3 but on a different bus.* |
| IIC1 | **BQ27220**  | `0x55`  | Battery Fuel Gauge |
| IIC1 | **MLX90642** | `0x66`  | Thermal IR Camera (32x24 pixels). Operates on 3.3V high-side logic. |
| IIC1 | **ADAU1860** | `0x64`  | Audio DSP / I2S |

> **⚠️ IMPORTANT PMIC NOTE (BQ25120A)**: 
> The PMIC's `CD` (Chip Disable) pin must be kept **HIGH** to maintain I2C communication and allow charging. If pulled LOW, the PMIC drops off the I2C bus and reverts to factory defaults, which re-enables the thermistor (TS) check. Without a physical thermistor, this triggers a `TS_FAULT` (0x40) and permanently blocks charging. This logic is handled in `src/Battery/BQ25120a.cpp`.

---

## 3. Firmware Architecture: How to Write Code

The firmware is based on Zephyr RTOS. The core application logic is split into several managers.

### Adding a New Sensor
1. **Device Tree**: Add the sensor to the corresponding I2C bus in `boards/teco/openearable_v2/openearable_v2_nrf5340_cpuapp_common.dts`.
2. **SensorManager**: Create a C++ class for your sensor in `src/SensorManager/`. Inherit from existing sensor bases if applicable.
3. **Initialization**: Instantiate your sensor in `main.cpp` and add it to the `SensorManager` list. Ensure you assign a unique `sensor_id` for BLE streaming.

### BLE Communication
- Sensor data is streamed via custom GATT characteristics.
- **Thermal Camera (MLX90642)**: Due to BLE MTU limits, the 768 pixels (32x24) are transmitted in **Chunks**. The firmware sends 16 pixels per chunk. The Web UI dynamically detects this chunk size and reassembles the frame.
- **Audio Waveform**: Audio is streamed as PCM16.

### Power & PMIC Initialization
- PMIC safe init happens very early in boot (`bq25120a_safe_init.c`) to bring up the 3.3V LDO for the sensors (MLX90642).
- It forces the PMIC's `LSCTRL` register to `0xE4` (3.3V).
- If you need to change voltage rails, do it here.

---

## 4. Inhaler Console (Web Bluetooth UI)

We built a modern, "Pro Max" Web Bluetooth dashboard to interact with the device.

### How to Run
```powershell
cd tools/openearable-web-bluetooth
node server.js --port=8766
```
Open `http://127.0.0.1:8766/` in Google Chrome or Microsoft Edge (Web Bluetooth requires Chromium-based browsers).

### UI Features & Code Structure
- **`index.html`**: Uses a CSS Grid + Sidebar layout. Includes sections for Device Info, Thermal Camera, IMU, Audio Waveform, and Advanced Diagnostics.
- **`style.css`**: Features a premium Glassmorphism design. 
  - **Day/Night Mode**: Controlled by `body[data-theme="light"]`.
- **`app.js`**: The core logic.
  - **Thermal Frame Assembly**: `decodeThermalChunk()` reads the `count` byte from the BLE packet (e.g., 16 or 18 pixels) and dynamically adapts the canvas mapping. This ensures backward compatibility with older firmware.
  - **Event Listeners**: Handles BLE connection, GATT characteristic subscriptions, and UI toggles (like the Theme toggle).

### Modifying the Web Console
- **To add a new metric**: Add the HTML placeholder in `index.html` (e.g., `<strong id="myMetric">-</strong>`), map it in the `els` object at the top of `app.js`, and update it inside the `decodeSensorData` function.
- **Troubleshooting**: If the UI stops responding or buttons don't work, check the Chrome DevTools Console (`F12`). A missing DOM element mapped in `app.js` will throw a `ReferenceError` and halt the script.

---

## 5. Branch Strategy & Git
- `HuixinThermal`: The main active branch containing the Thermal Camera + 48kHz Audio + Pro Max UI.
- `48khz`: Legacy base branch for audio only.

When committing changes, please follow conventional commits (e.g., `feat: added XYZ`, `fix: resolved I2C hang`).
