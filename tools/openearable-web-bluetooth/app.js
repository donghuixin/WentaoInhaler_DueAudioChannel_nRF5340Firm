const UUIDS = {
  sensorService: '34c2e3bb-34aa-11eb-adc1-0242ac120002',
  sensorConfig: '34c2e3be-34aa-11eb-adc1-0242ac120002',
  sensorThermalData: '34c2e3c1-34aa-11eb-adc1-0242ac120002',
  audioWaveformService: '1410dfa0-5f68-4ebb-a7c7-5e0fb9ae7557',
  audioWaveformControl: '1410dfa1-5f68-4ebb-a7c7-5e0fb9ae7557',
  audioWaveformData: '1410dfa2-5f68-4ebb-a7c7-5e0fb9ae7557',
  batteryService: 0x180f,
  batteryLevel: 0x2a19
};

const IMU_SENSOR_ID = 0;
const IMU_SAMPLE_RATE_INDEX = 2;
const THERMAL_SENSOR_ID = 8;
const THERMAL_SAMPLE_RATE_INDEX = 2;
const MICROPHONE_SENSOR_ID = 2;
const MICROPHONE_SAMPLE_RATE_INDEX = 0;
const STORAGE_STREAMING = 0x01;

const AUDIO_WAVE_CONTROL_ENABLE = 0x01;
const AUDIO_WAVE_CONTROL_RESET = 0x02;

const SENSOR_STREAM_HEADER_SIZE = 16;
const SENSOR_STREAM_TYPE_THERMAL = 1;
const SENSOR_STREAM_TYPE_IMU_BATCH = 2;
const THERMAL_NUM_COLS = 32;
const THERMAL_NUM_ROWS = 24;
const THERMAL_NUM_PIXELS = THERMAL_NUM_COLS * THERMAL_NUM_ROWS;
const THERMAL_BLE_PACKETS_PER_FRAME = 7;
const IMU_STREAM_SAMPLE_SIZE = 4 + 36;
const TDM_MIC_OFFSET = 4;
const AUDIO_PCM_SAMPLE_RATE_HZ = 16000;
const IMU_HISTORY_LIMIT = 300;
const IMU_AXIS_COLORS = ['#00d4ff', '#5bc0ff', '#9b7bff', '#ff5f8a', '#ff8f5f', '#ffd166', '#06d6a0', '#72efdd', '#a7f432'];
const VIDEO_MIME_CANDIDATES = ['video/mp4;codecs=avc1.42E01E', 'video/mp4'];

const els = {
  linkState: document.querySelector('#linkState'),
  deviceName: document.querySelector('#deviceName'),
  batteryLevel: document.querySelector('#batteryLevel'),
  refreshBatteryBtn: document.querySelector('#refreshBatteryBtn'),
  connectNamedBtn: document.querySelector('#connectNamedBtn'),
  connectAnyBtn: document.querySelector('#connectAnyBtn'),
  disconnectBtn: document.querySelector('#disconnectBtn'),
  filePrefix: document.querySelector('#filePrefix'),
  startBtn: document.querySelector('#startBtn'),
  stopBtn: document.querySelector('#stopBtn'),
  saveBtn: document.querySelector('#saveBtn'),
  statusText: document.querySelector('#statusText'),
  log: document.querySelector('#log'),
  webcamPreview: document.querySelector('#webcamPreview'),
  thermalCanvas: document.querySelector('#thermalCanvas'),
  thermalInfo: document.querySelector('#thermalInfo'),
  imuInfo: document.querySelector('#imuInfo'),
  imuCanvas: document.querySelector('#imuCanvas'),
  audioCanvas: document.querySelector('#audioCanvas'),
  audioInfo: document.querySelector('#audioInfo')
};

let device = null;
let server = null;
let sensorConfigChar = null;
let sensorStreamChar = null;
let audioControlChar = null;
let audioDataChar = null;
let batteryLevelChar = null;
let sensorStreamNotifying = false;
let audioNotifying = false;
let isCollecting = false;

