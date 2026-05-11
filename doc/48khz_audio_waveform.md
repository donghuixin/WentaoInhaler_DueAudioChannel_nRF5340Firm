# 48 kHz Audio Waveform Preview

This branch contains a bring-up/debug path for viewing the on-board PDM microphone signal in a Web Bluetooth page.

## Audio Path

The on-board microphone is `SPH0641LU4H-1`. It is not connected directly to the nRF5340 as PCM. The current path is:

```text
SPH0641LU4H-1 PDM mic
  -> ADAU1860 DMIC input
  -> ADAU1860 decimation / routing
  -> I2S PCM
  -> nRF5340 I2S RX
  -> Audio Waveform GATT service
  -> Web Bluetooth waveform page
```

The system audio sample rate is currently 48 kHz, 16-bit, stereo PCM. The waveform page receives a short diagnostic preview extracted from this I2S RX stream. It does not receive the raw 1-bit PDM stream.

## ADAU1860 Configuration

The current ADAU1860 bring-up config enables the DMIC path and routes channel 0/1 through the decimator and ASRC output to SPT0/I2S. The important path in `src/drivers/ADAU1860.cpp` is:

```text
DMIC channel 0/1
  -> FDEC 0/1
  -> ASRCO 0/1
  -> SPT0 route 0/1
  -> nRF I2S RX
```

Current comments/configuration describe the internal ADAU path as decimating from 192 kHz to 48 kHz before the nRF receives PCM. The nRF-side sample rate is exposed to the web packet as `CONFIG_AUDIO_SAMPLE_RATE_HZ`, currently `48000`.

## BLE Waveform Service

The diagnostic GATT service is implemented in:

- `src/bluetooth/gatt_services/audio_waveform_service.h`
- `src/bluetooth/gatt_services/audio_waveform_service.c`
- `src/audio/audio_datapath.c`

UUIDs:

```text
Service: 1410dfa0-5f68-4ebb-a7c7-5e0fb9ae7557
Control: 1410dfa1-5f68-4ebb-a7c7-5e0fb9ae7557
Data:    1410dfa2-5f68-4ebb-a7c7-5e0fb9ae7557
```

The data packet contains:

- sequence number
- sample rate in Hz
- frame count
- left/right peak and mean absolute values
- a 96-sample signed 8-bit waveform preview
- mono raw min/max/peak-to-peak values

At 48 kHz, 96 frames represent a 2.00 ms window. This is enough to verify tones in the 1 kHz to 10 kHz range, but it is a preview window rather than a continuous oscilloscope-grade stream.

## Web Bluetooth Page

The web page source is included under:

```text
tools/openearable-web-bluetooth/
```

It connects over Web Bluetooth, starts the microphone preview, plots the current 2 ms waveform, and displays:

- sample rate
- frame count
- left/right peak
- mean absolute value
- estimated frequency
- peak-to-peak value
- estimate confidence

Frequency is estimated in the browser from positive-going zero crossings in the 96 preview samples. Peak-to-peak uses raw min/max fields when available and falls back to an estimate for older firmware packets.

## Build Notes

Use the normal `openearable_v2/nrf5340/cpuapp` sysbuild configuration. For the Windows Web Bluetooth diagnostic path, add the `debug_web_ble_legacy.conf` fragment so the device uses legacy connectable advertising that Chromium can show in the chooser.

Expected setup:

```text
Board: openearable_v2/nrf5340/cpuapp
System build: sysbuild
Base config: prj.conf or prj_fota.conf
Extra config: debug_web_ble_legacy.conf
Build dir: build_48khz_audio_waveform
```

## Limits

This branch verifies the microphone/audio chain as a 48 kHz PCM diagnostic preview. It is not a raw PDM recorder and it is not a high-rate ultrasonic measurement path.

With 48 kHz PCM, the Nyquist limit is 24 kHz. In practice, waveform shape becomes visually coarse at high audio frequencies because each period has few samples. For example, 10 kHz has only 4.8 samples per period at 48 kHz.

To inspect ultrasonic content, the ADAU1860 output path, nRF I2S configuration, BLE packet format, and web plotting code would need a dedicated 96 kHz or 192 kHz design.
