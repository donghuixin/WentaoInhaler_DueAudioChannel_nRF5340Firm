const UUIDS = {
  sensorService: '34c2e3bb-34aa-11eb-adc1-0242ac120002',
  sensorConfig: '34c2e3be-34aa-11eb-adc1-0242ac120002',
  sensorData: '34c2e3bc-34aa-11eb-adc1-0242ac120002',
  sensorConfigStatus: '34c2e3bf-34aa-11eb-adc1-0242ac120002',
  sensorRecordingName: '34c2e3c0-34aa-11eb-adc1-0242ac120002',
  parseInfoService: 'caa25cb7-7e1b-44f2-adc9-e8c06c9ced43',
  parseInfo: 'caa25cb9-7e1b-44f2-adc9-e8c06c9ced43',
  parseInfoRequest: 'caa25cba-7e1b-44f2-adc9-e8c06c9ced43',
  parseInfoResponse: 'caa25cbb-7e1b-44f2-adc9-e8c06c9ced43',
  ledService: '81040a2e-4819-11ee-be56-0242ac120002',
  ledRgb: '81040e7a-4819-11ee-be56-0242ac120002',
  ledState: '81040e7b-4819-11ee-be56-0242ac120002',
  buttonService: '29c10bdc-4773-11ee-be56-0242ac120002',
  buttonState: '29c10f38-4773-11ee-be56-0242ac120002',
  audioConfigService: '1410df95-5f68-4ebb-a7c7-5e0fb9ae7557',
  audioMode: '1410df96-5f68-4ebb-a7c7-5e0fb9ae7557',
  micSelect: '1410df97-5f68-4ebb-a7c7-5e0fb9ae7557',
  audioChannel: '1410df98-5f68-4ebb-a7c7-5e0fb9ae7557',
  audioWaveformService: '1410dfa0-5f68-4ebb-a7c7-5e0fb9ae7557',
  audioWaveformControl: '1410dfa1-5f68-4ebb-a7c7-5e0fb9ae7557',
  audioWaveformData: '1410dfa2-5f68-4ebb-a7c7-5e0fb9ae7557',
  deviceInfoService: '45622510-6468-465a-b141-0b9b0f96b468',
  identifier: '45622511-6468-465a-b141-0b9b0f96b468',
  generation: '45622512-6468-465a-b141-0b9b0f96b468',
  firmware: '45622513-6468-465a-b141-0b9b0f96b468',
  hardwareStatus: '45622514-6468-465a-b141-0b9b0f96b468',
  smpDfuService: '8d53dc1d-1db7-4cd3-868b-8a527460aa84'
};

const ADVERTISED_UUIDS = {
  volumeControl: '00001844-0000-1000-8000-00805f9b34fb',
  mediaControl: '00001848-0000-1000-8000-00805f9b34fb',
  commonAudio: '00001853-0000-1000-8000-00805f9b34fb',
  audioStreamControl: '0000184e-0000-1000-8000-00805f9b34fb',
  openEarableAdvertised: '0000fe58-0000-1000-8000-00805f9b34fb'
};

const KNOWN_SERVICES = [
  { uuid: UUIDS.sensorService, name: 'Sensor Service' },
  { uuid: UUIDS.parseInfoService, name: 'Parse Info Service' },
  { uuid: UUIDS.deviceInfoService, name: 'OpenEarable Device Info' },
  { uuid: UUIDS.buttonService, name: 'Button Service' },
  { uuid: UUIDS.audioConfigService, name: 'Audio Config Service' },
  { uuid: UUIDS.audioWaveformService, name: 'Audio Waveform Service' },
  { uuid: UUIDS.ledService, name: 'LED Service' },
  { uuid: 'battery_service', name: 'Battery Service' },
  { uuid: UUIDS.smpDfuService, name: 'MCUmgr SMP DFU' }
];

const KNOWN_CHARS = new Map([
  [UUIDS.sensorConfig, 'Sensor Config'],
  [UUIDS.sensorData, 'Sensor Data'],
  [UUIDS.sensorConfigStatus, 'Sensor Config Status'],
  [UUIDS.sensorRecordingName, 'Recording Name'],
  [UUIDS.parseInfo, 'Parse Info'],
  [UUIDS.parseInfoRequest, 'Parse Info Request'],
  [UUIDS.parseInfoResponse, 'Parse Info Response'],
  [UUIDS.identifier, 'Identifier'],
  [UUIDS.generation, 'Generation'],
  [UUIDS.firmware, 'Firmware'],
  [UUIDS.hardwareStatus, 'Hardware Status'],
  [UUIDS.buttonState, 'Button State'],
  [UUIDS.audioMode, 'Audio Mode'],
  [UUIDS.micSelect, 'Mic Select'],
  [UUIDS.audioChannel, 'Audio Channel'],
  [UUIDS.audioWaveformControl, 'Audio Waveform Control'],
  [UUIDS.audioWaveformData, 'Audio Waveform Data'],
  [UUIDS.ledRgb, 'LED RGB'],
  [UUIDS.ledState, 'LED State'],
  ['00002a19-0000-1000-8000-00805f9b34fb', 'Battery Level']
]);

const SENSOR_NAMES = new Map([
  [0, 'IMU'],
  [1, 'Temp/Baro'],
  [2, 'Microphone'],
  [4, 'PPG'],
  [5, 'PulseOx'],
  [6, 'Optical Temp'],
  [7, 'Bone Conduct.'],
  [8, 'Thermal IR']
]);

const IMU_SENSOR_ID = 0;
const IMU_SAMPLE_RATE_INDEX = 2;
const MICROPHONE_SENSOR_ID = 2;
const MICROPHONE_SAMPLE_RATE_INDEX = 0;
const STORAGE_STREAMING = 0x01;
const STORAGE_DATA_STORAGE = 0x02;
const STORAGE_AUDIO_LEFT = 0x10;
const STORAGE_AUDIO_RIGHT = 0x20;
const AUDIO_WAVE_CONTROL_ENABLE = 0x01;
const AUDIO_WAVE_CONTROL_RESET = 0x02;
const AUDIO_WAVE_WINDOW_SHIFT = 2;
const AUDIO_SAMPLE_FORMAT_PCM16 = 1;
const TDM_PACKET_SIZE = 251;
const TDM_SAMPLE_RATE_HZ = 16000;
const TDM_PACKET_PERIOD_MS = 5;
const TDM_MIC_OFFSET = 6;
const TDM_MIC_BYTES = 160;
const TDM_THERMAL_LEN_OFFSET = 166;
const TDM_THERMAL_ROW_OFFSET = 167;
const TDM_THERMAL_OFFSET = 168;
const TDM_THERMAL_BYTES = 64;
const TDM_IMU_LEN_OFFSET = 232;
const TDM_IMU_OFFSET = 233;
const TDM_IMU_BYTES = 18;
const TDM_ACCEL_SCALE_MPS2_PER_LSB = 9.80665 / 1000;
const TDM_GYRO_SCALE_DPS_PER_LSB = 0.1;
const TDM_MAG_SCALE_UT_PER_LSB = 0.1;

// Thermal IR (MLX90642) constants. Must match the firmware in
// src/SensorManager/Thermal.{h,cpp} and the chunk header layout.
const THERMAL_SENSOR_ID = 8;
const THERMAL_SAMPLE_RATE_INDEX = 1; // default 4 Hz
const THERMAL_NUM_COLS = 32;
const THERMAL_NUM_ROWS = 24;
const THERMAL_NUM_PIXELS = THERMAL_NUM_COLS * THERMAL_NUM_ROWS;
let thermalPixelsPerChunk = 16;
let thermalTotalChunks = Math.ceil(THERMAL_NUM_PIXELS / thermalPixelsPerChunk);
const THERMAL_RAW_TO_C = 1 / 50; // raw int16 / 50 = degrees Celsius

const els = {
  themeToggleBtn: document.querySelector('#themeToggleBtn'),
  browserState: document.querySelector('#browserState'),
  linkState: document.querySelector('#linkState'),
  deviceName: document.querySelector('#deviceName'),
  gattState: document.querySelector('#gattState'),
  scanState: document.querySelector('#scanState'),
  notifyState: document.querySelector('#notifyState'),
  serviceCount: document.querySelector('#serviceCount'),
  lastEvent: document.querySelector('#lastEvent'),
  log: document.querySelector('#log'),
  advertisements: document.querySelector('#advertisements'),
  servicesList: document.querySelector('#servicesList'),
  scanBtn: document.querySelector('#scanBtn'),
  connectNamedBtn: document.querySelector('#connectNamedBtn'),
  connectAdvertisedBtn: document.querySelector('#connectAdvertisedBtn'),
  connectAnyBtn: document.querySelector('#connectAnyBtn'),
  disconnectBtn: document.querySelector('#disconnectBtn'),
  readBatteryBtn: document.querySelector('#readBatteryBtn'),
  clearLogBtn: document.querySelector('#clearLogBtn'),
  startImuBtn: document.querySelector('#startImuBtn'),
  stopImuBtn: document.querySelector('#stopImuBtn'),
  startAudioWaveBtn: document.querySelector('#startAudioWaveBtn'),
  stopAudioWaveBtn: document.querySelector('#stopAudioWaveBtn'),
  readAudioWaveBtn: document.querySelector('#readAudioWaveBtn'),
  enableSensorBtn: document.querySelector('#enableSensorBtn'),
  disableSensorBtn: document.querySelector('#disableSensorBtn'),
  subscribeSensorBtn: document.querySelector('#subscribeSensorBtn'),
  subscribeStatusBtn: document.querySelector('#subscribeStatusBtn'),
  sensorId: document.querySelector('#sensorId'),
  sampleRateIndex: document.querySelector('#sampleRateIndex'),
  packetCount: document.querySelector('#packetCount'),
  lastSensor: document.querySelector('#lastSensor'),
  lastPayload: document.querySelector('#lastPayload'),
  factName: document.querySelector('#factName'),
  factId: document.querySelector('#factId'),
  factBattery: document.querySelector('#factBattery'),
  factIdentifier: document.querySelector('#factIdentifier'),
  factGeneration: document.querySelector('#factGeneration'),
  factFirmware: document.querySelector('#factFirmware'),
  factAudioChannel: document.querySelector('#factAudioChannel'),
  imuTime: document.querySelector('#imuTime'),
  imuAx: document.querySelector('#imuAx'),
  imuAy: document.querySelector('#imuAy'),
  imuAz: document.querySelector('#imuAz'),
  imuGx: document.querySelector('#imuGx'),
  imuGy: document.querySelector('#imuGy'),
  imuGz: document.querySelector('#imuGz'),
  imuMx: document.querySelector('#imuMx'),
  imuMy: document.querySelector('#imuMy'),
  imuMz: document.querySelector('#imuMz'),
  audioWaveState: document.querySelector('#audioWaveState'),
  audioWindowMode: document.querySelector('#audioWindowMode'),
  audioSeq: document.querySelector('#audioSeq'),
  audioRate: document.querySelector('#audioRate'),
  audioSampleInterval: document.querySelector('#audioSampleInterval'),
  audioFrames: document.querySelector('#audioFrames'),
  audioPeak: document.querySelector('#audioPeak'),
  audioMean: document.querySelector('#audioMean'),
  audioFrequency: document.querySelector('#audioFrequency'),
  audioSamplesPerCycle: document.querySelector('#audioSamplesPerCycle'),
  audioPeakToPeak: document.querySelector('#audioPeakToPeak'),
  audioFrequencyConfidence: document.querySelector('#audioFrequencyConfidence'),
  audioPlotScale: document.querySelector('#audioPlotScale'),
  audioPacketInfo: document.querySelector('#audioPacketInfo'),
  audioWaveCanvas: document.querySelector('#audioWaveCanvas'),
  recordAudioBtn: document.querySelector('#recordAudioBtn'),
  playAudioBtn: document.querySelector('#playAudioBtn'),
  recordStatus: document.querySelector('#recordStatus'),
  playbackVolume: document.querySelector('#playbackVolume'),
  thermalCanvas: document.querySelector('#thermalCanvas'),
  thermalState: document.querySelector('#thermalState'),
  thermalRateIndex: document.querySelector('#thermalRateIndex'),
  thermalPalette: document.querySelector('#thermalPalette'),
  thermalAutoScale: document.querySelector('#thermalAutoScale'),
  thermalMinTemp: document.querySelector('#thermalMinTemp'),
  thermalMaxTemp: document.querySelector('#thermalMaxTemp'),
  startThermalBtn: document.querySelector('#startThermalBtn'),
  stopThermalBtn: document.querySelector('#stopThermalBtn'),
  thermalFrames: document.querySelector('#thermalFrames'),
  thermalFps: document.querySelector('#thermalFps'),
  thermalChunks: document.querySelector('#thermalChunks'),
  thermalMin: document.querySelector('#thermalMin'),
  thermalMax: document.querySelector('#thermalMax'),
  thermalAvg: document.querySelector('#thermalAvg'),
  thermalDropped: document.querySelector('#thermalDropped'),
  thermalDebugInfo: document.querySelector('#thermalDebugInfo'),
  thermalLastTime: document.querySelector('#thermalLastTime'),
  sdLoggerState: document.querySelector('#sdLoggerState'),
  sdLoggerEnabled: document.querySelector('#sdLoggerEnabled'),
  sdLogImu: document.querySelector('#sdLogImu'),
  sdImuRateIndex: document.querySelector('#sdImuRateIndex'),
  sdLogThermal: document.querySelector('#sdLogThermal'),
  sdThermalRateIndex: document.querySelector('#sdThermalRateIndex'),
  sdLogAudio: document.querySelector('#sdLogAudio'),
  sdAudioLeft: document.querySelector('#sdAudioLeft'),
  sdAudioRight: document.querySelector('#sdAudioRight'),
  sdAudioRateIndex: document.querySelector('#sdAudioRateIndex'),
  startSdLoggerBtn: document.querySelector('#startSdLoggerBtn'),
  stopSdLoggerBtn: document.querySelector('#stopSdLoggerBtn'),
  sdLoggerMode: document.querySelector('#sdLoggerMode'),
  sdLoggerAudioFiles: document.querySelector('#sdLoggerAudioFiles'),
  sdLoggerLastCommand: document.querySelector('#sdLoggerLastCommand'),
  readHwStatusBtn: document.querySelector('#readHwStatusBtn'),
  hwStatusTime: document.querySelector('#hwStatusTime'),
  hwStatusSummary: document.querySelector('#hwStatusSummary'),
  hwStatusTableBody: document.querySelector('#hwStatusTableBody'),
  i2cBusTableBody: document.querySelector('#i2cBusTableBody'),
  hwBootLog: document.querySelector('#hwBootLog'),
  hwRecentLog: document.querySelector('#hwRecentLog'),
  tabSdLogger: document.querySelector('#tabSdLogger'),
  tabBleLogger: document.querySelector('#tabBleLogger'),
  sdLoggerTab: document.querySelector('#sdLoggerTab'),
  bleLoggerTab: document.querySelector('#bleLoggerTab'),
  bleLoggerState: document.querySelector('#bleLoggerState'),
  bleLogImu: document.querySelector('#bleLogImu'),
  bleLogThermal: document.querySelector('#bleLogThermal'),
  bleLogAudio: document.querySelector('#bleLogAudio'),
  startBleLoggerBtn: document.querySelector('#startBleLoggerBtn'),
  stopBleLoggerBtn: document.querySelector('#stopBleLoggerBtn'),
  downloadBleLogBtn: document.querySelector('#downloadBleLogBtn'),
  bleLogDuration: document.querySelector('#bleLogDuration'),
  bleLogAudioCount: document.querySelector('#bleLogAudioCount'),
  bleLogImuCount: document.querySelector('#bleLogImuCount'),
  bleLogThermalCount: document.querySelector('#bleLogThermalCount'),
  bleLogDropped: document.querySelector('#bleLogDropped'),
  hwLogDropped: document.querySelector('#hwLogDropped')
};