const buffers = {
  startMs: null,
  stopMs: null,
  imu: [],
  thermal: [],
  audio: [],
  video: {
    chunks: [],
    startMs: null,
    endMs: null,
    mimeType: ''
  }
};

const thermalState = {
  frameRaw: new Int16Array(THERMAL_NUM_PIXELS),
  chunkReceived: new Uint8Array(THERMAL_BLE_PACKETS_PER_FRAME),
  chunkCount: 0,
  frameSequence: null,
  frameTimestampUs: null
};

let audioTimestamp = {
  lastRawUs: null,
  lastUnwrappedUs: null
};

const imuWaveState = {
  series: []
};

let webcamStream = null;
let mediaRecorder = null;
let videoRecordingActive = false;
let webcamStopPromise = null;

function log(message) {
  const stamp = new Date().toLocaleTimeString();
  els.log.textContent = `[${stamp}] ${message}\n${els.log.textContent}`;
}

function setStatus(text) {
  els.statusText.textContent = text;
}

function setLinkState(text) {
  if (els.linkState) {
    els.linkState.textContent = text;
  }
}

function sanitizePrefix(text) {
  const value = String(text || '').trim().replace(/[^a-zA-Z0-9_-]/g, '_').slice(0, 40);
  return value || 'inhaler';
}

function resetBuffers() {
  buffers.startMs = null;
  buffers.stopMs = null;
  buffers.imu = [];
  buffers.thermal = [];
  buffers.audio = [];
  buffers.video = {
    chunks: [],
    startMs: null,
    endMs: null,
    mimeType: ''
  };
}

function updateButtons() {
  els.startBtn.disabled = isCollecting;
  els.stopBtn.disabled = !isCollecting;
  const hasData = buffers.imu.length > 0 ||
    buffers.thermal.length > 0 ||
    buffers.audio.length > 0 ||
    buffers.video.chunks.length > 0;
  els.saveBtn.disabled = !hasData;
}

function readUint64Le(view, offset) {
  if (view.getBigUint64) {
    return Number(view.getBigUint64(offset, true));
  }
  const lo = view.getUint32(offset, true);
  const hi = view.getUint32(offset + 4, true);
  return hi * 4294967296 + lo;
}

function formatTimestampMs(valueMs) {
  if (!Number.isFinite(valueMs)) return '';
  return valueMs.toFixed(3);
}

function sensorClockAnchor() {
  if (buffers.thermal.length > 0) {
    return {
      deviceUs: buffers.thermal[0].timestamp_us,
      wallMs: buffers.thermal[0].timestamp_ms
    };
  }
  return null;
}

function alignedSensorTimestampMs(item) {
  const anchor = sensorClockAnchor();
  if (!anchor || !Number.isFinite(item.timestamp_us)) {
    return item.timestamp_ms;
  }
  return anchor.wallMs + (item.timestamp_us - anchor.deviceUs) / 1000;
}

function thermalColor(v, min, max) {
  const u = Math.max(0, Math.min(1, (v - min) / Math.max(1e-6, max - min)));
  const r = Math.round(255 * Math.min(1, 1.5 * u));
  const g = Math.round(255 * Math.max(0, Math.min(1, 1.5 * u - 0.5)));
  const b = Math.round(255 * (u < 0.33 ? 1.5 * u : u < 0.66 ? 1 - 2.5 * (u - 0.33) : Math.max(0, 0.5 * (u - 0.66) / 0.34)));
  return [r, g, b];
}

