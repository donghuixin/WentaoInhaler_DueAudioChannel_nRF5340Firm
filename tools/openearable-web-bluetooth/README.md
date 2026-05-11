# OpenEarable BLE Console

Local Web Bluetooth console for OpenEarable v2.

Run:

```powershell
node server.js --port=8766
```

Open:

```text
http://127.0.0.1:8766/
```

Use Chrome or Edge on a machine with a Bluetooth adapter. Web Bluetooth requires a secure context; `localhost` and `127.0.0.1` are accepted by Chromium browsers.

IMU quick test:

1. Click `Connect OpenEarable` and select `OpenEarable-XXXX`.
2. Click `Read Battery` to read Battery Service `180F` / Battery Level `2A19`.
3. Click `Start IMU`. The page subscribes to Sensor Data Notify and writes `00 01 01` to Sensor Config.
4. Move the board and watch `Accel` / `Gyro` values update. `Mag` remains zero unless the BMM150 AUX path is implemented in firmware.
5. Click `Stop IMU` to write `00 01 00`.

Sensor packets are GATT notifications, not advertisement data. Advertisement scanning only confirms that the device is visible and advertising.

Audio waveform quick test:

1. Build and flash firmware that includes `Audio Waveform Service` UUID `1410dfa0-5f68-4ebb-a7c7-5e0fb9ae7557`.
2. Connect from this page and confirm the service list contains `Audio Waveform Service`.
3. Click `Start Wave`. The page subscribes to waveform notifications, writes `03` to `Audio Waveform Control`, then writes `02 00 02` to `Sensor Config` to start the microphone/audio datapath.
4. Watch the `Audio Waveform` canvas and `Peak` / `Mean Abs` metrics. A moving waveform means ADAU1860 DMIC to nRF I2S RX to BLE preview is producing audio samples.
5. Click `Stop Wave` to write `00` to waveform control and `02 00 00` to stop the microphone path.

The waveform channel is a low-rate preview. It sends compact signed 8-bit waveform snapshots over GATT Notify, not full-quality PCM audio.

Expected OpenEarable names:

- Normal firmware: `OpenEarable-XXXX`
- DFU mode: `OpenEarable_L_DFU` or `OpenEarable_R_DFU`

If no scanner can see the device after a mass erase flash, write the channel and hardware revision UICR values again, reset the board, and then scan.