let device = null;
let server = null;
let leScan = null;
let packetCount = 0;
let sensorConfigChar = null;
let sensorDataChar = null;
let sensorStatusChar = null;
let sensorRecordingNameChar = null;
let audioWaveformControlChar = null;
let audioWaveformDataChar = null;
let sensorDataNotifying = false;
let sensorStatusNotifying = false;
let audioWaveformNotifying = false;
const genericNotifyHandlers = new WeakMap();
const audioPeakHistory = [];
const AUDIO_PEAK_HISTORY_LIMIT = 180;
let lastAudioSamples = [];
let lastAudioPeak = 0;
let lastAudioDurationMs = 0;
let lastAudioPointRate = 0;
let audioWindowAssembly = null;
let lastTdmSeq = null;
let tdmDroppedPackets = 0;
let thermalTdmFrameActive = false;
let thermalTdmExpectedRow = 0;

let isRecordingAudio = false;
let recordedAudioBuffer = [];
let recordedAudioPointRate = 0;
let recordedAudioStartMs = null;
let playbackAudioContext = null;
let currentAudioSource = null;

// Web Bluetooth allows one GATT operation at a time per connection.
let gattQueue = Promise.resolve();
let bleLoggerOperation = null;

function enqueueGatt(fn) {
  const run = async () => {
    for (let attempt = 1; attempt <= 5; attempt++) {
      try {
        return await fn();
      } catch (error) {
        const busy = /already in progress|gatt operation/i.test(error.message);
        if (busy && attempt < 5) {
          await new Promise((resolve) => setTimeout(resolve, 60 * attempt));
          continue;
        }
        throw error;
      }
    }
    return undefined;
  };
  const next = gattQueue.then(run, run);
  gattQueue = next.catch(() => {});
  return next;
}

async function gattWriteValue(characteristic, value) {
  if (!characteristic) {
    throw new Error('Characteristic unavailable');
  }
  await enqueueGatt(() => characteristic.writeValue(value));
}

async function gattReadValue(characteristic) {
  if (!characteristic) {
    throw new Error('Characteristic unavailable');
  }
  return enqueueGatt(() => characteristic.readValue());
}

async function gattStartNotifications(characteristic, handler) {
  await enqueueGatt(async () => {
    await characteristic.startNotifications();
    characteristic.addEventListener('characteristicvaluechanged', handler);
  });
}

async function gattStopNotifications(characteristic, handler) {
  await enqueueGatt(async () => {
    await characteristic.stopNotifications();
    characteristic.removeEventListener('characteristicvaluechanged', handler);
  });
}

function ensureRecordedAudioCapacity(sampleIndex) {
  while (recordedAudioBuffer.length <= sampleIndex) {
    recordedAudioBuffer.push(0);
  }
}

function appendRecordedTdmSamples(timestampMs, micSamples, droppedNow) {
  if (recordedAudioStartMs === null) {
    recordedAudioStartMs = timestampMs;
    recordedAudioPointRate = TDM_SAMPLE_RATE_HZ;
  }

  const samplesPerPacket = TDM_MIC_BYTES / 2;
  if (droppedNow > 0) {
    const silenceStart = Math.max(
      0,
      Math.round(((timestampMs - recordedAudioStartMs) - droppedNow * TDM_PACKET_PERIOD_MS) * TDM_SAMPLE_RATE_HZ / 1000)
    );
    ensureRecordedAudioCapacity(silenceStart + droppedNow * samplesPerPacket - 1);
    for (let s = 0; s < droppedNow * samplesPerPacket; s++) {
      recordedAudioBuffer[silenceStart + s] = 0;
    }
  }

  const sampleOffset = Math.max(
    0,
    Math.round((timestampMs - recordedAudioStartMs) * TDM_SAMPLE_RATE_HZ / 1000)
  );
  ensureRecordedAudioCapacity(sampleOffset + micSamples.length - 1);
  for (let i = 0; i < micSamples.length; i++) {
    recordedAudioBuffer[sampleOffset + i] = micSamples[i];
  }
}

// BLE Data Logger state
let isBleLogging = false;
let bleLogStartTime = 0;
let bleLogAudioBuffer = [];   // { timestamp_ms, samples: Int16Array }
let bleLogImuBuffer = [];     // { timestamp_ms, ax, ay, az, gx, gy, gz, mx, my, mz }
let bleLogThermalBuffer = []; // { timestamp_ms, row, pixels: Int16Array }
let bleLogDroppedCount = 0;

// Thermal IR streaming state. The frame buffer holds raw int16 values from
// the MLX90642 (raw / 50 = degrees Celsius); we render the most recent
// fully-assembled frame to the canvas.
const thermalFrameRaw = new Int16Array(THERMAL_NUM_PIXELS);
const thermalChunkReceived = new Uint8Array(thermalTotalChunks);
let thermalCurrentFrameTime = null;
let thermalFramesRendered = 0;
let thermalDroppedFrames = 0;
let thermalChunksThisFrame = 0;
let thermalLastFpsSampleAt = 0;
let thermalFramesAtLastSample = 0;
let thermalNotifyEnabled = false;

function setStatus(text, state = 'neutral') {
  els.linkState.textContent = text;
  els.linkState.className = `status-pill ${state}`;
}

function log(message, data) {
  const stamp = new Date().toLocaleTimeString();
  const suffix = data === undefined ? '' : ` ${JSON.stringify(data)}`;
  els.log.textContent = `[${stamp}] ${message}${suffix}\n${els.log.textContent}`;
  els.lastEvent.textContent = message;
}

function normalizeUuid(uuid) {
  return typeof uuid === 'string' ? uuid.toLowerCase() : uuid;
}

function bytesToHex(view) {
  const bytes = view instanceof DataView ? new Uint8Array(view.buffer, view.byteOffset, view.byteLength) : new Uint8Array(view);
  return [...bytes].map((byte) => byte.toString(16).padStart(2, '0')).join(' ');
}

function bytesToText(view) {
  const bytes = view instanceof DataView ? new Uint8Array(view.buffer, view.byteOffset, view.byteLength) : new Uint8Array(view);
  return new TextDecoder().decode(bytes).replace(/\0+$/g, '').trim();
}

function readUint64Le(view, offset) {
  if (view.getBigUint64) {
    return view.getBigUint64(offset, true).toString();
  }
  const lo = view.getUint32(offset, true);
  const hi = view.getUint32(offset + 4, true);
  return (BigInt(hi) << 32n | BigInt(lo)).toString();
}

function valuePreview(view) {
  if (!view || view.byteLength === 0) {
    return 'empty';
  }
  const text = bytesToText(view);
  const printable = /^[\x20-\x7e]+$/.test(text);
  return printable && text.length > 0 ? text : bytesToHex(view);
}

function setFact(id, value) {
  els[id].textContent = value || '-';
}

function formatNumber(value, decimals = 3) {
  if (!Number.isFinite(value)) {
    return '-';
  }
  return value.toFixed(decimals);
}

function formatFrequency(value) {
  if (!Number.isFinite(value) || value <= 0) {
    return '-';
  }
  if (value >= 1000) {
    return `${(value / 1000).toFixed(2)} kHz`;
  }
  return `${Math.round(value)} Hz`;
}

function resetImuValues() {
  for (const id of ['imuTime', 'imuAx', 'imuAy', 'imuAz', 'imuGx', 'imuGy', 'imuGz', 'imuMx', 'imuMy', 'imuMz']) {
    els[id].textContent = '-';
  }
}

function resetAudioWaveformValues() {
  for (const id of [
    'audioSeq',
    'audioRate',
    'audioSampleInterval',
    'audioFrames',
    'audioPeak',
    'audioMean',
    'audioFrequency',
    'audioSamplesPerCycle',
    'audioPeakToPeak',
    'audioFrequencyConfidence',
    'audioPlotScale',
    'audioPacketInfo'
  ]) {
    els[id].textContent = '-';
  }
  audioPeakHistory.length = 0;
  lastAudioSamples = [];
  lastAudioPeak = 0;
  lastAudioDurationMs = 0;
  lastAudioPointRate = 0;
  audioWindowAssembly = null;
  lastTdmSeq = null;
  tdmDroppedPackets = 0;
  els.audioWaveState.textContent = 'Preview off';
  drawAudioWaveform([]);
}

function resetFacts() {
  for (const id of ['factName', 'factId', 'factBattery', 'factIdentifier', 'factGeneration', 'factFirmware', 'factAudioChannel']) {
    setFact(id, '-');
  }
}

function resetHardwareStatusUi(message = '尚未读取') {
  if (els.hwStatusTime) {
    els.hwStatusTime.textContent = '-';
  }
  if (els.hwStatusSummary) {
    els.hwStatusSummary.textContent = message;
  }
  if (els.hwStatusTableBody) {
    els.hwStatusTableBody.innerHTML = '<tr><td colspan="5" class="fine">无数据</td></tr>';
  }
  if (els.i2cBusTableBody) {
    els.i2cBusTableBody.innerHTML = '<tr><td colspan="4" class="fine">无数据</td></tr>';
  }
  if (els.hwBootLog) {
    els.hwBootLog.textContent = '无数据';
  }
  if (els.hwRecentLog) {
    els.hwRecentLog.textContent = '无数据';
  }
  if (els.hwLogDropped) {
    els.hwLogDropped.textContent = 'dropped: boot=0, recent=0';
  }
}

function formatBootLogEntries(entries) {
  if (!Array.isArray(entries) || entries.length === 0) {
    return '无数据';
  }
  return entries
    .map((entry) => {
      const tms = Number.isFinite(entry?.t) ? entry.t : 0;
      const secs = (tms / 1000).toFixed(3).padStart(8, ' ');
      const msg = typeof entry?.m === 'string' ? entry.m : '';
      return `[${secs}s] ${msg}`;
    })
    .join('\n');
}

function boolBadge(ok) {
  if (ok) return '<span class="hw-badge ok">OK</span>';
  return '<span class="hw-badge bad">FAIL</span>';
}

function renderHardwareStatus(status) {
  if (!status || typeof status !== 'object') {
    resetHardwareStatusUi('状态格式错误');
    return;
  }

  const i2cItems = Array.isArray(status.i2c) ? status.i2c : [];
  const devices = Array.isArray(status.devices) ? status.devices : [];
  const derived = status.derived && typeof status.derived === 'object' ? status.derived : {};

  const okCount = devices.filter((d) => d && d.ok === 1).length;
  const totalCount = devices.length;
  if (els.hwStatusSummary) {
    els.hwStatusSummary.textContent = `设备通信 ${okCount}/${totalCount} 正常`;
  }
  if (els.hwStatusTime) {
    els.hwStatusTime.textContent = new Date().toLocaleTimeString();
  }

  if (els.i2cBusTableBody) {
    els.i2cBusTableBody.innerHTML = '';
    for (const bus of i2cItems) {
      const tr = document.createElement('tr');
      const foundList = Array.isArray(bus.found) ? bus.found.join(', ') : '-';
      tr.innerHTML = `
        <td>${bus.bus ?? '-'}</td>
        <td>${boolBadge((bus.ready ?? 0) === 1)}</td>
        <td>${bus.count ?? 0}</td>
        <td><code>${foundList || '-'}</code></td>
      `;
      els.i2cBusTableBody.appendChild(tr);
    }
    if (i2cItems.length === 0) {
      els.i2cBusTableBody.innerHTML = '<tr><td colspan="4" class="fine">无总线数据</td></tr>';
    }
  }

  if (els.hwStatusTableBody) {
    els.hwStatusTableBody.innerHTML = '';
    for (const dev of devices) {
      const tr = document.createElement('tr');
      tr.innerHTML = `
        <td>${dev.name ?? '-'}</td>
        <td>${dev.bus ?? '-'}</td>
        <td><code>${dev.addr ?? '-'}</code></td>
        <td>${boolBadge((dev.ok ?? 0) === 1)}</td>
        <td class="fine">通信探测</td>
      `;
      els.hwStatusTableBody.appendChild(tr);
    }

    const derivedRows = [
      ['imu', 'IMU状态'],
      ['microphone_inner', '麦克风内侧'],
      ['microphone_outer', '麦克风外侧'],
      ['thermal_ir', '红外传感器'],
      ['fuel_gauge', '电量计'],
      ['ppg', 'PPG'],
      ['opt_temp', '光学温度']
    ];
    for (const [key, name] of derivedRows) {
      if (Object.prototype.hasOwnProperty.call(derived, key)) {
        const tr = document.createElement('tr');
        tr.innerHTML = `
          <td>${name}</td>
          <td>-</td>
          <td>-</td>
          <td>${boolBadge(Number(derived[key]) === 1)}</td>
          <td class="fine">派生状态</td>
        `;
        els.hwStatusTableBody.appendChild(tr);
      }
    }

    if (devices.length === 0 && Object.keys(derived).length === 0) {
      els.hwStatusTableBody.innerHTML = '<tr><td colspan="5" class="fine">无设备数据</td></tr>';
    }
  }

  const diag = status.diag && typeof status.diag === 'object' ? status.diag : null;
  if (els.hwBootLog) {
    els.hwBootLog.textContent = diag ? formatBootLogEntries(diag.boot) : '无数据';
  }
  if (els.hwRecentLog) {
    els.hwRecentLog.textContent = diag ? formatBootLogEntries(diag.recent) : '无数据';
  }
  if (els.hwLogDropped) {
    const db = diag && Number.isFinite(diag.dropped_boot) ? diag.dropped_boot : 0;
    const dr = diag && Number.isFinite(diag.dropped_recent) ? diag.dropped_recent : 0;
    els.hwLogDropped.textContent = `dropped: boot=${db}, recent=${dr}`;
  }
}