function renderThermalFrame() {
  const canvas = els.thermalCanvas;
  const ctx = canvas.getContext('2d');
  if (!ctx) return;

  let minV = Infinity;
  let maxV = -Infinity;
  for (let i = 0; i < THERMAL_NUM_PIXELS; i++) {
    const v = thermalState.frameRaw[i] / 50;
    if (v < minV) minV = v;
    if (v > maxV) maxV = v;
  }
  if (!Number.isFinite(minV) || !Number.isFinite(maxV)) return;

  const small = document.createElement('canvas');
  small.width = THERMAL_NUM_COLS;
  small.height = THERMAL_NUM_ROWS;
  const sctx = small.getContext('2d');
  if (!sctx) return;
  const image = sctx.createImageData(THERMAL_NUM_COLS, THERMAL_NUM_ROWS);
  for (let row = 0; row < THERMAL_NUM_ROWS; row++) {
    for (let col = 0; col < THERMAL_NUM_COLS; col++) {
      const idx = row * THERMAL_NUM_COLS + col;
      const c = thermalColor(thermalState.frameRaw[idx] / 50, minV, maxV);
      const p = idx * 4;
      image.data[p] = c[0];
      image.data[p + 1] = c[1];
      image.data[p + 2] = c[2];
      image.data[p + 3] = 255;
    }
  }
  sctx.putImageData(image, 0, 0);
  ctx.imageSmoothingEnabled = false;
  ctx.drawImage(small, 0, 0, canvas.width, canvas.height);
  els.thermalInfo.textContent = `Frame ${thermalState.frameSequence ?? '-'} | ${minV.toFixed(1)}..${maxV.toFixed(1)} C`;
}

