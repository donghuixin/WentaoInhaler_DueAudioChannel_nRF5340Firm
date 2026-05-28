# OpenEarable Web Bluetooth Console

Static Web Bluetooth console for the `HuixinThermal` / `48khz` branches.

Run from this directory:

```powershell
node server.js --port=8766
```

Open Chrome or Edge:

```text
http://127.0.0.1:8766/
```

The page connects to the OpenEarable GATT services. It is not an advertisement-only scanner.

Hard-refresh (`Ctrl+F5`) after pulling firmware or UI updates so the browser loads the latest `app.js`.

## IMU

1. Connect to `OpenEarable-XXXX`.
2. Select `IMU` in `Sensor Stream`.
3. Use `Rate Index = 1`.
4. Click `Enable Stream` and `Start IMU`.
5. `Packets` should increase and accel/gyro values should update.

## Audio Waveform

1. Flash the branch with `prj.conf` and `build_48khz_pcm16`.
2. Connect from the page.
3. In `Audio Waveform`, select a window:
   - `2 ms raw`: 96 PCM16 samples, no decimation.
   - `10 ms mid`: 480 PCM16 samples, no decimation.
   - `50 ms low`: 2400 PCM frames represented as 480 averaged points.
   - `100 ms low`: 4800 PCM frames represented as 480 averaged points.
4. Click `Start Wave`.

The waveform service sends PCM16 preview chunks over BLE. It is useful for checking the ADAU1860/I2S microphone path, but it is not a full continuous audio recorder.

The canvas uses AC removal and auto gain. Use the numeric `Peak`, `Mean Abs`, and `Peak-to-Peak` values when judging actual amplitude.

## Thermal IR Camera (MLX90642)

The **Thermal IR Camera** panel is at the bottom of the page (scroll down past Services / Log).

Bus mapping used by firmware:

- `IIC0`: P0.24/P0.21 -> Zephyr `&i2c1`
- `IIC1`: P1.00/P1.15 -> Zephyr `&i2c2` (**MLX90642 on this bus**)
- `IIC2`: P1.02/P1.03 -> Zephyr `&i2c3`

1. Connect to `OpenEarable-XXXX`.
2. Select refresh rate (default **4 Hz**).
3. Click **Start Thermal**.
4. Watch the 32×24 heatmap and metrics (FPS, Min/Max/Avg °C, chunk progress).

Or use the generic **Sensor Stream** controls:

1. Select `Thermal IR` (sensor ID 8).
2. Set `Rate Index` (0 = 2 Hz, 1 = 4 Hz, 2 = 8 Hz).
3. Click **Enable Stream** and **Subscribe Data**.

Packet format (per BLE notification, after the 10-byte sensor header):

```text
[chunk_idx : u8][pixel_count : u8][int16 raw × pixel_count]
temperature_celsius = raw / 50.0
```

One frame = 43 chunks (768 pixels). All chunks of a frame share the same timestamp field.

Palette options: Iron (default), Jet, Grayscale. Toggle **Auto Range** or set manual Min/Max °C.

## Hardware Status Panel

After BLE connection, the page auto-reads `HW Status` and shows:

- Bus scan results for `IIC0/IIC1/IIC2` (`ready`, `count`, `found` addresses)
- Key device probe state (`ok`) and resolved bus names
- Derived high-level flags (`thermal_ir`, `imu`, `fuel_gauge`, etc.)

If thermal is missing, first verify `mlx90642_thermal` appears on `IIC1` with `0x66`.