async function readHardwareStatus() {
  if (!server) {
    resetHardwareStatusUi('未连接');
    return null;
  }

  try {
    const service = await server.getPrimaryService(UUIDS.deviceInfoService);
    const characteristic = await service.getCharacteristic(UUIDS.hardwareStatus);
    const value = await gattReadValue(characteristic);
    const text = bytesToText(value);
    if (!text) {
      resetHardwareStatusUi('状态为空');
      return null;
    }

    const status = JSON.parse(text);
    renderHardwareStatus(status);
    return status;
  } catch (error) {
    resetHardwareStatusUi('读取失败');
    log(`硬件状态读取失败: ${error.message}`);
    return null;
  }
}

function setConnectedUi(isConnected) {
  els.disconnectBtn.disabled = !isConnected;
  els.readBatteryBtn.disabled = !isConnected;
  els.startImuBtn.disabled = !isConnected || !sensorConfigChar || !sensorDataChar;
  els.stopImuBtn.disabled = !isConnected || !sensorConfigChar;
  els.startAudioWaveBtn.disabled = !isConnected || !sensorConfigChar || !audioWaveformControlChar || !audioWaveformDataChar;
  els.stopAudioWaveBtn.disabled = !isConnected || !audioWaveformControlChar;
  els.recordAudioBtn.disabled = !isConnected || !audioWaveformControlChar;
  els.playAudioBtn.disabled = recordedAudioBuffer.length === 0;
  els.readAudioWaveBtn.disabled = !isConnected || !audioWaveformDataChar;
  els.enableSensorBtn.disabled = !isConnected || !sensorConfigChar;
  els.disableSensorBtn.disabled = !isConnected || !sensorConfigChar;
  els.subscribeSensorBtn.disabled = !isConnected || !sensorDataChar;
  els.subscribeStatusBtn.disabled = !isConnected || !sensorStatusChar;
  if (els.startThermalBtn) {
    els.startThermalBtn.disabled = !isConnected || !sensorConfigChar || !sensorDataChar;
  }
  if (els.stopThermalBtn) {
    els.stopThermalBtn.disabled = !isConnected || !sensorConfigChar;
  }
  if (els.readHwStatusBtn) {
    els.readHwStatusBtn.disabled = !isConnected;
  }
  if (els.startSdLoggerBtn) {
    els.startSdLoggerBtn.disabled = !isConnected || !sensorConfigChar;
  }
  if (els.stopSdLoggerBtn) {
    els.stopSdLoggerBtn.disabled = !isConnected || !sensorConfigChar;
  }
  if (els.startBleLoggerBtn) {
    els.startBleLoggerBtn.disabled = !isConnected || !sensorConfigChar || !audioWaveformControlChar;
  }
  if (els.stopBleLoggerBtn) {
    els.stopBleLoggerBtn.disabled = !isConnected || !isBleLogging;
  }
  if (els.downloadBleLogBtn) {
    els.downloadBleLogBtn.disabled = bleLogAudioBuffer.length === 0 && bleLogImuBuffer.length === 0 && bleLogThermalBuffer.length === 0;
  }
  els.gattState.textContent = isConnected ? 'GATT connected' : 'GATT disconnected';
}

function resetConnectionState() {
  device = null;
  server = null;
  sensorConfigChar = null;
  sensorDataChar = null;
  sensorStatusChar = null;
  sensorRecordingNameChar = null;
  audioWaveformControlChar = null;
  audioWaveformDataChar = null;
  sensorDataNotifying = false;
  sensorStatusNotifying = false;
  audioWaveformNotifying = false;
  packetCount = 0;
  els.packetCount.textContent = '0';
  els.lastSensor.textContent = '-';
  els.lastPayload.textContent = '-';
  resetImuValues();
  resetAudioWaveformValues();

  isRecordingAudio = false;
  recordedAudioBuffer = [];
  recordedAudioStartMs = null;
  isBleLogging = false;
  bleLoggerOperation = null;
  bleLogAudioBuffer = [];
  bleLogImuBuffer = [];
  bleLogThermalBuffer = [];
  bleLogDroppedCount = 0;
  if (bleLogTimer) {
    clearInterval(bleLogTimer);
    bleLogTimer = null;
  }
  els.recordAudioBtn.textContent = '⏺ Record';
  els.recordAudioBtn.disabled = true;
  els.playAudioBtn.disabled = true;
  els.recordStatus.textContent = '0.0s';

  resetThermalValues();
  els.notifyState.textContent = 'Notifications off';
  els.deviceName.textContent = 'No device';
  els.factName.textContent = '-';
  els.factId.textContent = '-';
  els.serviceCount.textContent = '0 found';
  els.servicesList.innerHTML = '';
  resetHardwareStatusUi('尚未读取');
  setConnectedUi(false);
}

async function checkSupport() {
  if (!window.isSecureContext) {
    els.browserState.textContent = 'Open this page from localhost or HTTPS.';
    setStatus('Blocked', 'danger');
    return;
  }

  if (!navigator.bluetooth) {
    els.browserState.textContent = 'Web Bluetooth is not available in this browser.';
    setStatus('Unsupported', 'danger');
    return;
  }

  try {
    const available = await navigator.bluetooth.getAvailability();
    els.browserState.textContent = available ? 'Web Bluetooth is available.' : 'Bluetooth adapter is unavailable.';
    setStatus(available ? 'Ready' : 'No Adapter', available ? 'ok' : 'warn');
  } catch (error) {
    els.browserState.textContent = 'Web Bluetooth support detected.';
    log(`Availability check failed: ${error.message}`);
  }
}

async function connectOpenEarable() {
  await requestDevice({
    acceptAllDevices: true,
    optionalServices: KNOWN_SERVICES.map((svc) => svc.uuid)
  });
}

async function connectOpenEarableAdvertised() {
  await requestDevice({
    filters: [
      { namePrefix: 'OpenEarable' },
      { services: [ADVERTISED_UUIDS.openEarableAdvertised] },
      { services: [ADVERTISED_UUIDS.volumeControl] },
      { services: [ADVERTISED_UUIDS.mediaControl] },
      { services: [ADVERTISED_UUIDS.commonAudio] },
      { services: [ADVERTISED_UUIDS.audioStreamControl] }
    ],
    optionalServices: [
      ...KNOWN_SERVICES.map((svc) => svc.uuid),
      ...Object.values(ADVERTISED_UUIDS)
    ]
  });
}

async function connectAnyBle() {
  await requestDevice({
    acceptAllDevices: true,
    optionalServices: KNOWN_SERVICES.map((svc) => svc.uuid)
  });
}

async function requestDevice(options) {
  if (!navigator.bluetooth) {
    log('Web Bluetooth unavailable');
    return;
  }

  try {
    setStatus('Choosing', 'warn');
    log('Opening browser Bluetooth chooser');
    device = await navigator.bluetooth.requestDevice(options);
    device.addEventListener('gattserverdisconnected', handleDisconnect);
    els.deviceName.textContent = device.name || '(unnamed)';
    setFact('factName', device.name || '(unnamed)');
    setFact('factId', device.id || '-');

    setStatus('Connecting', 'warn');
    server = await device.gatt.connect();
    setStatus('Connected', 'ok');
    log('GATT connected', { name: device.name || null, id: device.id || null });

    await discoverKnownServices();
    await readDeviceSummary();
    setConnectedUi(true);
  } catch (error) {
    setStatus('Error', 'danger');
    log(`Connect failed: ${error.message}`);
  }
}

function handleDisconnect() {
  log('Device disconnected');
  setStatus('Disconnected', 'warn');
  resetConnectionState();
}

function disconnect() {
  if (device?.gatt?.connected) {
    device.gatt.disconnect();
  } else {
    handleDisconnect();
  }
}

async function discoverKnownServices() {
  els.servicesList.innerHTML = '';
  let found = 0;

  for (const svcInfo of KNOWN_SERVICES) {
    try {
      const service = await server.getPrimaryService(svcInfo.uuid);
      const card = serviceCard(svcInfo.name, service.uuid);
      const chars = await service.getCharacteristics();
      const list = document.createElement('div');
      list.className = 'char-list';

      for (const characteristic of chars) {
        cacheCharacteristic(characteristic);
        list.appendChild(characteristicRow(characteristic));
      }

      card.appendChild(list);
      els.servicesList.appendChild(card);
      found += 1;
    } catch (error) {
      log(`Service unavailable: ${svcInfo.name}`);
    }
  }

  els.serviceCount.textContent = `${found} found`;
  if (found === 0) {
    els.servicesList.innerHTML = '<div class="service-card">No known OpenEarable services were exposed to this page.</div>';
  }
}

function cacheCharacteristic(characteristic) {
  const uuid = normalizeUuid(characteristic.uuid);
  if (uuid === UUIDS.sensorConfig) {
    sensorConfigChar = characteristic;
  } else if (uuid === UUIDS.sensorData) {
    sensorDataChar = characteristic;
  } else if (uuid === UUIDS.sensorConfigStatus) {
    sensorStatusChar = characteristic;
  } else if (uuid === UUIDS.sensorRecordingName) {
    sensorRecordingNameChar = characteristic;
  } else if (uuid === UUIDS.audioWaveformControl) {
    audioWaveformControlChar = characteristic;
  } else if (uuid === UUIDS.audioWaveformData) {
    audioWaveformDataChar = characteristic;
  }
}

function serviceCard(name, uuid) {
  const card = document.createElement('div');
  card.className = 'service-card';
  const title = document.createElement('div');
  title.className = 'service-title';
  title.innerHTML = `<span>${name}</span><code class="uuid">${uuid}</code>`;
  card.appendChild(title);
  return card;
}

function characteristicRow(characteristic) {
  const uuid = normalizeUuid(characteristic.uuid);
  const row = document.createElement('div');
  row.className = 'char-row';

  const meta = document.createElement('div');
  const name = KNOWN_CHARS.get(uuid) || 'Characteristic';
  const props = characteristicProperties(characteristic).join(', ') || 'none';
  meta.innerHTML = `<div>${name}</div><code class="uuid">${uuid}</code><div class="fine">${props}</div><div class="value-line fine"></div>`;

  const actions = document.createElement('div');
  actions.className = 'char-actions';

  if (characteristic.properties.read) {
    const button = document.createElement('button');
    button.type = 'button';
    button.textContent = 'Read';
    button.addEventListener('click', async () => {
      await readCharacteristic(characteristic, row);
    });
    actions.appendChild(button);
  }

  if (characteristic.properties.notify || characteristic.properties.indicate) {
    const button = document.createElement('button');
    button.type = 'button';
    button.textContent = 'Notify';
    button.addEventListener('click', async () => {
      await toggleGenericNotify(characteristic, button, row);
    });
    actions.appendChild(button);
  }

  row.append(meta, actions);
  return row;
}

function characteristicProperties(characteristic) {
  const props = characteristic.properties;
  return [
    ['read', props.read],
    ['write', props.write],
    ['writeWithoutResponse', props.writeWithoutResponse],
    ['notify', props.notify],
    ['indicate', props.indicate],
    ['broadcast', props.broadcast],
    ['authenticatedSignedWrites', props.authenticatedSignedWrites],
    ['reliableWrite', props.reliableWrite],
    ['writableAuxiliaries', props.writableAuxiliaries]
  ].filter(([, enabled]) => enabled).map(([name]) => name);
}

async function readCharacteristic(characteristic, row) {
  try {
    const value = await gattReadValue(characteristic);
    const preview = valuePreview(value);
    row.querySelector('.value-line').textContent = preview;
    log(`Read ${KNOWN_CHARS.get(normalizeUuid(characteristic.uuid)) || characteristic.uuid}: ${preview}`);
  } catch (error) {
    log(`Read failed: ${error.message}`);
  }
}

async function toggleGenericNotify(characteristic, button, row) {
  const enabled = button.dataset.enabled === 'true';
  let handler = genericNotifyHandlers.get(characteristic);

  if (!handler) {
    handler = (event) => {
      const value = event.target.value;
      const uuid = normalizeUuid(event.target.uuid);
      const preview = uuid === UUIDS.sensorData
        ? decodeSensorPacket(value)
        : uuid === UUIDS.sensorConfigStatus
          ? decodeSensorConfigStatus(value)
          : uuid === UUIDS.audioWaveformData
            ? decodeAudioWaveformPacket(value)
          : valuePreview(value);
      row.querySelector('.value-line').textContent = preview;
      log(`Notify ${KNOWN_CHARS.get(uuid) || uuid}: ${preview}`);
    };
    genericNotifyHandlers.set(characteristic, handler);
  }

  try {
    if (enabled) {
      await gattStopNotifications(characteristic, handler);
      button.dataset.enabled = 'false';
      button.textContent = 'Notify';
    } else {
      await gattStartNotifications(characteristic, handler);
      button.dataset.enabled = 'true';
      button.textContent = 'Stop';
    }
  } catch (error) {
    log(`Notify toggle failed: ${error.message}`);
  }
}

async function readDeviceSummary() {
  await tryRead(UUIDS.deviceInfoService, UUIDS.identifier, (value) => setFact('factIdentifier', bytesToText(value)));
  await tryRead(UUIDS.deviceInfoService, UUIDS.generation, (value) => setFact('factGeneration', bytesToText(value)));
  await tryRead(UUIDS.deviceInfoService, UUIDS.firmware, (value) => setFact('factFirmware', bytesToText(value)));
  await readBatteryLevel();
  await tryRead(UUIDS.audioConfigService, UUIDS.audioChannel, (value) => {
    const channel = value.getUint8(0);
    setFact('factAudioChannel', channel === 0 ? 'Left (0)' : channel === 1 ? 'Right (1)' : String(channel));
  });
  await readHardwareStatus();
  setConnectedUi(true);
}

async function tryRead(serviceUuid, characteristicUuid, onValue) {
  try {
    const service = await server.getPrimaryService(serviceUuid);
    const characteristic = await service.getCharacteristic(characteristicUuid);
    const value = await gattReadValue(characteristic);
    onValue(value);
  } catch {
    // Optional service or characteristic is absent.
  }
}

async function readBatteryLevel() {
  if (!server) {
    log('Battery read skipped: GATT is not connected');
    return null;
  }

  try {
    const service = await server.getPrimaryService('battery_service');
    const characteristic = await service.getCharacteristic('battery_level');
    const value = await gattReadValue(characteristic);
    const level = value.getUint8(0);
    setFact('factBattery', `${level}%`);
    log(`Battery level: ${level}%`);
    return level;
  } catch (error) {
    setFact('factBattery', '-');
    log(`Battery read failed: ${error.message}`);
    return null;
  }
}

