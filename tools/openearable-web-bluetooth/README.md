# OpenEarable Web Bluetooth Console

Static Web Bluetooth console for the `48khz` branch.

Run from this directory:

```powershell
node server.js --port=8766
```

Open Chrome or Edge:

```text
http://127.0.0.1:8766/
```

The page connects to the OpenEarable GATT services. It is not an advertisement-only scanner.

## IMU

1. Connect to `OpenEarable-XXXX`.
2. Select `IMU` in `Sensor Stream`.
3. Use `Rate Index = 1`.
4. Click `Enable Stream` and `Start IMU`.
5. `Packets` should increase and accel/gyro values should update.

## Audio Waveform

1. Flash the `48khz` branch with `prj.conf` and `build_48khz_pcm16`.
2. Connect from the page.
3. In `Audio Waveform`, select a window:
   - `2 ms raw`: 96 PCM16 samples, no decimation.
   - `10 ms mid`: 480 PCM16 samples, no decimation.
   - `50 ms low`: 2400 PCM frames represented as 480 averaged points.
   - `100 ms low`: 4800 PCM frames represented as 480 averaged points.
4. Click `Start Wave`.

The waveform service sends PCM16 preview chunks over BLE. It is useful for checking the ADAU1860/I2S microphone path, but it is not a full continuous audio recorder.

The canvas uses AC removal and auto gain. Use the numeric `Peak`, `Mean Abs`, and `Peak-to-Peak` values when judging actual amplitude.
