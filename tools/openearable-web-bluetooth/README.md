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
3. In `Audio Waveform`, select the microphone inputs:
   - `MP1 DMIC1`: the single MP1 microphone on ADAU1860 DMIC1.
   - `MP2 Left`: the left microphone on MP2 / DMIC23.
   - `MP2 Right`: the right microphone on MP2 / DMIC23.
4. Adjust `Gain` or `Noise Gate` if needed, then click `Apply Mic Config`.
5. Click `Start Wave`.

The waveform service sends a fixed 244-byte value every 7.5 ms: a little-endian uint32 I2S first-sample timestamp followed by 120 mono PCM16 samples. Including ATT and L2CAP headers, this fits one 251-byte Link Layer PDU.

IMU and Thermal IR are not discarded. They share the `34c2e3c1-...` Sensor
Stream characteristic, while audio uses the Audio Waveform characteristic.
IR packets are drained before IMU batches; IMU acquisition still runs at
100 Hz on its own work queue.

The page writes microphone selection as a `micSelect` bitmask (`0x01` MP1, `0x02` MP2 left, `0x04` MP2 right) and writes input controls through `micControl` (`gain_reg`, `noise_gate_threshold_le16`). The firmware routes the first selected microphone to I2S left and the second selected microphone to I2S right. BLE waveform preview still displays the primary mono slot; full independent three-channel audio requires a wider firmware transport format.

Recorded BLE audio is appended sequentially. The packet timestamp is the I2S capture time of the first PCM sample, so timestamp deltas detect real missing samples without turning Windows notification arrival jitter into artificial holes. Missing samples are bridged with a linear transition instead of hard zero-filled gaps.

The canvas uses AC removal and auto gain. Use the numeric `Peak`, `Mean Abs`, and `Peak-to-Peak` values when judging actual amplitude.

## Thermal IR Camera (MLX90642)

The **Thermal IR Camera** panel is at the bottom of the page (scroll down past Services / Log).

Bus mapping used by firmware:

- `IIC0`: P0.24/P0.21 -> Zephyr `&i2c1`
- `IIC1`: P1.00/P1.15 -> Zephyr `&i2c2` (**MLX90642 on this bus**)
- `IIC2`: P1.02/P1.03 -> Zephyr `&i2c3`

1. Connect to `OpenEarable-XXXX`.
2. Select refresh rate (default **8 Hz**).
3. Click **Start Thermal**.
4. Watch the 32×24 heatmap and metrics (FPS, Min/Max/Avg °C, chunk progress).

The panel subscribes to the shared Sensor Stream characteristic. Common header:

```text
[timestamp_us : u64 LE]
[sequence : u16 LE]
[packet_type : u8][item_count : u8]
[item_offset : u16 LE]
[packet_index : u8][packet_count : u8]
[payload]
```

For Thermal IR, `packet_type=1`, `item_count=pixel_count`, and the payload is:

```text
[int16 raw pixels x pixel_count]
temperature_celsius = raw / 50.0
```

One BLE frame = seven packets: six packets with 114 pixels and one with 84 pixels. The Web renders and records a frame only after all 768 pixels have arrived. SD storage still uses 48 independent 16-pixel CSV chunks.

For IMU, `packet_type=2`; one packet carries five records:

```text
[timestamp_delta_us : u32 LE]
[accel xyz : 3 x float32 LE]
[gyro xyz : 3 x float32 LE]
[mag xyz : 3 x float32 LE]
```

Palette options: Iron (default), Jet, Grayscale. Toggle **Auto Range** or set manual Min/Max °C.

## SD card Data logger

Use the **SD card Data logger** panel to switch into SD-only recording mode. The page stops live BLE data notifications and sends only sensor configuration commands.

Files created on the card follow:

```text
IMU_<timestamp>.csv
IR_<timestamp>.csv
Audio_<timestamp>.csv
Audio_<timestamp>_L.pcm
Audio_<timestamp>_R.pcm
```

Audio channel bits are sent in `storageOptions` with `0x10` for left and `0x20` for right, in addition to `0x02` for `DATA_STORAGE`.

## BLE Data Logger

Audio, IMU, and Thermal IR can be selected together. Audio arrives on the
Audio Waveform characteristic; IMU and IR arrive on the shared Sensor Stream.
The downloaded CSV is sorted by the shared device capture timestamp:

```text
timestamp_us,type,format,data...
```

- `AUDIO`: first-sample timestamp plus all 120 raw PCM16 samples.
- `IMU`: one timestamp plus configured rate, accel, gyro, and BMM150 magnetometer values.
- `IR`: one row per complete 32 x 24 frame, including configured rate, frame sequence, and all 768 raw MLX90642 pixels.

The BLE logger defaults to 100 Hz IMU, 8 Hz IR, and 16 kHz Audio. Audio uses
about 133.3 notifications/s, IR uses 56 notifications/s, and five-sample IMU
batches use 20 notifications/s, for about 209.3 notifications/s total. It
reuses streams only when their rates match the selected logger rates;
otherwise it temporarily switches them and restores the previous
configuration afterward. Starting a recording resets the audio firmware ring
to exclude pre-session backlog. Timestamp gaps, audio missing samples, and IR
frame-sequence gaps are displayed as separate counters plus a total. Stopping
the logger only stops streams and notifications that the logger itself
started. Data storage remains unthrottled, while on-screen values and the
audio canvas update at a lower visual frame rate to keep Chrome/Edge
responsive.

The firmware log must confirm the final Windows-negotiated values:

```text
Conn params updated ... 7.500 ms
LE data len updated ... 251 bytes
LE PHY updated ... 2M/2M
ATT MTU updated ... >=247
```

## Hardware Status Panel

After BLE connection, the page auto-reads `HW Status` and shows:

- Bus scan results for `IIC0/IIC1/IIC2` (`ready`, `count`, `found` addresses)
- Key device probe state (`ok`) and resolved bus names
- Derived high-level flags (`thermal_ir`, `imu`, `fuel_gauge`, etc.)

If thermal is missing, first verify `mlx90642_thermal` appears on `IIC1` with `0x66`.