async function writeSensorConfig(enable) {
  if (!sensorConfigChar) {
    log('Sensor config characteristic unavailable');
    return;
  }

  const sensorId = Number.parseInt(els.sensorId.value, 10) & 0xff;
  const sampleRateIndex = Number.parseInt(els.sampleRateIndex.value, 10) & 0xff;
  const storageOptions = enable ? STORAGE_STREAMING : 0x00;

  await writeSensorConfigPayload(sensorId, sampleRateIndex, storageOptions);
}

async function writeSensorConfigPayload(sensorId, sampleRateIndex, storageOptions) {
  if (!sensorConfigChar) {
    log('Sensor config characteristic unavailable');
    return false;
  }

  const payload = new Uint8Array([sensorId, sampleRateIndex, storageOptions]);

  try {
    await gattWriteValue(sensorConfigChar, payload);
    const sensorName = SENSOR_NAMES.get(sensorId) || `Sensor ${sensorId}`;
    const action = storageOptions === 0 ? 'Disabled' : 'Configured';
    log(`${action} ${sensorName}`, { sensorId, sampleRateIndex, storageOptions, hex: bytesToHex(payload) });
    return true;
  } catch (error) {
    log(`Sensor config write failed: ${error.message}`);
    return false;
  }
}

async function toggleSensorDataNotify() {
  if (!sensorDataChar) {
    log('Sensor data characteristic unavailable');
    return;
  }

  try {
    await setSensorDataNotify(!sensorDataNotifying);
  } catch (error) {
    log(`Sensor notify failed: ${error.message}`);
  }
}

async function setSensorDataNotify(enable) {
  if (!sensorDataChar) {
    log('Sensor data characteristic unavailable');
    return false;
  }

  if (enable && !sensorDataNotifying) {
    await gattStartNotifications(sensorDataChar, handleSensorData);
    sensorDataNotifying = true;
    els.subscribeSensorBtn.textContent = 'Stop Data';
    els.notifyState.textContent = 'Data notifications on';
    log('Sensor data notifications enabled');
  } else if (!enable && sensorDataNotifying) {
    await gattStopNotifications(sensorDataChar, handleSensorData);
    sensorDataNotifying = false;
    els.subscribeSensorBtn.textContent = 'Subscribe Data';
    els.notifyState.textContent = sensorStatusNotifying ? 'Status notifications on' : 'Notifications off';
    log('Sensor data notifications disabled');
  }

  return true;
}

async function startImuStream() {
  els.sensorId.value = String(IMU_SENSOR_ID);
  els.sampleRateIndex.value = String(IMU_SAMPLE_RATE_INDEX);

  await ensureTdmNotifications({ reset: lastTdmSeq == null });
  await setSensorDataNotify(true);

  const written = await writeSensorConfigPayload(IMU_SENSOR_ID, IMU_SAMPLE_RATE_INDEX, STORAGE_STREAMING);
  if (written) {
    log('IMU stream requested. TDM packets will carry accel, gyro, and magnetometer samples in the 18-byte slot.');
  }
}

async function stopImuStream() {
  els.sensorId.value = String(IMU_SENSOR_ID);
  els.sampleRateIndex.value = String(IMU_SAMPLE_RATE_INDEX);
  await writeSensorConfigPayload(IMU_SENSOR_ID, IMU_SAMPLE_RATE_INDEX, 0x00);
}

function selectedAudioWindowMode() {
  const parsed = Number.parseInt(els.audioWindowMode?.value ?? '2', 10);
  return Number.isFinite(parsed) ? Math.max(0, Math.min(3, parsed)) : 2;
}

function audioWaveformControlValue({ reset = true } = {}) {
  const mode = selectedAudioWindowMode();
  return AUDIO_WAVE_CONTROL_ENABLE |
    (reset ? AUDIO_WAVE_CONTROL_RESET : 0) |
    (mode << AUDIO_WAVE_WINDOW_SHIFT);
}

async function ensureTdmNotifications({ reset = false } = {}) {
  if (!audioWaveformControlChar || !audioWaveformDataChar) {
    return false;
  }

  if (!audioWaveformNotifying) {
    await gattStartNotifications(audioWaveformDataChar, handleAudioWaveformData);
    audioWaveformNotifying = true;
  }

  await gattWriteValue(audioWaveformControlChar, new Uint8Array([audioWaveformControlValue({ reset })]));
  return true;
}

async function updateAudioWaveformWindow() {
  if (!audioWaveformControlChar || !audioWaveformNotifying) {
    return;
  }

  audioWindowAssembly = null;
  await gattWriteValue(audioWaveformControlChar, new Uint8Array([audioWaveformControlValue({ reset: true })]));
  log(`Audio waveform window changed to ${els.audioWindowMode.value}`);
}

async function startAudioWaveform() {
  if (!sensorConfigChar || !audioWaveformControlChar || !audioWaveformDataChar) {
    log('Audio waveform channel unavailable');
    return;
  }

  try {
    audioWindowAssembly = null;
    await ensureTdmNotifications({ reset: true });
    await writeSensorConfigPayload(MICROPHONE_SENSOR_ID, MICROPHONE_SAMPLE_RATE_INDEX, STORAGE_STREAMING);

    els.audioWaveState.textContent = 'TDM stream on';
    els.startAudioWaveBtn.disabled = true;
    els.stopAudioWaveBtn.disabled = false;
    log(`16 kHz TDM audio stream requested, packet=${TDM_PACKET_SIZE} B`);
  } catch (error) {
    log(`Audio waveform start failed: ${error.message}`);
  }
}

async function stopAudioWaveform() {
  try {
    if (audioWaveformControlChar) {
      await gattWriteValue(audioWaveformControlChar, new Uint8Array([0x00]));
    }

    if (sensorConfigChar) {
      await writeSensorConfigPayload(MICROPHONE_SENSOR_ID, MICROPHONE_SAMPLE_RATE_INDEX, 0x00);
    }

    if (audioWaveformDataChar && audioWaveformNotifying) {
      await gattStopNotifications(audioWaveformDataChar, handleAudioWaveformData);
      audioWaveformNotifying = false;
    }

    els.audioWaveState.textContent = 'Preview off';
    audioWindowAssembly = null;
    setConnectedUi(Boolean(server?.connected));
    log('Audio waveform preview stopped');
  } catch (error) {
    log(`Audio waveform stop failed: ${error.message}`);
  }
}

async function readAudioWaveform() {
  if (!audioWaveformDataChar) {
    log('Audio waveform data characteristic unavailable');
    return;
  }

  try {
    const value = await gattReadValue(audioWaveformDataChar);
    const preview = decodeAudioWaveformPacket(value);
    log(`Audio waveform read: ${preview}`);
  } catch (error) {
    log(`Audio waveform read failed: ${error.message}`);
  }
}

function handleAudioWaveformData(event) {
  const preview = decodeAudioWaveformPacket(event.target.value);
  const seq = Number.parseInt(els.audioSeq.textContent, 10);
  if (!Number.isFinite(seq) || seq < 5 || seq % 25 === 0) {
    log(`Audio waveform: ${preview}`);
  }
}

async function toggleSensorStatusNotify() {
  if (!sensorStatusChar) {
    log('Sensor status characteristic unavailable');
    return;
  }

  try {
    if (sensorStatusNotifying) {
      await gattStopNotifications(sensorStatusChar, handleSensorStatus);
      sensorStatusNotifying = false;
      els.subscribeStatusBtn.textContent = 'Subscribe Status';
      els.notifyState.textContent = sensorDataNotifying ? 'Data notifications on' : 'Notifications off';
    } else {
      await gattStartNotifications(sensorStatusChar, handleSensorStatus);
      sensorStatusNotifying = true;
      els.subscribeStatusBtn.textContent = 'Stop Status';
      els.notifyState.textContent = 'Status notifications on';
    }
  } catch (error) {
    log(`Status notify failed: ${error.message}`);
  }
}

function handleSensorData(event) {
  const preview = decodeSensorPacket(event.target.value);
  if (packetCount <= 5 || packetCount % 25 === 0) {
    log(`Sensor data: ${preview}`);
  }
}

function handleSensorStatus(event) {
  log(`Sensor status: ${decodeSensorConfigStatus(event.target.value)}`);
}

function decodeSensorPacket(value) {
  packetCount += 1;
  els.packetCount.textContent = String(packetCount);

  if (value.byteLength < 10) {
    const shortValue = bytesToHex(value);
    els.lastSensor.textContent = '?';
    els.lastPayload.textContent = `${value.byteLength} B`;
    return `short packet ${shortValue}`;
  }

  const id = value.getUint8(0);
  const size = value.getUint8(1);
  const time = readUint64Le(value, 2);
  const payloadLength = Math.max(0, Math.min(size, value.byteLength - 10));
  const payload = new Uint8Array(value.buffer, value.byteOffset + 10, payloadLength);
  const hex = bytesToHex(payload);
  const sensorName = SENSOR_NAMES.get(id) || `Sensor ${id}`;
  els.lastSensor.textContent = sensorName;
  els.lastPayload.textContent = `${payload.byteLength} B`;

  if (id === IMU_SENSOR_ID) {
    const imuText = decodeImuPayload(value, payloadLength, time);
    if (imuText) {
      return `id=${id} ${sensorName} size=${size} time=${time} ${imuText}`;
    }
  }

  if (id === THERMAL_SENSOR_ID) {
    const thermalText = decodeThermalChunk(value, payloadLength, time);
    if (thermalText) {
      return `id=${id} ${sensorName} size=${size} time=${time} ${thermalText}`;
    }
  }

  return `id=${id} ${sensorName} size=${size} time=${time} payload=${hex}`;
}

function decodeImuPayload(packetView, payloadLength, time) {
  if (payloadLength < 36) {
    return null;
  }

  const values = [];
  for (let offset = 10; offset < 46; offset += 4) {
    values.push(packetView.getFloat32(offset, true));
  }

  const [ax, ay, az, gx, gy, gz, mx, my, mz] = values;
  els.imuTime.textContent = time;
  els.imuAx.textContent = formatNumber(ax);
  els.imuAy.textContent = formatNumber(ay);
  els.imuAz.textContent = formatNumber(az);
  els.imuGx.textContent = formatNumber(gx);
  els.imuGy.textContent = formatNumber(gy);
  els.imuGz.textContent = formatNumber(gz);
  els.imuMx.textContent = formatNumber(mx);
  els.imuMy.textContent = formatNumber(my);
  els.imuMz.textContent = formatNumber(mz);

  return `accel=[${formatNumber(ax)}, ${formatNumber(ay)}, ${formatNumber(az)}] gyro=[${formatNumber(gx)}, ${formatNumber(gy)}, ${formatNumber(gz)}] mag=[${formatNumber(mx)}, ${formatNumber(my)}, ${formatNumber(mz)}]`;
}

function resetThermalAssembly() {
  thermalChunkReceived.fill(0);
  thermalChunksThisFrame = 0;
}

function thermalPaletteColor(t, lo, hi, paletteName) {
  if (!Number.isFinite(t)) {
    return [0, 0, 0];
  }
  const span = hi - lo;
  let u = span > 0 ? (t - lo) / span : 0.5;
  if (u < 0) u = 0;
  else if (u > 1) u = 1;

  switch (paletteName) {
    case 'gray': {
      const g = Math.round(u * 255);
      return [g, g, g];
    }
    case 'jet': {
      // Classic jet: blue -> cyan -> green -> yellow -> red.
      const r = Math.round(255 * Math.max(0, Math.min(1, 1.5 - Math.abs(4 * u - 3))));
      const g = Math.round(255 * Math.max(0, Math.min(1, 1.5 - Math.abs(4 * u - 2))));
      const b = Math.round(255 * Math.max(0, Math.min(1, 1.5 - Math.abs(4 * u - 1))));
      return [r, g, b];
    }
    case 'iron':
    default: {
      // Iron-bow style: black -> purple -> red -> orange -> yellow -> white.
      const r = Math.round(255 * Math.min(1, 1.5 * u));
      const g = Math.round(255 * Math.max(0, Math.min(1, 1.5 * u - 0.5)));
      const b = Math.round(255 * (u < 0.33
        ? 1.5 * u
        : u < 0.66
          ? 1 - 2.5 * (u - 0.33)
          : Math.max(0, 0.5 * (u - 0.66) / 0.34)));
      return [r, g, b];
    }
  }
}

function renderThermalFrame() {
  const canvas = els.thermalCanvas;
  if (!canvas) {
    return;
  }
  const ctx = canvas.getContext('2d');
  if (!ctx) {
    return;
  }

  let lo;
  let hi;
  let minPixel = Infinity;
  let maxPixel = -Infinity;
  let sum = 0;

  // First pass to gather statistics in degrees C.
  for (let i = 0; i < THERMAL_NUM_PIXELS; i++) {
    const t = thermalFrameRaw[i] * THERMAL_RAW_TO_C;
    if (t < minPixel) minPixel = t;
    if (t > maxPixel) maxPixel = t;
    sum += t;
  }
  const avg = sum / THERMAL_NUM_PIXELS;

  if (els.thermalAutoScale?.checked) {
    lo = minPixel;
    hi = maxPixel;
    if (hi - lo < 0.5) {
      hi = lo + 0.5;
    }
  } else {
    lo = Number.parseFloat(els.thermalMinTemp?.value ?? '20');
    hi = Number.parseFloat(els.thermalMaxTemp?.value ?? '40');
    if (!Number.isFinite(lo)) lo = 20;
    if (!Number.isFinite(hi)) hi = 40;
    if (hi <= lo) hi = lo + 1;
  }

  const paletteName = els.thermalPalette?.value ?? 'iron';

  // Draw at native sensor resolution into an offscreen ImageData buffer,
  // then upscale with the canvas to fit the display.
  if (canvas.width !== THERMAL_NUM_COLS * 12) {
    canvas.width = THERMAL_NUM_COLS * 12;
    canvas.height = THERMAL_NUM_ROWS * 12;
  }
  const small = document.createElement('canvas');
  small.width = THERMAL_NUM_COLS;
  small.height = THERMAL_NUM_ROWS;
  const smallCtx = small.getContext('2d');
  const image = smallCtx.createImageData(THERMAL_NUM_COLS, THERMAL_NUM_ROWS);

  for (let row = 0; row < THERMAL_NUM_ROWS; row++) {
    for (let col = 0; col < THERMAL_NUM_COLS; col++) {
      const idx = row * THERMAL_NUM_COLS + col;
      const t = thermalFrameRaw[idx] * THERMAL_RAW_TO_C;
      const [r, g, b] = thermalPaletteColor(t, lo, hi, paletteName);
      const pix = idx * 4;
      image.data[pix] = r;
      image.data[pix + 1] = g;
      image.data[pix + 2] = b;
      image.data[pix + 3] = 255;
    }
  }
  smallCtx.putImageData(image, 0, 0);

  ctx.imageSmoothingEnabled = false;
  ctx.clearRect(0, 0, canvas.width, canvas.height);
  ctx.drawImage(small, 0, 0, canvas.width, canvas.height);

  // Update metrics + FPS.
  thermalFramesRendered += 1;
  els.thermalFrames.textContent = String(thermalFramesRendered);
  els.thermalMin.textContent = `${minPixel.toFixed(1)}`;
  els.thermalMax.textContent = `${maxPixel.toFixed(1)}`;
  els.thermalAvg.textContent = `${avg.toFixed(1)}`;
  els.thermalLastTime.textContent = thermalCurrentFrameTime != null
    ? `${(Number(thermalCurrentFrameTime) / 1e6).toFixed(2)} s`
    : '-';

  const now = performance.now();
  if (thermalLastFpsSampleAt === 0) {
    thermalLastFpsSampleAt = now;
    thermalFramesAtLastSample = thermalFramesRendered;
  } else if (now - thermalLastFpsSampleAt >= 1000) {
    const frames = thermalFramesRendered - thermalFramesAtLastSample;
    const seconds = (now - thermalLastFpsSampleAt) / 1000;
    els.thermalFps.textContent = (frames / seconds).toFixed(1);
    thermalLastFpsSampleAt = now;
    thermalFramesAtLastSample = thermalFramesRendered;
  }
}

