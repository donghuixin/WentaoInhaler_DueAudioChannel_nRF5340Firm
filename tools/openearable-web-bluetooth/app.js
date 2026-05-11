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
  [7, 'Bone Conduct.']
]);

const IMU_SENSOR_ID = 0;
const IMU_SAMPLE_RATE_INDEX = 1;
const MICROPHONE_SENSOR_ID = 2;
const MICROPHONE_SAMPLE_RATE_INDEX = 0;
const STORAGE_STREAMING = 0x01;
const STORAGE_DATA_STORAGE = 0x02;
const AUDIO_WAVE_CONTROL_ENABLE = 0x01;
const AUDIO_WAVE_CONTROL_RESET = 0x02;
const AUDIO_WAVE_WINDOW_SHIFT = 2;
const AUDIO_SAMPLE_FORMAT_PCM16 = 1;

const els = {
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
  audioWaveCanvas: document.querySelector('#audioWaveCanvas')
};

let device = null;
let server = null;
let leScan = null;
let packetCount = 0;
let sensorConfigChar = null;
let sensorDataChar = null;
let sensorStatusChar = null;
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
  els.audioWaveState.textContent = 'Preview off';
  drawAudioWaveform([]);
}

function resetFacts() {
  for (const id of ['factName', 'factId', 'factBattery', 'factIdentifier', 'factGeneration', 'factFirmware', 'factAudioChannel']) {
    setFact(id, '-');
  }
}

function setConnectedUi(isConnected) {
  els.disconnectBtn.disabled = !isConnected;
  els.readBatteryBtn.disabled = !isConnected;
  els.startImuBtn.disabled = !isConnected || !sensorConfigChar || !sensorDataChar;
  els.stopImuBtn.disabled = !isConnected || !sensorConfigChar;
  els.startAudioWaveBtn.disabled = !isConnected || !sensorConfigChar || !audioWaveformControlChar || !audioWaveformDataChar;
  els.stopAudioWaveBtn.disabled = !isConnected || !audioWaveformControlChar;
  els.readAudioWaveBtn.disabled = !isConnected || !audioWaveformDataChar;
  els.enableSensorBtn.disabled = !isConnected || !sensorConfigChar;
  els.disableSensorBtn.disabled = !isConnected || !sensorConfigChar;
  els.subscribeSensorBtn.disabled = !isConnected || !sensorDataChar;
  els.subscribeStatusBtn.disabled = !isConnected || !sensorStatusChar;
  els.gattState.textContent = isConnected ? 'GATT connected' : 'GATT disconnected';
}

function resetConnectionState() {
  device = null;
  server = null;
  sensorConfigChar = null;
  sensorDataChar = null;
  sensorStatusChar = null;
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
  els.notifyState.textContent = 'Notifications off';
  els.deviceName.textContent = 'No device';
  els.factName.textContent = '-';
  els.factId.textContent = '-';
  els.serviceCount.textContent = '0 found';
  els.servicesList.innerHTML = '';
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
    const value = await characteristic.readValue();
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
      await characteristic.stopNotifications();
      characteristic.removeEventListener('characteristicvaluechanged', handler);
      button.dataset.enabled = 'false';
      button.textContent = 'Notify';
    } else {
      await characteristic.startNotifications();
      characteristic.addEventListener('characteristicvaluechanged', handler);
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
  setConnectedUi(true);
}

async function tryRead(serviceUuid, characteristicUuid, onValue) {
  try {
    const service = await server.getPrimaryService(serviceUuid);
    const characteristic = await service.getCharacteristic(characteristicUuid);
    const value = await characteristic.readValue();
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
    const value = await characteristic.readValue();
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
    await sensorConfigChar.writeValue(payload);
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
    await sensorDataChar.startNotifications();
    sensorDataChar.addEventListener('characteristicvaluechanged', handleSensorData);
    sensorDataNotifying = true;
    els.subscribeSensorBtn.textContent = 'Stop Data';
    els.notifyState.textContent = 'Data notifications on';
    log('Sensor data notifications enabled');
  } else if (!enable && sensorDataNotifying) {
    await sensorDataChar.stopNotifications();
    sensorDataChar.removeEventListener('characteristicvaluechanged', handleSensorData);
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

  const notifyReady = await setSensorDataNotify(true);
  if (!notifyReady) {
    return;
  }

  const written = await writeSensorConfigPayload(IMU_SENSOR_ID, IMU_SAMPLE_RATE_INDEX, STORAGE_STREAMING);
  if (written) {
    log('IMU stream requested. Move the board and watch Accel/Gyro values.');
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

async function updateAudioWaveformWindow() {
  if (!audioWaveformControlChar || !audioWaveformNotifying) {
    return;
  }

  audioWindowAssembly = null;
  await audioWaveformControlChar.writeValue(new Uint8Array([audioWaveformControlValue({ reset: true })]));
  log(`Audio waveform window changed to ${els.audioWindowMode.value}`);
}

async function startAudioWaveform() {
  if (!sensorConfigChar || !audioWaveformControlChar || !audioWaveformDataChar) {
    log('Audio waveform channel unavailable');
    return;
  }

  try {
    if (!audioWaveformNotifying) {
      await audioWaveformDataChar.startNotifications();
      audioWaveformDataChar.addEventListener('characteristicvaluechanged', handleAudioWaveformData);
      audioWaveformNotifying = true;
    }

    audioWindowAssembly = null;
    await audioWaveformControlChar.writeValue(new Uint8Array([audioWaveformControlValue({ reset: true })]));
    await writeSensorConfigPayload(MICROPHONE_SENSOR_ID, MICROPHONE_SAMPLE_RATE_INDEX, STORAGE_DATA_STORAGE);

    els.audioWaveState.textContent = 'Preview on';
    els.startAudioWaveBtn.disabled = true;
    els.stopAudioWaveBtn.disabled = false;
    log(`Audio waveform preview requested, window=${els.audioWindowMode.value}`);
  } catch (error) {
    log(`Audio waveform start failed: ${error.message}`);
  }
}

async function stopAudioWaveform() {
  try {
    if (audioWaveformControlChar) {
      await audioWaveformControlChar.writeValue(new Uint8Array([0x00]));
    }

    if (sensorConfigChar) {
      await writeSensorConfigPayload(MICROPHONE_SENSOR_ID, MICROPHONE_SAMPLE_RATE_INDEX, 0x00);
    }

    if (audioWaveformDataChar && audioWaveformNotifying) {
      await audioWaveformDataChar.stopNotifications();
      audioWaveformDataChar.removeEventListener('characteristicvaluechanged', handleAudioWaveformData);
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
    const value = await audioWaveformDataChar.readValue();
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
      await sensorStatusChar.stopNotifications();
      sensorStatusChar.removeEventListener('characteristicvaluechanged', handleSensorStatus);
      sensorStatusNotifying = false;
      els.subscribeStatusBtn.textContent = 'Subscribe Status';
      els.notifyState.textContent = sensorDataNotifying ? 'Data notifications on' : 'Notifications off';
    } else {
      await sensorStatusChar.startNotifications();
      sensorStatusChar.addEventListener('characteristicvaluechanged', handleSensorStatus);
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

function decodeAudioWaveformPacket(value) {
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

resetFacts();
resetAudioWaveformValues();
setConnectedUi(false);
checkSupport();
window.addEventListener('resize', () => drawAudioWaveform(lastAudioSamples, {
  peak: lastAudioPeak,
  durationMs: lastAudioDurationMs,
  pointRate: lastAudioPointRate
}));