function renderAudioWave(samples) {
  const canvas = els.audioCanvas;
  const ctx = canvas.getContext('2d');
  if (!ctx) return;
  const w = canvas.width;
  const h = canvas.height;
  ctx.clearRect(0, 0, w, h);
  ctx.fillStyle = '#080c11';
  ctx.fillRect(0, 0, w, h);
  if (!samples || samples.length === 0) return;
  let maxAbs = 1;
  for (const s of samples) maxAbs = Math.max(maxAbs, Math.abs(s));
  ctx.strokeStyle = '#36d9a6';
  ctx.lineWidth = 2;
  ctx.beginPath();
  for (let i = 0; i < samples.length; i++) {
    const x = (i / (samples.length - 1)) * (w - 1);
    const y = h / 2 - (samples[i] / maxAbs) * (h * 0.42);
    if (i === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  }
  ctx.stroke();
}

function renderImuWave() {
  const canvas = els.imuCanvas;
  const ctx = canvas.getContext('2d');
  if (!ctx) return;
  const w = canvas.width;
  const h = canvas.height;
  ctx.clearRect(0, 0, w, h);
  ctx.fillStyle = '#080c11';
  ctx.fillRect(0, 0, w, h);

  const samples = imuWaveState.series;
  if (samples.length < 2) return;

  let maxAbs = 0.001;
  for (const sample of samples) {
    for (let i = 0; i < 9; i++) {
      maxAbs = Math.max(maxAbs, Math.abs(sample[i]));
    }
  }
  const xDen = Math.max(1, samples.length - 1);
  for (let axis = 0; axis < 9; axis++) {
    ctx.strokeStyle = IMU_AXIS_COLORS[axis];
    ctx.lineWidth = 1.2;
    ctx.beginPath();
    for (let i = 0; i < samples.length; i++) {
      const x = (i / xDen) * (w - 1);
      const y = h / 2 - (samples[i][axis] / maxAbs) * (h * 0.46);
      if (i === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    }
    ctx.stroke();
  }
}

function resetPreviews() {
  els.thermalInfo.textContent = 'No frame';
  els.imuInfo.textContent = 'No sample';
  els.audioInfo.textContent = 'No packet';
  imuWaveState.series = [];
  const tctx = els.thermalCanvas.getContext('2d');
  if (tctx) {
    tctx.fillStyle = '#080c11';
    tctx.fillRect(0, 0, els.thermalCanvas.width, els.thermalCanvas.height);
  }
  const ictx = els.imuCanvas.getContext('2d');
  if (ictx) {
    ictx.fillStyle = '#080c11';
    ictx.fillRect(0, 0, els.imuCanvas.width, els.imuCanvas.height);
  }
  renderAudioWave([]);
}

async function refreshBatteryLevel() {
  if (!batteryLevelChar) {
    els.batteryLevel.textContent = '--';
    return;
  }
  try {
    const value = await batteryLevelChar.readValue();
    const level = value.getUint8(0);
    els.batteryLevel.textContent = `${level}%`;
    log(`Battery ${level}%`);
  } catch (error) {
    els.batteryLevel.textContent = '--';
    log(`Battery read failed: ${error.message}`);
  }
}

async function gattStartNotifications(characteristic, handler) {
  await characteristic.startNotifications();
  characteristic.addEventListener('characteristicvaluechanged', handler);
}

async function gattStopNotifications(characteristic, handler) {
  characteristic.removeEventListener('characteristicvaluechanged', handler);
  await characteristic.stopNotifications();
}

async function writeSensorConfig(sensorId, sampleRateIndex, storageOptions) {
  if (!sensorConfigChar) throw new Error('Sensor config characteristic unavailable');
  const payload = new Uint8Array([sensorId & 0xff, sampleRateIndex & 0xff, storageOptions & 0xff]);
  await sensorConfigChar.writeValue(payload);
}

function chooseVideoMimeType() {
  if (!window.MediaRecorder || typeof window.MediaRecorder.isTypeSupported !== 'function') {
    return '';
  }
  for (const mimeType of VIDEO_MIME_CANDIDATES) {
    if (MediaRecorder.isTypeSupported(mimeType)) return mimeType;
  }
  return '';
}

async function startWebcamRecording() {
  if (videoRecordingActive) return;
  if (!navigator.mediaDevices?.getUserMedia || !window.MediaRecorder) {
    throw new Error('Webcam recording is not supported in this browser');
  }
  const mimeType = chooseVideoMimeType();
  if (!mimeType) {
    throw new Error('This browser does not support MediaRecorder MP4');
  }
  webcamStream = await navigator.mediaDevices.getUserMedia({ video: true, audio: false });
  if (els.webcamPreview) {
    els.webcamPreview.srcObject = webcamStream;
    try {
      await els.webcamPreview.play();
    } catch (_) {
      // Some browsers may block play() promise; stream is still attached.
    }
  }
  const recorder = new MediaRecorder(webcamStream, { mimeType });
  const chunks = [];
  recorder.addEventListener('dataavailable', (event) => {
    if (event.data && event.data.size > 0) chunks.push(event.data);
  });
  recorder.addEventListener('stop', () => {
    buffers.video.chunks = chunks;
    if (!Number.isFinite(buffers.video.endMs)) {
      buffers.video.endMs = Date.now();
    }
    videoRecordingActive = false;
    cleanupWebcamStream();
    updateButtons();
  });
  recorder.start(200);
  mediaRecorder = recorder;
  buffers.video.startMs = Date.now();
  buffers.video.endMs = null;
  buffers.video.chunks = [];
  buffers.video.mimeType = mimeType;
  videoRecordingActive = true;
  webcamStopPromise = null;
  log('Webcam recording started');
}

function cleanupWebcamStream() {
  if (els.webcamPreview) {
    els.webcamPreview.srcObject = null;
  }
  if (webcamStream) {
    for (const track of webcamStream.getTracks()) {
      track.stop();
    }
    webcamStream = null;
  }
}

function stopWebcamRecording() {
  if (webcamStopPromise) return webcamStopPromise;
  if (!videoRecordingActive || !mediaRecorder) {
    cleanupWebcamStream();
    return Promise.resolve();
  }
  const recorder = mediaRecorder;
  mediaRecorder = null;
  buffers.video.endMs = Date.now();
  webcamStopPromise = new Promise((resolve) => {
    const done = () => {
      webcamStopPromise = null;
      resolve();
    };
    recorder.addEventListener('stop', done, { once: true });
    if (recorder.state === 'inactive') {
      done();
      return;
    }
    recorder.stop();
  });
  return webcamStopPromise;
}

async function ensureConnected() {
  if (server?.connected) return;
  await requestDeviceAndConnect({
    acceptAllDevices: true,
    optionalServices: [UUIDS.sensorService, UUIDS.audioWaveformService, UUIDS.batteryService]
  });
}

function onDisconnected() {
  log('Device disconnected');
  device = null;
  server = null;
  sensorConfigChar = null;
  sensorStreamChar = null;
  audioControlChar = null;
  audioDataChar = null;
  batteryLevelChar = null;
  isCollecting = false;
  sensorStreamNotifying = false;
  audioNotifying = false;
  setStatus('Disconnected');
  setLinkState('Disconnected');
  if (els.deviceName) els.deviceName.textContent = 'No device';
  if (els.batteryLevel) els.batteryLevel.textContent = '--';
  if (els.disconnectBtn) els.disconnectBtn.disabled = true;
  if (els.refreshBatteryBtn) els.refreshBatteryBtn.disabled = true;
  updateButtons();
}

async function requestDeviceAndConnect(options) {
  setStatus('Choosing device...');
  device = await navigator.bluetooth.requestDevice(options);
  device.addEventListener('gattserverdisconnected', onDisconnected);
  if (els.deviceName) els.deviceName.textContent = device.name || 'Unnamed device';
  setStatus('Connecting...');
  server = await device.gatt.connect();
  const sensorService = await server.getPrimaryService(UUIDS.sensorService);
  sensorConfigChar = await sensorService.getCharacteristic(UUIDS.sensorConfig);
  sensorStreamChar = await sensorService.getCharacteristic(UUIDS.sensorThermalData);
  const audioService = await server.getPrimaryService(UUIDS.audioWaveformService);
  audioControlChar = await audioService.getCharacteristic(UUIDS.audioWaveformControl);
  audioDataChar = await audioService.getCharacteristic(UUIDS.audioWaveformData);
  try {
    const batteryService = await server.getPrimaryService(UUIDS.batteryService);
    batteryLevelChar = await batteryService.getCharacteristic(UUIDS.batteryLevel);
  } catch (error) {
    batteryLevelChar = null;
    log(`Battery service unavailable: ${error.message}`);
  }
  if (els.disconnectBtn) els.disconnectBtn.disabled = false;
  if (els.refreshBatteryBtn) els.refreshBatteryBtn.disabled = !batteryLevelChar;
  setStatus('Connected');
  setLinkState('Connected');
  log(`Connected: ${device.name || 'Unnamed device'}`);
  await refreshBatteryLevel();
}

async function connectNamed() {
  try {
    await requestDeviceAndConnect({
      filters: [{ namePrefix: 'OpenEarable' }],
      optionalServices: [UUIDS.sensorService, UUIDS.audioWaveformService, UUIDS.batteryService]
    });
  } catch (error) {
    setStatus('Connect failed');
    log(`Connect failed: ${error.message}`);
  }
}

async function connectAny() {
  try {
    await requestDeviceAndConnect({
      acceptAllDevices: true,
      optionalServices: [UUIDS.sensorService, UUIDS.audioWaveformService, UUIDS.batteryService]
    });
  } catch (error) {
    setStatus('Connect failed');
    log(`Connect failed: ${error.message}`);
  }
}

function disconnectDevice() {
  if (device?.gatt?.connected) {
    device.gatt.disconnect();
  } else {
    onDisconnected();
  }
}

function handleImuSample(timestampUs, values, timestampMs) {
  const [ax, ay, az, gx, gy, gz, mx, my, mz] = values;
  imuWaveState.series.push([ax, ay, az, gx, gy, gz, mx, my, mz]);
  if (imuWaveState.series.length > IMU_HISTORY_LIMIT) {
    imuWaveState.series.splice(0, imuWaveState.series.length - IMU_HISTORY_LIMIT);
  }
  renderImuWave();
  els.imuInfo.textContent = `ts=${timestampUs}`;
  if (isCollecting) {
    buffers.imu.push({ timestamp_ms: timestampMs, timestamp_us: timestampUs, ax, ay, az, gx, gy, gz, mx, my, mz });
  }
}

function decodeImuBatchPacket(view) {
  const packetReceiveMs = Date.now();
  const baseTimestampUs = readUint64Le(view, 0);
  const sampleCount = view.getUint8(11);
  const expectedBytes = SENSOR_STREAM_HEADER_SIZE + sampleCount * IMU_STREAM_SAMPLE_SIZE;
  if (sampleCount <= 0 || view.byteLength !== expectedBytes) return;
  const lastOffset = SENSOR_STREAM_HEADER_SIZE + (sampleCount - 1) * IMU_STREAM_SAMPLE_SIZE;
  const lastSampleTimestampUs = baseTimestampUs + view.getUint32(lastOffset, true);
  for (let sampleIndex = 0; sampleIndex < sampleCount; sampleIndex++) {
    const offset = SENSOR_STREAM_HEADER_SIZE + sampleIndex * IMU_STREAM_SAMPLE_SIZE;
    const deltaUs = view.getUint32(offset, true);
    const sampleTimestampUs = baseTimestampUs + deltaUs;
    const sampleTimestampMs = packetReceiveMs - (lastSampleTimestampUs - sampleTimestampUs) / 1000;
    const values = [];
    for (let axis = 0; axis < 9; axis++) {
      values.push(view.getFloat32(offset + 4 + axis * 4, true));
    }
    handleImuSample(sampleTimestampUs, values, sampleTimestampMs);
  }
}

function decodeThermalPacket(view) {
  const packetReceiveMs = Date.now();
  const timestampUs = readUint64Le(view, 0);
  const frameSequence = view.getUint16(8, true);
  const pixelCount = view.getUint8(11);
  const pixelOffset = view.getUint16(12, true);
  const packetIndex = view.getUint8(14);
  const packetCount = view.getUint8(15);
  const expectedBytes = SENSOR_STREAM_HEADER_SIZE + pixelCount * 2;
  if (packetCount !== THERMAL_BLE_PACKETS_PER_FRAME ||
      packetIndex >= packetCount ||
      pixelCount <= 0 ||
      pixelOffset + pixelCount > THERMAL_NUM_PIXELS ||
      view.byteLength !== expectedBytes) {
    return;
  }

  if (thermalState.frameSequence !== frameSequence) {
    thermalState.chunkReceived.fill(0);
    thermalState.chunkCount = 0;
    thermalState.frameSequence = frameSequence;
    thermalState.frameTimestampUs = timestampUs;
    thermalState.frameHostMs = packetReceiveMs;
  }

  if (thermalState.chunkReceived[packetIndex]) return;
  for (let i = 0; i < pixelCount; i++) {
    thermalState.frameRaw[pixelOffset + i] = view.getInt16(SENSOR_STREAM_HEADER_SIZE + i * 2, true);
  }
  thermalState.chunkReceived[packetIndex] = 1;
  thermalState.chunkCount += 1;

  if (thermalState.chunkCount === packetCount) {
    renderThermalFrame();
    if (isCollecting) {
      buffers.thermal.push({
        timestamp_ms: thermalState.frameHostMs ?? packetReceiveMs,
        timestamp_us: thermalState.frameTimestampUs,
        frame_sequence: thermalState.frameSequence,
        pixels: new Int16Array(thermalState.frameRaw)
      });
    }
    thermalState.chunkReceived.fill(0);
    thermalState.chunkCount = 0;
  }
}

function handleSensorStream(event) {
  const value = event.target.value;
  if (!value || value.byteLength < SENSOR_STREAM_HEADER_SIZE) return;
  const packetType = value.getUint8(10);
  if (packetType === SENSOR_STREAM_TYPE_IMU_BATCH) {
    decodeImuBatchPacket(value);
  } else if (packetType === SENSOR_STREAM_TYPE_THERMAL) {
    decodeThermalPacket(value);
  }
}

function decodeTdmPacket(view) {
  const packetReceiveMs = Date.now();
  const payloadBytes = view.byteLength - TDM_MIC_OFFSET;
  if (payloadBytes <= 0 || payloadBytes % 2 !== 0) {
    return null;
  }
  const sampleCount = payloadBytes / 2;
  const packetPeriodUs = Math.round((sampleCount / AUDIO_PCM_SAMPLE_RATE_HZ) * 1000000);
  const rawUs = view.getUint32(0, true);
  let unwrappedUs = rawUs;
  let missingSamples = 0;
  if (audioTimestamp.lastRawUs != null && audioTimestamp.lastUnwrappedUs != null) {
    const deltaUs = (rawUs - audioTimestamp.lastRawUs) >>> 0;
    unwrappedUs = audioTimestamp.lastUnwrappedUs + deltaUs;
    if (deltaUs > packetPeriodUs) {
      missingSamples = Math.max(0, Math.round((deltaUs - packetPeriodUs) / (1000000 / AUDIO_PCM_SAMPLE_RATE_HZ)));
    }
  }
  audioTimestamp.lastRawUs = rawUs;
  audioTimestamp.lastUnwrappedUs = unwrappedUs;
  const samples = new Int16Array(sampleCount);
  for (let i = 0; i < sampleCount; i++) {
    samples[i] = view.getInt16(TDM_MIC_OFFSET + i * 2, true);
  }
  const timestampMs = packetReceiveMs - (sampleCount / AUDIO_PCM_SAMPLE_RATE_HZ) * 1000;
  return { timestamp_ms: timestampMs, timestamp_us: unwrappedUs, raw_timestamp_u32_us: rawUs, missing_samples: missingSamples, samples };
}

function handleAudioData(event) {
  const value = event.target.value;
  if (!value || value.byteLength <= TDM_MIC_OFFSET) return;
  const decoded = decodeTdmPacket(value);
  if (!decoded) return;
  renderAudioWave(decoded.samples);
  els.audioInfo.textContent = `ts=${decoded.timestamp_us}us, missing=${decoded.missing_samples}`;
  if (isCollecting) {
    buffers.audio.push(decoded);
  }
}

async function startCollection() {
  try {
    await ensureConnected();
    resetBuffers();
    resetPreviews();
    buffers.startMs = Date.now();
    audioTimestamp = { lastRawUs: null, lastUnwrappedUs: null };
    await startWebcamRecording();

    if (!sensorStreamNotifying) {
      await gattStartNotifications(sensorStreamChar, handleSensorStream);
      sensorStreamNotifying = true;
    }
    if (!audioNotifying) {
      await gattStartNotifications(audioDataChar, handleAudioData);
      audioNotifying = true;
    }

    // Enable waveform preview stream first, then start microphone sensor streaming.
    await audioControlChar.writeValue(new Uint8Array([AUDIO_WAVE_CONTROL_ENABLE | AUDIO_WAVE_CONTROL_RESET]));
    await writeSensorConfig(IMU_SENSOR_ID, IMU_SAMPLE_RATE_INDEX, STORAGE_STREAMING);
    await writeSensorConfig(THERMAL_SENSOR_ID, THERMAL_SAMPLE_RATE_INDEX, STORAGE_STREAMING);
    await writeSensorConfig(
      MICROPHONE_SENSOR_ID,
      MICROPHONE_SAMPLE_RATE_INDEX,
      STORAGE_STREAMING
    );

    isCollecting = true;
    setStatus('Collecting...');
    updateButtons();
    log('Collection started');
  } catch (error) {
    log(`Start failed: ${error.message}`);
    setStatus('Start failed');
    await stopWebcamRecording();
    isCollecting = false;
    updateButtons();
  }
}

async function stopCollection() {
  isCollecting = false;
  buffers.stopMs = Date.now();
  const webcamStop = stopWebcamRecording();
  try {
    if (!server?.connected) return;
    await writeSensorConfig(IMU_SENSOR_ID, IMU_SAMPLE_RATE_INDEX, 0);
    await writeSensorConfig(THERMAL_SENSOR_ID, THERMAL_SAMPLE_RATE_INDEX, 0);
    await writeSensorConfig(MICROPHONE_SENSOR_ID, MICROPHONE_SAMPLE_RATE_INDEX, 0);
    await audioControlChar.writeValue(new Uint8Array([0x00]));

    if (sensorStreamNotifying) {
      await gattStopNotifications(sensorStreamChar, handleSensorStream);
      sensorStreamNotifying = false;
    }
    if (audioNotifying) {
      await gattStopNotifications(audioDataChar, handleAudioData);
      audioNotifying = false;
    }
  } catch (error) {
    log(`Stop warning: ${error.message}`);
  } finally {
    await webcamStop;
    setStatus('Stopped');
    updateButtons();
    log('Collection stopped');
  }
}

function buildCsv() {
  const rows = [];
  for (const item of buffers.imu) {
    const timestampMs = alignedSensorTimestampMs(item);
    rows.push({
      timestamp_ms: timestampMs,
      csv: `${formatTimestampMs(timestampMs)},IMU,9_axis_float32;rate_hz=100;device_us=${item.timestamp_us},${item.ax},${item.ay},${item.az},${item.gx},${item.gy},${item.gz},${item.mx},${item.my},${item.mz}`
    });
  }
  for (const item of buffers.audio) {
    const timestampMs = item.timestamp_ms;
    rows.push({
      timestamp_ms: timestampMs,
      csv: `${formatTimestampMs(timestampMs)},AUDIO,pcm16_16k_120_samples;raw_u32_us=${item.raw_timestamp_u32_us};missing_samples=${item.missing_samples},${Array.from(item.samples).join(',')}`
    });
  }
  for (const item of buffers.thermal) {
    const timestampMs = alignedSensorTimestampMs(item);
    const fmt = `raw_int16_frame_32x24;frame_sequence=${item.frame_sequence};rate_hz=8;pixel_count=${THRM_PIXEL_COUNT};device_us=${item.timestamp_us}`;
    rows.push({
      timestamp_ms: timestampMs,
      csv: `${formatTimestampMs(timestampMs)},IR,${fmt},${Array.from(item.pixels).join(',')}`
    });
  }
  rows.sort((a, b) => a.timestamp_ms - b.timestamp_ms);
  const lines = rows.map((r) => r.csv);
  if (Number.isFinite(buffers.video.startMs) && Number.isFinite(buffers.video.endMs)) {
    lines.push(`${formatTimestampMs(buffers.video.startMs)},VIDEO,start`);
    lines.push(`${formatTimestampMs(buffers.video.endMs)},VIDEO,end`);
  }
  return `timestamp_ms,type,format,data...\n${lines.join('\n')}\n`;
}

function downloadTextFile(filename, text) {
  const blob = new Blob([text], { type: 'text/csv;charset=utf-8' });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = filename;
  document.body.appendChild(a);
  a.click();
  a.remove();
  URL.revokeObjectURL(url);
}

function downloadBlob(filename, blob) {
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = filename;
  document.body.appendChild(a);
  a.click();
  a.remove();
  URL.revokeObjectURL(url);
}

function saveCollection() {
  if (
    buffers.imu.length === 0 &&
    buffers.thermal.length === 0 &&
    buffers.audio.length === 0 &&
    buffers.video.chunks.length === 0
  ) {
    log('Save skipped: no data');
    return;
  }
  const prefix = sanitizePrefix(els.filePrefix.value);
  const filename = `${prefix}.csv`;
  const csv = buildCsv();
  downloadTextFile(filename, csv);
  if (buffers.video.chunks.length > 0) {
    const videoBlob = new Blob(buffers.video.chunks, { type: buffers.video.mimeType || 'video/mp4' });
    downloadBlob(`${prefix}.mp4`, videoBlob);
  }
  setStatus(`Saved ${filename}`);
  log(`Saved ${filename}`);
}

const THRM_PIXEL_COUNT = THERMAL_NUM_PIXELS;

els.startBtn.addEventListener('click', () => {
  startCollection();
});
els.stopBtn.addEventListener('click', () => {
  stopCollection();
});
els.saveBtn.addEventListener('click', () => {
  saveCollection();
});
if (els.connectNamedBtn) els.connectNamedBtn.addEventListener('click', connectNamed);
if (els.connectAnyBtn) els.connectAnyBtn.addEventListener('click', connectAny);
if (els.disconnectBtn) els.disconnectBtn.addEventListener('click', disconnectDevice);
if (els.refreshBatteryBtn) els.refreshBatteryBtn.addEventListener('click', refreshBatteryLevel);

resetPreviews();
updateButtons();
setLinkState('Disconnected');
setStatus('Idle - click Start');