function decodeThermalChunk(value, payloadLength, time) {
  if (payloadLength < 2) {
    return null;
  }

  const chunkIdx = value.getUint8(10);
  const count = value.getUint8(11);
  
  // Auto-detect firmware chunk size to be compatible with both old (18px) and new (16px) versions
  if (count === 18) thermalPixelsPerChunk = 18;
  else if (count === 16) thermalPixelsPerChunk = 16;
  else if (count === 12 && thermalPixelsPerChunk !== 18) thermalPixelsPerChunk = 18; // last chunk of 18px firmware
  thermalTotalChunks = Math.ceil(THERMAL_NUM_PIXELS / thermalPixelsPerChunk);

  const expectedBytes = 2 + count * 2;
  if (count === 0 || expectedBytes > payloadLength) {
    return `bad thermal chunk idx=${chunkIdx} count=${count} payload=${payloadLength}B`;
  }

  // Detect a new frame: either the timestamp moved on or the chunk index
  // wrapped back to zero. The firmware tags every chunk of one frame with
  // the same microsecond timestamp.
  if (thermalCurrentFrameTime !== time) {
    if (thermalCurrentFrameTime !== null) {
      // If the previous frame did not complete, count it as a drop.
      if (thermalChunksThisFrame < thermalTotalChunks) {
        thermalDroppedFrames += 1;
        if (els.thermalDropped) els.thermalDropped.textContent = String(thermalDroppedFrames);
      }
      
      // Render the incomplete frame instead of dropping it entirely.
      // The missing pixels will retain their previous values, which is
      // much better than a frozen screen (severe packet loss).
      if (thermalChunksThisFrame > 0) {
        renderThermalFrame();
      }
    }
    thermalCurrentFrameTime = time;
    resetThermalAssembly();
  }

  if (chunkIdx >= thermalTotalChunks) {
    return `bad thermal chunk idx=${chunkIdx} (max ${thermalTotalChunks - 1})`;
  }

  const start = chunkIdx * thermalPixelsPerChunk;
  for (let i = 0; i < count; i++) {
    const dst = start + i;
    if (dst >= THERMAL_NUM_PIXELS) break;
    // Each pixel is little-endian int16 in the BLE payload.
    thermalFrameRaw[dst] = value.getInt16(12 + i * 2, true);
  }

  if (!thermalChunkReceived[chunkIdx]) {
    thermalChunkReceived[chunkIdx] = 1;
    thermalChunksThisFrame += 1;
  }
  if (els.thermalChunks) els.thermalChunks.textContent = `${thermalChunksThisFrame}/${thermalTotalChunks}`;
  if (els.thermalDebugInfo) els.thermalDebugInfo.textContent = `Stride: ${thermalPixelsPerChunk}px`;

  if (thermalChunksThisFrame >= thermalTotalChunks) {
    renderThermalFrame();
    resetThermalAssembly();
    // Mark the current frame as "consumed" so the next packet (with a new
    // timestamp) is treated as a fresh frame, not as a dropped continuation.
    thermalCurrentFrameTime = null;
  }

  return `chunk=${chunkIdx} count=${count} progress=${thermalChunksThisFrame}/${thermalTotalChunks}`;
}

function resetThermalValues() {
  thermalFrameRaw.fill(0);
  resetThermalAssembly();
  thermalCurrentFrameTime = null;
  thermalFramesRendered = 0;
  thermalDroppedFrames = 0;
  thermalLastFpsSampleAt = 0;
  thermalFramesAtLastSample = 0;
  thermalNotifyEnabled = false;
  thermalTdmFrameActive = false;
  thermalTdmExpectedRow = 0;
  if (els.thermalFrames) els.thermalFrames.textContent = '0';
  if (els.thermalFps) els.thermalFps.textContent = '-';
  if (els.thermalChunks) els.thermalChunks.textContent = `0/${thermalTotalChunks}`;
  if (els.thermalMin) els.thermalMin.textContent = '-';
  if (els.thermalMax) els.thermalMax.textContent = '-';
  if (els.thermalAvg) els.thermalAvg.textContent = '-';
  if (els.thermalDropped) els.thermalDropped.textContent = '0';
  if (els.thermalLastTime) els.thermalLastTime.textContent = '-';
  if (els.thermalState) els.thermalState.textContent = 'Stream off';
  if (els.thermalDebugInfo) els.thermalDebugInfo.textContent = '-';
  const canvas = els.thermalCanvas;
  if (canvas) {
    const ctx = canvas.getContext('2d');
    if (ctx) {
      ctx.fillStyle = '#101512';
      ctx.fillRect(0, 0, canvas.width, canvas.height);
    }
  }
}

async function startThermalStream() {
  const rateIdxRaw = Number.parseInt(els.thermalRateIndex?.value ?? String(THERMAL_SAMPLE_RATE_INDEX), 10);
  const sampleRateIndex = Number.isFinite(rateIdxRaw) ? rateIdxRaw & 0xff : THERMAL_SAMPLE_RATE_INDEX;
  els.sensorId.value = String(THERMAL_SENSOR_ID);
  els.sampleRateIndex.value = String(sampleRateIndex);

  await ensureTdmNotifications({ reset: lastTdmSeq == null });
  await setSensorDataNotify(true);
  thermalNotifyEnabled = true;

  resetThermalAssembly();
  thermalCurrentFrameTime = null;

  const written = await writeSensorConfigPayload(THERMAL_SENSOR_ID, sampleRateIndex, STORAGE_STREAMING);
  if (written) {
    els.thermalState.textContent = `Streaming @ idx ${sampleRateIndex}`;
    log('Thermal IR stream requested. TDM packets will carry one 32-pixel row per valid slot.');
  }
}

async function stopThermalStream() {
  els.sensorId.value = String(THERMAL_SENSOR_ID);
  els.sampleRateIndex.value = String(THERMAL_SAMPLE_RATE_INDEX);
  await writeSensorConfigPayload(THERMAL_SENSOR_ID, THERMAL_SAMPLE_RATE_INDEX, 0x00);
  els.thermalState.textContent = 'Stream off';
  thermalNotifyEnabled = false;
}

async function writeRecordingName(prefix) {
  if (!sensorRecordingNameChar) {
    return;
  }

  const safePrefix = prefix.replace(/[^a-zA-Z0-9_-]/g, '_').slice(0, 24);
  await gattWriteValue(sensorRecordingNameChar, new TextEncoder().encode(safePrefix));
}

async function setBleControlOnlyMode() {
  if (sensorDataChar && sensorDataNotifying) {
    await setSensorDataNotify(false);
  }

  if (audioWaveformControlChar) {
    await gattWriteValue(audioWaveformControlChar, new Uint8Array([0x00]));
  }

  if (audioWaveformDataChar && audioWaveformNotifying) {
    await gattStopNotifications(audioWaveformDataChar, handleAudioWaveformData);
    audioWaveformNotifying = false;
  }

  audioWindowAssembly = null;
}

function selectedSdAudioStorageOptions() {
  let options = STORAGE_DATA_STORAGE;
  if (els.sdAudioLeft?.checked) options |= STORAGE_AUDIO_LEFT;
  if (els.sdAudioRight?.checked) options |= STORAGE_AUDIO_RIGHT;
  if ((options & (STORAGE_AUDIO_LEFT | STORAGE_AUDIO_RIGHT)) === 0) {
    options |= STORAGE_AUDIO_LEFT | STORAGE_AUDIO_RIGHT;
  }
  return options;
}

async function startSdDataLogger() {
  if (!sensorConfigChar) {
    log('Sensor config characteristic unavailable');
    return;
  }

  if (!els.sdLoggerEnabled?.checked) {
    els.sdLoggerState.textContent = 'SD card not selected';
    return;
  }

  const configs = [];
  if (els.sdLogImu?.checked) {
    const imuRate = Number.parseInt(els.sdImuRateIndex.value, 10) || IMU_SAMPLE_RATE_INDEX;
    configs.push([IMU_SENSOR_ID, imuRate, STORAGE_DATA_STORAGE]);
  }
  if (els.sdLogThermal?.checked) {
    const thermalRate = Number.parseInt(els.sdThermalRateIndex.value, 10) || THERMAL_SAMPLE_RATE_INDEX;
    configs.push([THERMAL_SENSOR_ID, thermalRate, STORAGE_DATA_STORAGE]);
  }
  if (els.sdLogAudio?.checked) {
    const audioRate = Number.parseInt(els.sdAudioRateIndex.value, 10) || MICROPHONE_SAMPLE_RATE_INDEX;
    configs.push([MICROPHONE_SENSOR_ID, audioRate, selectedSdAudioStorageOptions()]);
  }

  if (configs.length === 0) {
    els.sdLoggerState.textContent = 'No sensors selected';
    return;
  }

  if (els.startSdLoggerBtn) els.startSdLoggerBtn.disabled = true;

  try {
    await setBleControlOnlyMode();
    await new Promise((resolve) => setTimeout(resolve, 150));

    await writeRecordingName(`SD_${Date.now()}_`);
    await new Promise((resolve) => setTimeout(resolve, 100));

    let successCount = 0;
    for (const [sensorId, rateIndex, storageOptions] of configs) {
      const ok = await writeSensorConfigPayload(sensorId, rateIndex, storageOptions);
      if (ok) successCount++;
      await new Promise((resolve) => setTimeout(resolve, 80));
    }

    if (successCount === 0) {
      els.sdLoggerState.textContent = 'Start failed';
      log('SD card data logger failed: no sensor config accepted');
      return;
    }

    const audioFiles = els.sdLogAudio?.checked
      ? `${els.sdAudioLeft?.checked ? '_L' : ''}${els.sdAudioRight?.checked ? '_R' : ''}` || '_L_R'
      : '-';
    els.sdLoggerState.textContent = 'Recording to SD';
    els.sdLoggerMode.textContent = 'SD';
    els.sdLoggerAudioFiles.textContent = audioFiles;
    els.sdLoggerLastCommand.textContent = `${successCount}/${configs.length} sensor config(s)`;
    if (els.stopSdLoggerBtn) els.stopSdLoggerBtn.disabled = false;
    log('SD card data logger started', { configs, successCount });
  } catch (error) {
    els.sdLoggerState.textContent = 'Start failed';
    log(`SD logger start failed: ${error.message}`);
  } finally {
    if (els.startSdLoggerBtn) els.startSdLoggerBtn.disabled = !server?.connected;
  }
}

async function stopSdDataLogger() {
  if (!sensorConfigChar) {
    log('Sensor config characteristic unavailable');
    return;
  }

  if (els.stopSdLoggerBtn) els.stopSdLoggerBtn.disabled = true;

  const stopConfigs = [
    [IMU_SENSOR_ID, IMU_SAMPLE_RATE_INDEX, 0x00],
    [THERMAL_SENSOR_ID, THERMAL_SAMPLE_RATE_INDEX, 0x00],
    [MICROPHONE_SENSOR_ID, MICROPHONE_SAMPLE_RATE_INDEX, 0x00]
  ];

  try {
    for (const [sensorId, rateIndex, storageOptions] of stopConfigs) {
      await writeSensorConfigPayload(sensorId, rateIndex, storageOptions);
      await new Promise((resolve) => setTimeout(resolve, 80));
    }

    els.sdLoggerState.textContent = 'Idle';
    els.sdLoggerMode.textContent = 'BLE';
    els.sdLoggerAudioFiles.textContent = '-';
    els.sdLoggerLastCommand.textContent = 'Stop';
    log('SD card data logger stopped');
  } catch (error) {
    els.sdLoggerState.textContent = 'Stop failed';
    log(`SD logger stop failed: ${error.message}`);
  } finally {
    if (els.startSdLoggerBtn) els.startSdLoggerBtn.disabled = !server?.connected;
  }
}

let bleLogTimer = null;

async function waitForBleLoggerOperation() {
  while (bleLoggerOperation !== null) {
    await new Promise((resolve) => setTimeout(resolve, 50));
  }
}

async function startBleDataLogger() {
  if (!sensorConfigChar) {
    log('Sensor config characteristic unavailable');
    return;
  }

  if (bleLoggerOperation) {
    log('BLE logger busy, please wait');
    return;
  }

  bleLoggerOperation = 'starting';
  if (els.startBleLoggerBtn) els.startBleLoggerBtn.disabled = true;
  if (els.stopBleLoggerBtn) els.stopBleLoggerBtn.disabled = true;
  if (els.downloadBleLogBtn) els.downloadBleLogBtn.disabled = true;

  bleLogAudioBuffer = [];
  bleLogImuBuffer = [];
  bleLogThermalBuffer = [];
  bleLogDroppedCount = 0;
  bleLogStartTime = Date.now();

  const configs = [];
  if (els.bleLogImu?.checked) {
    configs.push([IMU_SENSOR_ID, IMU_SAMPLE_RATE_INDEX, STORAGE_STREAMING]);
  }
  if (els.bleLogThermal?.checked) {
    configs.push([THERMAL_SENSOR_ID, THERMAL_SAMPLE_RATE_INDEX, STORAGE_STREAMING]);
  }
  if (els.bleLogAudio?.checked) {
    configs.push([MICROPHONE_SENSOR_ID, MICROPHONE_SAMPLE_RATE_INDEX, STORAGE_STREAMING]);
  }

  if (configs.length === 0) {
    els.bleLoggerState.textContent = 'No sensors selected';
    bleLoggerOperation = null;
    if (els.startBleLoggerBtn) els.startBleLoggerBtn.disabled = !server?.connected;
    return;
  }

  try {
    const needTdm = configs.length > 0;
    if (needTdm && !audioWaveformNotifying) {
      audioWindowAssembly = null;
      await ensureTdmNotifications({ reset: true });
      await new Promise((resolve) => setTimeout(resolve, 100));
    }

    let successCount = 0;
    for (const [sensorId, rateIndex, storageOptions] of configs) {
      const ok = await writeSensorConfigPayload(sensorId, rateIndex, storageOptions);
      if (ok) successCount++;
      await new Promise((resolve) => setTimeout(resolve, 80));
    }

    if (successCount === 0) {
      els.bleLoggerState.textContent = 'Start failed';
      log('BLE data logger failed: no sensor config accepted');
      return;
    }

    isBleLogging = true;
    if (els.bleLoggerState) els.bleLoggerState.textContent = 'Recording to Memory';
    if (els.bleLogDuration) els.bleLogDuration.textContent = '0.0s';
    if (els.bleLogDropped) els.bleLogDropped.textContent = '0';
    if (els.stopBleLoggerBtn) els.stopBleLoggerBtn.disabled = false;

    if (bleLogTimer) clearInterval(bleLogTimer);
    bleLogTimer = setInterval(() => {
      if (els.bleLogDuration && isBleLogging) {
        els.bleLogDuration.textContent = `${((Date.now() - bleLogStartTime) / 1000).toFixed(1)}s`;
      }
    }, 100);

    log('BLE data logger started', { successCount, total: configs.length });
  } catch (err) {
    isBleLogging = false;
    els.bleLoggerState.textContent = 'Start failed';
    log(`BLE data logger error: ${err.message}`);
  } finally {
    bleLoggerOperation = null;
    if (els.startBleLoggerBtn) els.startBleLoggerBtn.disabled = !server?.connected || isBleLogging;
  }
}

async function stopBleDataLogger() {
  await waitForBleLoggerOperation();
  bleLoggerOperation = 'stopping';
  if (els.stopBleLoggerBtn) els.stopBleLoggerBtn.disabled = true;

  const stopConfigs = [
    [IMU_SENSOR_ID, IMU_SAMPLE_RATE_INDEX, 0x00],
    [THERMAL_SENSOR_ID, THERMAL_SAMPLE_RATE_INDEX, 0x00],
    [MICROPHONE_SENSOR_ID, MICROPHONE_SAMPLE_RATE_INDEX, 0x00]
  ];

  try {
    for (const [sensorId, rateIndex, storageOptions] of stopConfigs) {
      await writeSensorConfigPayload(sensorId, rateIndex, storageOptions);
      await new Promise((resolve) => setTimeout(resolve, 80));
    }

    if (audioWaveformNotifying) {
      await stopAudioWaveform();
    }

    isBleLogging = false;
    if (bleLogTimer) {
      clearInterval(bleLogTimer);
      bleLogTimer = null;
    }

    const hasData = bleLogAudioBuffer.length > 0 || bleLogImuBuffer.length > 0 || bleLogThermalBuffer.length > 0;
    if (els.bleLoggerState) els.bleLoggerState.textContent = 'Idle';
    if (els.downloadBleLogBtn) els.downloadBleLogBtn.disabled = !hasData;
    log('BLE data logger stopped');
  } catch (error) {
    log(`BLE logger stop failed: ${error.message}`);
  } finally {
    bleLoggerOperation = null;
    if (els.startBleLoggerBtn) els.startBleLoggerBtn.disabled = !server?.connected;
    if (els.stopBleLoggerBtn) els.stopBleLoggerBtn.disabled = isBleLogging;
  }
}

function downloadBleLogCSV() {
  if (bleLogAudioBuffer.length === 0 && bleLogImuBuffer.length === 0 && bleLogThermalBuffer.length === 0) {
    return;
  }
  
  let csvContent = "timestamp_ms,type,data...\n";
  
  for (const item of bleLogImuBuffer) {
    csvContent += `${item.timestamp_ms},IMU,${item.ax},${item.ay},${item.az},${item.gx},${item.gy},${item.gz},${item.mx},${item.my},${item.mz}\n`;
  }
  for (const item of bleLogAudioBuffer) {
    csvContent += `${item.timestamp_ms},AUDIO,${item.samples.join(',')}\n`;
  }
  for (const item of bleLogThermalBuffer) {
    csvContent += `${item.timestamp_ms},THERMAL,ROW${item.row},${item.pixels.join(',')}\n`;
  }
  
  const blob = new Blob([csvContent], { type: "text/csv;charset=utf-8;" });
  const link = document.createElement("a");
  const url = URL.createObjectURL(blob);
  link.setAttribute("href", url);
  link.setAttribute("download", `ble_data_${Date.now()}.csv`);
  link.style.visibility = 'hidden';
  document.body.appendChild(link);
  link.click();
  document.body.removeChild(link);
}

function decodeSensorConfigStatus(value) {
  if (!value || value.byteLength === 0) {
    return 'empty';
  }

  if (value.byteLength % 3 !== 0) {
    return bytesToHex(value);
  }

  const configs = [];
  for (let offset = 0; offset < value.byteLength; offset += 3) {
    const sensorId = value.getUint8(offset);
    const sampleRateIndex = value.getUint8(offset + 1);
    const storageOptions = value.getUint8(offset + 2);
    configs.push(`${SENSOR_NAMES.get(sensorId) || sensorId}:rate=${sampleRateIndex},storage=0x${storageOptions.toString(16).padStart(2, '0')}`);
  }
  return configs.join(' | ');
}

function decodeTdmImuSlot(value, timestampMs) {
  const imuLen = value.getUint8(TDM_IMU_LEN_OFFSET);
  if (imuLen !== TDM_IMU_BYTES) {
    return 'imu=hold';
  }

  const raw = [];
  for (let i = 0; i < 9; i++) {
    raw.push(value.getInt16(TDM_IMU_OFFSET + i * 2, true));
  }

  const ax = raw[0] * TDM_ACCEL_SCALE_MPS2_PER_LSB;
  const ay = raw[1] * TDM_ACCEL_SCALE_MPS2_PER_LSB;
  const az = raw[2] * TDM_ACCEL_SCALE_MPS2_PER_LSB;
  const gx = raw[3] * TDM_GYRO_SCALE_DPS_PER_LSB;
  const gy = raw[4] * TDM_GYRO_SCALE_DPS_PER_LSB;
  const gz = raw[5] * TDM_GYRO_SCALE_DPS_PER_LSB;
  const mx = raw[6] * TDM_MAG_SCALE_UT_PER_LSB;
  const my = raw[7] * TDM_MAG_SCALE_UT_PER_LSB;
  const mz = raw[8] * TDM_MAG_SCALE_UT_PER_LSB;

  els.imuTime.textContent = `${timestampMs} ms`;
  els.imuAx.textContent = formatNumber(ax);
  els.imuAy.textContent = formatNumber(ay);
  els.imuAz.textContent = formatNumber(az);
  els.imuGx.textContent = formatNumber(gx);
  els.imuGy.textContent = formatNumber(gy);
  els.imuGz.textContent = formatNumber(gz);
  els.imuMx.textContent = formatNumber(mx);
  els.imuMy.textContent = formatNumber(my);
  els.imuMz.textContent = formatNumber(mz);

  if (isBleLogging && els.bleLogImu?.checked) {
    bleLogImuBuffer.push({ timestamp_ms: timestampMs, ax, ay, az, gx, gy, gz, mx, my, mz });
    if (els.bleLogImuCount) els.bleLogImuCount.textContent = bleLogImuBuffer.length;
  }

  return `imu=[${raw.join(',')}]`;
}

function decodeTdmThermalSlot(value, timestampMs) {
  const thermalLen = value.getUint8(TDM_THERMAL_LEN_OFFSET);
  if (thermalLen !== TDM_THERMAL_BYTES) {
    return 'thermal=0';
  }

  const row = value.getUint8(TDM_THERMAL_ROW_OFFSET);
  if (row >= THERMAL_NUM_ROWS) {
    thermalDroppedFrames += 1;
    thermalTdmFrameActive = false;
    thermalTdmExpectedRow = 0;
    if (els.thermalDropped) els.thermalDropped.textContent = String(thermalDroppedFrames);
    if (els.thermalDebugInfo) els.thermalDebugInfo.textContent = `Bad TDM row ${row}`;
    return `thermalBadRow=${row}`;
  }

  if (row === 0) {
    resetThermalAssembly();
    thermalCurrentFrameTime = timestampMs * 1000;
    thermalTdmFrameActive = true;
    thermalTdmExpectedRow = 0;
  } else if (!thermalTdmFrameActive) {
    thermalDroppedFrames += 1;
    thermalTdmExpectedRow = 0;
    if (els.thermalDropped) els.thermalDropped.textContent = String(thermalDroppedFrames);
    if (els.thermalDebugInfo) els.thermalDebugInfo.textContent = `Waiting for TDM row 0, dropped row ${row}`;
    return `thermalWaitingRow0=${row}`;
  }

  if (row !== thermalTdmExpectedRow) {
    const expected = thermalTdmExpectedRow;
    thermalDroppedFrames += 1;
    resetThermalAssembly();
    thermalTdmFrameActive = false;
    thermalTdmExpectedRow = 0;
    thermalCurrentFrameTime = null;
    if (els.thermalDropped) els.thermalDropped.textContent = String(thermalDroppedFrames);
    if (els.thermalDebugInfo) els.thermalDebugInfo.textContent = `TDM row jump ${row}, expected ${expected}`;
    return `thermalRowJump=${row}/${expected}`;
  }

  for (let col = 0; col < THERMAL_NUM_COLS; col++) {
    thermalFrameRaw[row * THERMAL_NUM_COLS + col] =
      value.getInt16(TDM_THERMAL_OFFSET + col * 2, false);
  }

  if (!thermalChunkReceived[row]) {
    thermalChunkReceived[row] = 1;
    thermalChunksThisFrame += 1;
  }

  thermalTdmExpectedRow += 1;
  if (els.thermalChunks) els.thermalChunks.textContent = `${thermalChunksThisFrame}/${THERMAL_NUM_ROWS}`;
  if (els.thermalDebugInfo) els.thermalDebugInfo.textContent = `TDM row ${row + 1}/${THERMAL_NUM_ROWS}`;
  if (els.thermalState) els.thermalState.textContent = 'Receiving TDM rows';

  if (thermalChunksThisFrame >= THERMAL_NUM_ROWS) {
    if (isBleLogging && els.bleLogThermal?.checked) {
      bleLogThermalBuffer.push({ timestamp_ms: timestampMs, row, pixels: Array.from(thermalFrameRaw) });
      if (els.bleLogThermalCount) els.bleLogThermalCount.textContent = bleLogThermalBuffer.length;
    }
    
    renderThermalFrame();
    resetThermalAssembly();
    thermalTdmFrameActive = false;
    thermalTdmExpectedRow = 0;
    thermalCurrentFrameTime = null;
  }

  return `thermalRow=${row}`;
}

function decodeTdmStreamPacket(value) {
  const seq = value.getUint8(0);
  const timestampMs = value.getUint32(1, false);
  let droppedNow = 0;

  if (lastTdmSeq !== null) {
    droppedNow = (seq - lastTdmSeq - 1 + 256) & 0xff;
    if (droppedNow > 0) {
      tdmDroppedPackets += droppedNow;
    }
  }
  lastTdmSeq = seq;

  const micLenRaw = value.getUint8(5);
  const micLen = Math.min(micLenRaw & 0xfe, TDM_MIC_BYTES);
  const micSamples = [];
  for (let offset = 0; offset < micLen; offset += 2) {
    micSamples.push(value.getInt16(TDM_MIC_OFFSET + offset, true));
  }

  const thermalText = decodeTdmThermalSlot(value, timestampMs);
  const imuText = decodeTdmImuSlot(value, timestampMs);
  const thermalLen = value.getUint8(TDM_THERMAL_LEN_OFFSET);
  const imuLen = value.getUint8(TDM_IMU_LEN_OFFSET);

  els.audioSeq.textContent = String(seq);
  els.audioRate.textContent = `${TDM_SAMPLE_RATE_HZ} Hz PCM / ${formatFrequency(TDM_SAMPLE_RATE_HZ)} plot`;
  els.audioSampleInterval.textContent = `${(1000000 / TDM_SAMPLE_RATE_HZ).toFixed(2)} us`;
  els.audioFrames.textContent = `${micSamples.length}/${TDM_MIC_BYTES / 2} samples, ${TDM_PACKET_PERIOD_MS} ms slot`;
  els.audioPacketInfo.textContent =
    `TDM ${value.byteLength} B, mic=${micLen}/${TDM_MIC_BYTES}, thermal=${thermalLen}/${TDM_THERMAL_BYTES}, imu=${imuLen}/${TDM_IMU_BYTES}, dropped=${tdmDroppedPackets}`;

  if (droppedNow > 0) {
    els.audioFrequencyConfidence.textContent = `packet loss +${droppedNow}, total ${tdmDroppedPackets}`;
  }

  if (micSamples.length > 0) {
    let peak = 0;
    let sumAbs = 0;
    for (const sample of micSamples) {
      const abs = Math.abs(sample);
      peak = Math.max(peak, abs);
      sumAbs += abs;
    }

    const meanAbs = Math.round(sumAbs / micSamples.length);
    const frequency = estimateWaveFrequency(micSamples, TDM_SAMPLE_RATE_HZ);
    const samplesPerCycle = frequency.hz ? TDM_SAMPLE_RATE_HZ / frequency.hz : null;
    const p2p = getPeakToPeak(micSamples);

    els.audioPeak.textContent = `mono ${peak}`;
    els.audioMean.textContent = `mono ${meanAbs}`;
    els.audioFrequency.textContent = formatFrequency(frequency.hz);
    els.audioSamplesPerCycle.textContent = Number.isFinite(samplesPerCycle) ? `${samplesPerCycle.toFixed(2)} pts` : '-';
    els.audioPeakToPeak.textContent = `${p2p} raw`;
    if (droppedNow === 0) {
      els.audioFrequencyConfidence.textContent = frequency.detail;
    }
    els.audioWaveState.textContent = `Receiving ${TDM_PACKET_SIZE} B TDM stream`;

    audioPeakHistory.push(peak);
    if (audioPeakHistory.length > AUDIO_PEAK_HISTORY_LIMIT) {
      audioPeakHistory.shift();
    }

    lastAudioSamples = micSamples;
    lastAudioPeak = peak;
    lastAudioDurationMs = (micSamples.length / TDM_SAMPLE_RATE_HZ) * 1000;
    lastAudioPointRate = TDM_SAMPLE_RATE_HZ;

    if (isRecordingAudio) {
      appendRecordedTdmSamples(timestampMs, micSamples, droppedNow);
      const recordedSecs = (recordedAudioBuffer.length / TDM_SAMPLE_RATE_HZ).toFixed(1);
      els.recordStatus.textContent = `${recordedSecs}s`;
      els.playAudioBtn.disabled = false;
    }

    if (isBleLogging && els.bleLogAudio?.checked) {
      if (droppedNow > 0) {
        bleLogDroppedCount += droppedNow;
        if (els.bleLogDropped) els.bleLogDropped.textContent = String(bleLogDroppedCount);
      }
      bleLogAudioBuffer.push({ timestamp_ms: timestampMs, samples: micSamples, dropped_packets: droppedNow });
      if (els.bleLogAudioCount) els.bleLogAudioCount.textContent = String(bleLogAudioBuffer.length);
    }

    drawAudioWaveform(lastAudioSamples, {
      peak,
      durationMs: lastAudioDurationMs,
      pointRate: TDM_SAMPLE_RATE_HZ,
      sampleRate: TDM_SAMPLE_RATE_HZ,
      formatLabel: 'tdm pcm16'
    });
  } else {
    els.audioWaveState.textContent = 'TDM active, mic slot empty';
  }

  els.lastSensor.textContent = 'TDM composite';
  els.lastPayload.textContent = `${value.byteLength} B`;
  return `seq=${seq} t=${timestampMs}ms mic=${micLen}B thermal=${thermalLen}B imu=${imuLen}B drop=${tdmDroppedPackets} ${thermalText} ${imuText}`;
}

function decodeAudioWaveformPacket(value) {
  if (value && value.byteLength === TDM_PACKET_SIZE) {
    return decodeTdmStreamPacket(value);
  }

  if (!value || value.byteLength < 19) {
    return value ? `short audio packet ${bytesToHex(value)}` : 'empty audio packet';
  }

  const sequence = value.getUint32(0, true);
  const sampleRate = value.getUint32(4, true);
  const frameCount = value.getUint16(8, true);
  const peakL = value.getUint16(10, true);
  const peakR = value.getUint16(12, true);
  const meanL = value.getUint16(14, true);
  const meanR = value.getUint16(16, true);
  const declaredCount = value.getUint8(18);
  const isPcm16Packet = value.byteLength >= 28 && value.getUint8(19) === AUDIO_SAMPLE_FORMAT_PCM16;
  const peak = Math.max(peakL, peakR);

  if (!isPcm16Packet) {
    return decodeLegacyAudioWaveformPacket(value, {
      sequence,
      sampleRate,
      frameCount,
      peakL,
      peakR,
      meanL,
      meanR,
      declaredCount,
      peak
    });
  }

  const sampleFormat = value.getUint8(19);
  const windowId = value.getUint16(20, true);
  const sampleOffset = value.getUint16(22, true);
  const totalSampleCount = value.getUint16(24, true);
  const decimation = Math.max(1, value.getUint16(26, true));
  const sampleCount = Math.min(
    declaredCount,
    Math.floor(Math.max(0, value.byteLength - 34) / 2),
    Math.max(0, totalSampleCount - sampleOffset)
  );
  const samples = [];
  for (let i = 0; i < sampleCount; i++) {
    samples.push(value.getInt16(28 + (i * 2), true));
  }

  const extensionOffset = 28 + (sampleCount * 2);
  const hasRawPeakToPeak = value.byteLength >= extensionOffset + 6;
  const rawMin = hasRawPeakToPeak ? value.getInt16(extensionOffset, true) : null;
  const rawMax = hasRawPeakToPeak ? value.getInt16(extensionOffset + 2, true) : null;
  const rawPeakToPeak = hasRawPeakToPeak ? value.getUint16(extensionOffset + 4, true) : null;
  const pointRate = sampleRate ? sampleRate / decimation : 0;
  const durationMs = sampleRate ? (frameCount / sampleRate) * 1000 : 0;

  if (!audioWindowAssembly ||
      audioWindowAssembly.windowId !== windowId ||
      audioWindowAssembly.totalSampleCount !== totalSampleCount ||
      audioWindowAssembly.frameCount !== frameCount) {
    audioWindowAssembly = {
      windowId,
      totalSampleCount,
      frameCount,
      sampleRate,
      pointRate,
      decimation,
      peakL,
      peakR,
      meanL,
      meanR,
      rawMin,
      rawMax,
      rawPeakToPeak,
      samples: new Array(totalSampleCount),
      received: new Array(totalSampleCount).fill(false),
      receivedCount: 0
    };
  }

  for (let i = 0; i < samples.length; i++) {
    const index = sampleOffset + i;
    if (index < audioWindowAssembly.samples.length && !audioWindowAssembly.received[index]) {
      audioWindowAssembly.received[index] = true;
      audioWindowAssembly.receivedCount += 1;
    }
    if (index < audioWindowAssembly.samples.length) {
      audioWindowAssembly.samples[index] = samples[i];
    }
  }

  const complete = audioWindowAssembly.receivedCount >= audioWindowAssembly.totalSampleCount;
  const assembledSamples = complete
    ? audioWindowAssembly.samples
    : audioWindowAssembly.samples.filter((sample) => Number.isFinite(sample));
  const frequency = complete
    ? estimateWaveFrequency(assembledSamples, pointRate)
    : { hz: null, detail: `receiving ${audioWindowAssembly.receivedCount}/${totalSampleCount} pts` };
  const samplesPerCycle = frequency.hz && pointRate ? pointRate / frequency.hz : null;

  els.audioSeq.textContent = String(sequence);
  els.audioRate.textContent = sampleRate ? `${sampleRate} Hz PCM / ${formatFrequency(pointRate)} plot` : '-';
  els.audioSampleInterval.textContent = pointRate ? `${(1000000 / pointRate).toFixed(2)} us` : '-';
  els.audioFrames.textContent = `${frameCount} PCM / ${totalSampleCount} pts`;
  els.audioPeak.textContent = `L ${peakL} / R ${peakR}`;
  els.audioMean.textContent = `L ${meanL} / R ${meanR}`;
  els.audioFrequency.textContent = formatFrequency(frequency.hz);
  els.audioSamplesPerCycle.textContent = Number.isFinite(samplesPerCycle) ? `${samplesPerCycle.toFixed(2)} pts` : '-';
  els.audioPeakToPeak.textContent = hasRawPeakToPeak
    ? `${rawPeakToPeak} raw (${rawMin}..${rawMax})`
    : `${getPeakToPeak(assembledSamples)} raw`;
  els.audioFrequencyConfidence.textContent = frequency.detail;
  els.audioPacketInfo.textContent = `pcm16 chunk ${sampleOffset}-${sampleOffset + sampleCount}/${totalSampleCount}, ${value.byteLength} B`;
  els.audioWaveState.textContent = complete
    ? 'Receiving PCM16 scope'
    : `Receiving ${audioWindowAssembly.receivedCount}/${totalSampleCount}`;

  audioPeakHistory.push(peak);
  if (audioPeakHistory.length > AUDIO_PEAK_HISTORY_LIMIT) {
    audioPeakHistory.shift();
  }

  if (complete || assembledSamples.length > 0) {
    lastAudioSamples = assembledSamples.slice();
    lastAudioPeak = peak;
    lastAudioDurationMs = durationMs;
    lastAudioPointRate = pointRate;
    
    // Save to recording buffer if active
    if (complete && isRecordingAudio) {
      if (recordedAudioPointRate === 0) recordedAudioPointRate = pointRate;
      for (let i = 0; i < assembledSamples.length; i++) {
        recordedAudioBuffer.push(assembledSamples[i]);
      }
      const recordedSecs = (recordedAudioBuffer.length / recordedAudioPointRate).toFixed(1);
      els.recordStatus.textContent = `${recordedSecs}s`;
      els.playAudioBtn.disabled = false;
    }

    drawAudioWaveform(lastAudioSamples, {
      peak,
      durationMs,
      pointRate,
      sampleRate,
      formatLabel: sampleFormat === AUDIO_SAMPLE_FORMAT_PCM16 ? 'pcm16' : `fmt ${sampleFormat}`
    });
  }

  return `seq=${sequence} win=${windowId} offset=${sampleOffset} count=${sampleCount}/${totalSampleCount} rate=${sampleRate}Hz pointRate=${Math.round(pointRate)}Hz freq=${formatFrequency(frequency.hz)} p2p=${hasRawPeakToPeak ? rawPeakToPeak : '-'} peak=[${peakL},${peakR}]`;
}

function decodeLegacyAudioWaveformPacket(value, header) {
  const sampleCount = Math.min(header.declaredCount, value.byteLength - 19);
  const samples = Array.from(new Int8Array(value.buffer, value.byteOffset + 19, sampleCount));
  const extensionOffset = 19 + sampleCount;
  const hasRawPeakToPeak = value.byteLength >= extensionOffset + 6;
  const rawMin = hasRawPeakToPeak ? value.getInt16(extensionOffset, true) : null;
  const rawMax = hasRawPeakToPeak ? value.getInt16(extensionOffset + 2, true) : null;
  const rawPeakToPeak = hasRawPeakToPeak ? value.getUint16(extensionOffset + 4, true) : null;
  const durationMs = header.sampleRate ? (header.frameCount / header.sampleRate) * 1000 : 0;
  const pointRate = durationMs > 0 ? sampleCount / (durationMs / 1000) : header.sampleRate;
  const frequency = estimateWaveFrequency(samples, pointRate);
  const samplesPerCycle = frequency.hz && pointRate ? pointRate / frequency.hz : null;
  const previewPeakToPeak = getPeakToPeak(samples);

  els.audioSeq.textContent = String(header.sequence);
  els.audioRate.textContent = header.sampleRate ? `${header.sampleRate} Hz PCM / ${formatFrequency(pointRate)} plot` : '-';
  els.audioSampleInterval.textContent = pointRate ? `${(1000000 / pointRate).toFixed(2)} us` : '-';
  els.audioFrames.textContent = `${header.frameCount} PCM / ${sampleCount} pts`;
  els.audioPeak.textContent = `L ${header.peakL} / R ${header.peakR}`;
  els.audioMean.textContent = `L ${header.meanL} / R ${header.meanR}`;
  els.audioFrequency.textContent = formatFrequency(frequency.hz);
  els.audioSamplesPerCycle.textContent = Number.isFinite(samplesPerCycle) ? `${samplesPerCycle.toFixed(2)} pts` : '-';
  els.audioPeakToPeak.textContent = hasRawPeakToPeak
    ? `${rawPeakToPeak} raw (${rawMin}..${rawMax})`
    : `~${header.peak * 2} raw / ${previewPeakToPeak} preview`;
  els.audioFrequencyConfidence.textContent = frequency.detail;
  els.audioPacketInfo.textContent = `int8 preview, ${value.byteLength} B`;
  els.audioWaveState.textContent = audioWaveformNotifying ? 'Receiving legacy preview' : 'Preview available';

  audioPeakHistory.push(header.peak);
  if (audioPeakHistory.length > AUDIO_PEAK_HISTORY_LIMIT) {
    audioPeakHistory.shift();
  }

  lastAudioSamples = samples;
  lastAudioPeak = header.peak;
  lastAudioDurationMs = durationMs;
  lastAudioPointRate = pointRate;
  drawAudioWaveform(lastAudioSamples, {
    peak: header.peak,
    durationMs,
    pointRate,
    sampleRate: header.sampleRate,
    formatLabel: 'int8'
  });

  return `seq=${header.sequence} rate=${header.sampleRate}Hz frames=${header.frameCount} freq=${formatFrequency(frequency.hz)} p2p=${hasRawPeakToPeak ? rawPeakToPeak : `~${header.peak * 2}`} peak=[${header.peakL},${header.peakR}] mean=[${header.meanL},${header.meanR}] samples=${sampleCount}`;
}

function getPeakToPeak(samples) {
  if (!samples || samples.length === 0) {
    return 0;
  }

  let min = samples[0];
  let max = samples[0];
  for (const sample of samples) {
    min = Math.min(min, sample);
    max = Math.max(max, sample);
  }
  return max - min;
}

function estimateWaveFrequency(samples, sampleRate) {
  if (!samples || samples.length < 8 || !sampleRate) {
    return { hz: null, detail: 'no data' };
  }

  let mean = 0;
  for (const sample of samples) {
    mean += sample;
  }
  mean /= samples.length;

  const centered = [];
  let maxAbs = 0;
  for (const sample of samples) {
    const ac = sample - mean;
    centered.push(ac);
    maxAbs = Math.max(maxAbs, Math.abs(ac));
  }

  if (maxAbs < 6) {
    return { hz: null, detail: 'too quiet' };
  }

  const crossings = [];
  for (let i = 1; i < centered.length; i++) {
    const prev = centered[i - 1];
    const curr = centered[i];

    if (prev < 0 && curr >= 0) {
      const denom = curr - prev;
      const frac = denom === 0 ? 0 : -prev / denom;
      crossings.push((i - 1 + frac) / sampleRate);
    }
  }

  if (crossings.length < 2) {
    return { hz: null, detail: 'less than 1 cycle' };
  }

  const periods = [];
  for (let i = 1; i < crossings.length; i++) {
    const period = crossings[i] - crossings[i - 1];
    if (period > 0) {
      periods.push(period);
    }
  }

  if (periods.length === 0) {
    return { hz: null, detail: 'unstable' };
  }

  const avgPeriod = periods.reduce((sum, period) => sum + period, 0) / periods.length;
  const hz = 1 / avgPeriod;
  const jitter = Math.sqrt(
    periods.reduce((sum, period) => sum + ((period - avgPeriod) ** 2), 0) / periods.length
  ) / avgPeriod;
  const cycles = periods.length;
  const quality = cycles >= 3 && jitter < 0.18 ? 'good' : 'rough';

  return {
    hz,
    detail: `${quality}, ${cycles} cycles, ${(jitter * 100).toFixed(0)}% jitter`
  };
}

function drawAudioWaveform(samples, options = {}) {
  const canvas = els.audioWaveCanvas;
  if (!canvas) {
    return;
  }

  const rect = canvas.getBoundingClientRect();
  const width = Math.max(1, Math.floor(rect.width));
  const height = Math.max(1, Math.floor(rect.height));
  const dpr = window.devicePixelRatio || 1;

  if (canvas.width !== Math.floor(width * dpr) || canvas.height !== Math.floor(height * dpr)) {
    canvas.width = Math.floor(width * dpr);
    canvas.height = Math.floor(height * dpr);
  }

  const ctx = canvas.getContext('2d');
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  ctx.clearRect(0, 0, width, height);

  ctx.fillStyle = '#101512';
  ctx.fillRect(0, 0, width, height);

  const waveHeight = Math.round(height * 0.72);
  const historyTop = waveHeight + 18;
  const historyHeight = height - historyTop - 18;

  ctx.strokeStyle = 'rgba(220, 232, 223, 0.18)';
  ctx.lineWidth = 1;
  for (let i = 1; i < 4; i++) {
    const y = (waveHeight * i) / 4;
    ctx.beginPath();
    ctx.moveTo(0, y);
    ctx.lineTo(width, y);
    ctx.stroke();
  }

  ctx.strokeStyle = 'rgba(220, 232, 223, 0.34)';
  ctx.beginPath();
  ctx.moveTo(0, waveHeight / 2);
  ctx.lineTo(width, waveHeight / 2);
  ctx.stroke();

  if (!samples || samples.length === 0) {
    ctx.fillStyle = 'rgba(220, 232, 223, 0.58)';
    ctx.font = '12px Cascadia Mono, Consolas, monospace';
    ctx.fillText('waiting for audio waveform data', 14, Math.round(waveHeight / 2) - 12);
    return;
  }

  let mean = 0;
  for (const sample of samples) {
    mean += sample;
  }
  mean /= samples.length;

  const acSamples = [];
  let maxAbsSample = 1;
  for (const sample of samples) {
    const ac = sample - mean;
    acSamples.push(ac);
    maxAbsSample = Math.max(maxAbsSample, Math.abs(ac));
  }

  const rawPeak = Number.isFinite(options.peak) ? options.peak : 0;
  const durationLabel = Number.isFinite(options.durationMs) && options.durationMs > 0
    ? options.durationMs.toFixed(2)
    : '?';
  const pointRate = Number.isFinite(options.pointRate) && options.pointRate > 0 ? options.pointRate : 0;
  const sampleRate = Number.isFinite(options.sampleRate) && options.sampleRate > 0 ? options.sampleRate : 0;
  const formatLabel = options.formatLabel || 'pcm';
  els.audioPlotScale.textContent = `${formatLabel} raw AC auto, raw peak ${rawPeak}`;

  ctx.strokeStyle = '#29d3ad';
  ctx.lineWidth = 2;
  ctx.beginPath();
  for (let i = 0; i < acSamples.length; i++) {
    const x = acSamples.length === 1 ? width / 2 : (i / (acSamples.length - 1)) * width;
    const y = (waveHeight / 2) - (acSamples[i] / maxAbsSample) * (waveHeight * 0.42);

    if (i === 0) {
      ctx.moveTo(x, y);
    } else {
      ctx.lineTo(x, y);
    }
  }
  ctx.stroke();

  if (acSamples.length <= 520) {
    ctx.fillStyle = '#e8fff7';
    for (let i = 0; i < acSamples.length; i++) {
      const x = acSamples.length === 1 ? width / 2 : (i / (acSamples.length - 1)) * width;
      const y = (waveHeight / 2) - (acSamples[i] / maxAbsSample) * (waveHeight * 0.42);
      ctx.beginPath();
      ctx.arc(x, y, 2.1, 0, Math.PI * 2);
      ctx.fill();
    }
  }

  ctx.fillStyle = 'rgba(220, 232, 223, 0.58)';
  ctx.font = '12px Cascadia Mono, Consolas, monospace';
  const pointText = pointRate ? `${formatFrequency(pointRate)} plot` : 'unknown plot rate';
  const sampleText = sampleRate ? `${formatFrequency(sampleRate)} PCM` : 'unknown PCM';
  const intervalText = pointRate ? `${(1000000 / pointRate).toFixed(2)} us/point` : '? us/point';
  ctx.fillText(`latest ${durationLabel} ms AC scope, ${pointText} from ${sampleText}`, 14, 18);
  ctx.fillText(`${formatLabel}, ${acSamples.length} points, ${intervalText}`, 14, 40);

  if (audioPeakHistory.length > 1 && historyHeight > 20) {
    const maxPeak = Math.max(1, ...audioPeakHistory);
    ctx.strokeStyle = 'rgba(41, 211, 173, 0.38)';
    ctx.beginPath();
    for (let i = 0; i < audioPeakHistory.length; i++) {
      const x = (i / (AUDIO_PEAK_HISTORY_LIMIT - 1)) * width;
      const y = historyTop + historyHeight - (audioPeakHistory[i] / maxPeak) * historyHeight;

      if (i === 0) {
        ctx.moveTo(x, y);
      } else {
        ctx.lineTo(x, y);
      }
    }
    ctx.stroke();

    ctx.fillStyle = 'rgba(220, 232, 223, 0.58)';
    ctx.fillText(`peak history max ${maxPeak}`, 14, historyTop + historyHeight + 14);
  }
}

async function startAdvertisementScan() {
  if (!navigator.bluetooth?.requestLEScan) {
    log('Advertisement scan API unavailable. Use Connect Any BLE instead.');
    return;
  }

  try {
    if (leScan?.active) {
      leScan.stop();
      leScan = null;
      els.scanState.textContent = 'Not scanning';
      log('Advertisement scan stopped');
      return;
    }

    els.advertisements.innerHTML = '';
    navigator.bluetooth.addEventListener('advertisementreceived', handleAdvertisement);
    leScan = await navigator.bluetooth.requestLEScan({ acceptAllAdvertisements: true });
    els.scanState.textContent = 'Scanning';
    log('Advertisement scan started');
    window.setTimeout(() => {
      if (leScan?.active) {
        leScan.stop();
        leScan = null;
        els.scanState.textContent = 'Not scanning';
        log('Advertisement scan stopped automatically');
      }
    }, 15000);
  } catch (error) {
    log(`Advertisement scan failed: ${error.message}`);
  }
}

function handleAdvertisement(event) {
  const name = event.device?.name || '(unnamed)';
  const uuids = event.uuids?.length ? event.uuids.join(', ') : '-';
  const id = event.device?.id || '-';
  const rssi = typeof event.rssi === 'number' ? `${event.rssi} dBm` : '-';
  const key = event.device?.id || name;
  let card = els.advertisements.querySelector(`[data-key="${CSS.escape(key)}"]`);

  if (!card) {
    card = document.createElement('div');
    card.className = 'adv-card';
    card.dataset.key = key;
    els.advertisements.prepend(card);
  }

  card.innerHTML = `
    <div class="adv-title"><span>${name}</span><span class="fine">${rssi}</span></div>
    <div class="fine">ID: ${id}</div>
    <div class="fine uuid">UUIDs: ${uuids}</div>
  `;

  if (name.startsWith('OpenEarable')) {
    log('OpenEarable advertisement received', { name, rssi });
  }
}

els.scanBtn.addEventListener('click', startAdvertisementScan);
els.connectNamedBtn.addEventListener('click', connectOpenEarable);
els.connectAdvertisedBtn.addEventListener('click', connectOpenEarableAdvertised);
els.connectAnyBtn.addEventListener('click', connectAnyBle);
els.disconnectBtn.addEventListener('click', disconnect);
els.readBatteryBtn.addEventListener('click', readBatteryLevel);
els.clearLogBtn.addEventListener('click', () => {
  els.log.textContent = '';
  els.lastEvent.textContent = 'Log cleared';
});
els.startImuBtn.addEventListener('click', startImuStream);
els.stopImuBtn.addEventListener('click', stopImuStream);
els.startAudioWaveBtn.addEventListener('click', startAudioWaveform);
els.stopAudioWaveBtn.addEventListener('click', stopAudioWaveform);
els.readAudioWaveBtn.addEventListener('click', readAudioWaveform);
els.audioWindowMode.addEventListener('change', () => {
  updateAudioWaveformWindow().catch((error) => {
    log(`Audio waveform window update failed: ${error.message}`);
  });
});
els.enableSensorBtn.addEventListener('click', () => writeSensorConfig(true));
els.disableSensorBtn.addEventListener('click', () => writeSensorConfig(false));
els.subscribeSensorBtn.addEventListener('click', toggleSensorDataNotify);
els.subscribeStatusBtn.addEventListener('click', toggleSensorStatusNotify);

if (els.startThermalBtn) {
  els.startThermalBtn.addEventListener('click', () => {
    startThermalStream().catch((error) => log(`Thermal start failed: ${error.message}`));
  });
}
if (els.stopThermalBtn) {
  els.stopThermalBtn.addEventListener('click', () => {
    stopThermalStream().catch((error) => log(`Thermal stop failed: ${error.message}`));
  });
}
if (els.thermalAutoScale) {
  els.thermalAutoScale.addEventListener('change', () => {
    if (thermalFramesRendered > 0) renderThermalFrame();
  });
}
if (els.thermalMinTemp) {
  els.thermalMinTemp.addEventListener('change', () => {
    if (thermalFramesRendered > 0 && !els.thermalAutoScale.checked) renderThermalFrame();
  });
}
if (els.thermalMaxTemp) {
  els.thermalMaxTemp.addEventListener('change', () => {
    if (thermalFramesRendered > 0 && !els.thermalAutoScale.checked) renderThermalFrame();
  });
}
if (els.thermalPalette) {
  els.thermalPalette.addEventListener('change', () => {
    if (thermalFramesRendered > 0) renderThermalFrame();
  });
}
if (els.readHwStatusBtn) {
  els.readHwStatusBtn.addEventListener('click', () => {
    readHardwareStatus().catch((error) => log(`硬件状态刷新失败: ${error.message}`));
  });
}

if (els.startSdLoggerBtn) {
  els.startSdLoggerBtn.addEventListener('click', () => {
    startSdDataLogger().catch((error) => log(`SD logger start failed: ${error.message}`));
  });
}
if (els.stopSdLoggerBtn) {
  els.stopSdLoggerBtn.addEventListener('click', () => {
    stopSdDataLogger().catch((error) => log(`SD logger stop failed: ${error.message}`));
  });
}

if (els.startBleLoggerBtn) {
  els.startBleLoggerBtn.addEventListener('click', () => {
    startBleDataLogger().catch((error) => log(`BLE logger start failed: ${error.message}`));
  });
}
if (els.stopBleLoggerBtn) {
  els.stopBleLoggerBtn.addEventListener('click', () => {
    stopBleDataLogger().catch((error) => log(`BLE logger stop failed: ${error.message}`));
  });
}
if (els.downloadBleLogBtn) {
  els.downloadBleLogBtn.addEventListener('click', downloadBleLogCSV);
}

if (els.tabSdLogger && els.tabBleLogger) {
  els.tabSdLogger.addEventListener('click', () => {
    els.tabSdLogger.classList.add('active');
    els.tabBleLogger.classList.remove('active');
    els.sdLoggerTab.classList.add('active');
    els.bleLoggerTab.classList.remove('active');
  });
  els.tabBleLogger.addEventListener('click', () => {
    els.tabBleLogger.classList.add('active');
    els.tabSdLogger.classList.remove('active');
    els.bleLoggerTab.classList.add('active');
    els.sdLoggerTab.classList.remove('active');
  });
}


resetFacts();
resetAudioWaveformValues();
resetThermalValues();
resetHardwareStatusUi('尚未读取');
setConnectedUi(false);
checkSupport();
window.addEventListener('resize', () => drawAudioWaveform(lastAudioSamples, {
  peak: lastAudioPeak,
  durationMs: lastAudioDurationMs,
  pointRate: lastAudioPointRate
}));

if (els.themeToggleBtn) {
  els.themeToggleBtn.addEventListener('click', () => {
    const isLight = document.body.dataset.theme === 'light';
    if (isLight) {
      document.body.dataset.theme = 'dark';
      els.themeToggleBtn.textContent = '☀ Switch to Day Mode';
    } else {
      document.body.dataset.theme = 'light';
      els.themeToggleBtn.textContent = '🌙 Switch to Night Mode';
    }
  });
}

els.recordAudioBtn.addEventListener('click', () => {
  if (isRecordingAudio) {
    // Stop recording
    isRecordingAudio = false;
    els.recordAudioBtn.textContent = '⏺ Record';
    els.recordAudioBtn.classList.add('danger');
    els.recordAudioBtn.classList.remove('warning');
  } else {
    // Start recording
    isRecordingAudio = true;
    recordedAudioBuffer = [];
    recordedAudioStartMs = null;
    recordedAudioPointRate = TDM_SAMPLE_RATE_HZ;
    els.recordStatus.textContent = '0.0s';
    els.playAudioBtn.disabled = true;
    els.recordAudioBtn.textContent = '⏹ Stop Recording';
    els.recordAudioBtn.classList.remove('danger');
    els.recordAudioBtn.classList.add('warning');
  }
});

els.playAudioBtn.addEventListener('click', () => {
  if (recordedAudioBuffer.length === 0 || recordedAudioPointRate === 0) return;

  // Stop previous playback if exists
  if (currentAudioSource) {
    currentAudioSource.stop();
    currentAudioSource.disconnect();
    currentAudioSource = null;
    els.playAudioBtn.textContent = '▶ Play';
    return;
  }

  // Create AudioContext if not exists
  if (!playbackAudioContext) {
    playbackAudioContext = new (window.AudioContext || window.webkitAudioContext)();
  }

  // Web Audio API requires a minimum sample rate of 8000 Hz.
  // If the recorded rate is lower (e.g. 6000 Hz plot), we must upsample it.
  let targetRate = recordedAudioPointRate;
  let upsampleFactor = 1;
  while (targetRate < 8000) {
    targetRate *= 2;
    upsampleFactor *= 2;
  }

  // Cap targetRate at max supported (96000) just in case
  if (targetRate > 96000) targetRate = 96000;

  const numSamples = recordedAudioBuffer.length * upsampleFactor;
  const audioBuffer = playbackAudioContext.createBuffer(1, numSamples, targetRate);
  const channelData = audioBuffer.getChannelData(0);

  for (let i = 0; i < recordedAudioBuffer.length; i++) {
    // Convert Int16 (-32768 to 32767) to Float32 (-1.0 to 1.0)
    let floatVal = recordedAudioBuffer[i] / 32768.0;
    
    // Nearest-neighbor upsampling (repeat the sample)
    for (let f = 0; f < upsampleFactor; f++) {
      channelData[i * upsampleFactor + f] = floatVal;
    }
  }

  currentAudioSource = playbackAudioContext.createBufferSource();
  currentAudioSource.buffer = audioBuffer;

  const gainNode = playbackAudioContext.createGain();
  gainNode.gain.value = parseFloat(els.playbackVolume.value);
  
  els.playbackVolume.addEventListener('input', (e) => {
    gainNode.gain.value = parseFloat(e.target.value);
  });

  currentAudioSource.connect(gainNode);
  gainNode.connect(playbackAudioContext.destination);

  currentAudioSource.onended = () => {
    els.playAudioBtn.textContent = '▶ Play';
    currentAudioSource = null;
  };

  currentAudioSource.start();
  els.playAudioBtn.textContent = '⏹ Stop Playback';
});
