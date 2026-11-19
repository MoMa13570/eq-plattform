#ifdef ARDUINO_ARCH_ESP32

#include <Arduino.h>
#include <DNSServer.h>
#include <HardwareSerial.h>
#include <Preferences.h>
#include <TMCStepper.h>
#include <U8g2lib.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Wire.h>
#include <esp32-hal-timer.h>
#include <lwip/sockets.h>
#include <rom/ets_sys.h>

// ESP32 DevKit V1 pin assignment. All inputs use switches to GND.
static constexpr uint8_t PIN_STEP = 25;
static constexpr uint8_t PIN_DIR = 26;
static constexpr uint8_t PIN_EN = 27;
static constexpr uint8_t PIN_POT = 34;       // input only, external pot wiper
static constexpr uint8_t PIN_DIR_N = 32;
static constexpr uint8_t PIN_DIR_S = 33;
static constexpr uint8_t PIN_HOME_BTN = 13;
static constexpr uint8_t PIN_ENDSTOP_HOME = 18;
static constexpr uint8_t PIN_ENDSTOP_END = 19;
static constexpr uint8_t PIN_TMC_RX = 16;
static constexpr uint8_t PIN_TMC_TX = 17;
static constexpr uint8_t PIN_I2C_SDA = 21;
static constexpr uint8_t PIN_I2C_SCL = 22;

static constexpr float R_SENSE = 0.11f;
static constexpr uint8_t TMC_ADDR = 0;
static constexpr float STEPS_PER_REV = 200.0f;
static constexpr float MICROSTEPS = 32.0f;
static constexpr float ROLLER_R_MM = 10.0f;
static constexpr float R_MM = 561.263f;
static constexpr float MOTOR_PULLEY_TEETH = 16.0f;
static constexpr float SHAFT_PULLEY_TEETH = 66.0f;
static constexpr float GEAR_RATIO = SHAFT_PULLEY_TEETH / MOTOR_PULLEY_TEETH;
static constexpr float SIDEREAL_SEC = 86164.0f;
static constexpr float LUNAR_RATE_FACTOR = SIDEREAL_SEC / 89428.0f;
static constexpr uint16_t STEP_PULSE_US = 3;
static constexpr bool MOTOR_DIRECTION_INVERTED = true;

static constexpr char AP_SSID[] = "EQ-Plattform";
static constexpr char AP_PASSWORD[] = "eqplattform";

HardwareSerial TMC_SERIAL(2);
TMC2209Stepper driver(&TMC_SERIAL, R_SENSE, TMC_ADDR);
U8X8_SSD1306_128X64_NONAME_HW_I2C oled(U8X8_PIN_NONE);
WebServer server(80);
DNSServer dnsServer;
Preferences preferences;

struct WifiProfile {
  char ssid[33];
  char password[65];
};

static WifiProfile savedWifi{};
static WifiProfile pendingWifi{};
static uint32_t wifiAttemptMs = 0;
static const char *wifiError = "";
static constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 20000;
static constexpr uint32_t WIFI_RETRY_MS = 30000;

enum MotionMode : uint8_t { MODE_IDLE = 0, MODE_TRACKING, MODE_HOMING, MODE_JOG };
enum HomingPhase : uint8_t { HOME_IDLE = 0, HOME_FAST, HOME_BACKOFF, HOME_SLOW };
enum TrackRate : uint8_t { RATE_SIDEREAL = 0, RATE_LUNAR };

struct Settings {
  bool potEnabled;
  float trimMinPct;
  float trimMaxPct;
  uint16_t currentTrackingMa;
  uint16_t currentHomingFastMa;
  uint16_t currentHomingSlowMa;
  float homeFastMultiplier;
  float homeBackoffMultiplier;
  float homeSlowMultiplier;
};

struct Endstops {
  bool home;
  bool end;
};

static Settings config;
static MotionMode mode = MODE_IDLE;
static HomingPhase homePhase = HOME_IDLE;
static TrackRate trackRate = RATE_SIDEREAL;
static bool trackForward = true;
static bool homeForward = false;
static bool homingDirectionFlipped = false;
static uint32_t homingStartedMs = 0;
static volatile uint32_t backoffRemaining = 0;
static volatile TickType_t backoffDeadline = 0;
static float baseUsps = 0.0f;
static float targetUsps = 0.0f;
static float smoothedTrim = 1.0f;
static bool potOk = true;
static bool webTrimActive = false;
static float webTrimPct = 100.0f;
static float trimTarget = 1.0f;
static float filteredPotRaw = -1.0f;
static float potRawAtWebTrim = -1.0f;
static String lastEvent = "System bereit";

static constexpr uint32_t HOME_BACKOFF_USTEPS =
  (uint32_t)(0.25f * STEPS_PER_REV * MICROSTEPS * GEAR_RATIO + 0.5f);
static constexpr uint32_t HOME_BACKOFF_MAX_MS = 2500;
static constexpr uint32_t HOMING_END_GRACE_MS = 300;

static constexpr TickType_t JOG_TIMEOUT_TICKS = pdMS_TO_TICKS(450);
static String jogOwner;
static bool jogForward = false;
static float jogMultiplier = 100.0f;
static volatile TickType_t jogDeadline = 0;
static volatile uint32_t jogEndstopMask = 0;
static volatile bool jogEndstopHit = false;

static hw_timer_t *stepTimer = nullptr;
static portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
static volatile bool timerRunning = false;
static volatile bool timerBlocked = false;
static volatile uint32_t homingEndstopMask = 0;
static volatile bool timerHomeBackoff = false;

void IRAM_ATTR onStepTimer() {
  if (!timerRunning || timerBlocked) return;
  const uint32_t endstop = jogEndstopMask;
  if (endstop) {
    if (!(GPIO.in & endstop)) jogEndstopHit = true;
    if (jogEndstopHit || (int32_t)(xTaskGetTickCountFromISR() - jogDeadline) >= 0) return;
  }
  const uint32_t homingStop = homingEndstopMask;
  if (homingStop && !(GPIO.in & homingStop)) return;
  const bool backingOff = timerHomeBackoff;
  if (backingOff && (backoffRemaining == 0 ||
                     (int32_t)(xTaskGetTickCountFromISR() - backoffDeadline) >= 0)) return;

  GPIO.out_w1ts = (1UL << PIN_STEP);
  ets_delay_us(STEP_PULSE_US);
  GPIO.out_w1tc = (1UL << PIN_STEP);
  if (backingOff) --backoffRemaining;
}

static float clampFloat(float value, float low, float high) {
  return value < low ? low : (value > high ? high : value);
}

static uint16_t clampCurrent(int value) {
  return (uint16_t)(value < 100 ? 100 : (value > 2000 ? 2000 : value));
}

static const char *modeName() {
  if (mode == MODE_TRACKING) return trackRate == RATE_LUNAR ? "Mond" : "Nachführung";
  if (mode == MODE_HOMING) return "Homing";
  if (mode == MODE_JOG) return "Manuelles Verfahren";
  return "Bereit";
}

static const char *phaseName() {
  switch (homePhase) {
    case HOME_FAST: return "Schnellfahrt";
    case HOME_BACKOFF: return "Freifahren";
    case HOME_SLOW: return "Feinsuche";
    default: return "–";
  }
}

static uint8_t motorDirectionLevel(bool forward) {
  return (forward ^ MOTOR_DIRECTION_INVERTED) ? HIGH : LOW;
}

static Endstops readEndstops() {
  return {digitalRead(PIN_ENDSTOP_HOME) == LOW,
          digitalRead(PIN_ENDSTOP_END) == LOW};
}

static void setEvent(const String &message) {
  lastEvent = message;
  Serial.println(message);
}

static void loadSettings() {
  preferences.begin("eq-platform", true);
  config.potEnabled = preferences.getBool("pot", true);
  config.trimMinPct = preferences.getFloat("trimMin", 80.0f);
  config.trimMaxPct = preferences.getFloat("trimMax", 120.0f);
  config.currentTrackingMa = preferences.getUShort("curTrack", 1100);
  config.currentHomingFastMa = preferences.getUShort("curFast", 1200);
  config.currentHomingSlowMa = preferences.getUShort("curSlow", 1100);
  config.homeFastMultiplier = preferences.getFloat("homeFast", 350.0f);
  config.homeBackoffMultiplier = preferences.getFloat("homeBack", 160.0f);
  config.homeSlowMultiplier = preferences.getFloat("homeSlow", 50.0f);
  preferences.end();

  // Also sanitize values restored from older or interrupted writes.
  config.trimMinPct = clampFloat(config.trimMinPct, 50.0f, 100.0f);
  config.trimMaxPct = clampFloat(config.trimMaxPct, 100.0f, 150.0f);
  config.currentTrackingMa = clampCurrent(config.currentTrackingMa);
  config.currentHomingFastMa = clampCurrent(config.currentHomingFastMa);
  config.currentHomingSlowMa = clampCurrent(config.currentHomingSlowMa);
  config.homeFastMultiplier = clampFloat(config.homeFastMultiplier, 20.0f, 500.0f);
  config.homeBackoffMultiplier = clampFloat(config.homeBackoffMultiplier, 20.0f, 300.0f);
  config.homeSlowMultiplier = clampFloat(config.homeSlowMultiplier, 5.0f, 150.0f);
}

static void saveSettings() {
  preferences.begin("eq-platform", false);
  preferences.putBool("pot", config.potEnabled);
  preferences.putFloat("trimMin", config.trimMinPct);
  preferences.putFloat("trimMax", config.trimMaxPct);
  preferences.putUShort("curTrack", config.currentTrackingMa);
  preferences.putUShort("curFast", config.currentHomingFastMa);
  preferences.putUShort("curSlow", config.currentHomingSlowMa);
  preferences.putFloat("homeFast", config.homeFastMultiplier);
  preferences.putFloat("homeBack", config.homeBackoffMultiplier);
  preferences.putFloat("homeSlow", config.homeSlowMultiplier);
  preferences.end();
}

static float readTrim() {
  if (!config.potEnabled) {
    potOk = true;
    filteredPotRaw = -1.0f;
    potRawAtWebTrim = -1.0f;
    return webTrimActive ? webTrimPct / 100.0f : 1.0f;
  }

  uint32_t sum = 0;
  int minimum = 4095;
  int maximum = 0;
  for (uint8_t i = 0; i < 12; ++i) {
    int value = analogRead(PIN_POT);
    sum += value;
    minimum = min(minimum, value);
    maximum = max(maximum, value);
  }
  const float raw = sum / 12.0f;
  potOk = (maximum - minimum) < 260 && raw > 8.0f && raw < 4087.0f;
  if (potOk) {
    if (filteredPotRaw < 0.0f) filteredPotRaw = raw;
    else filteredPotRaw += 0.12f * (raw - filteredPotRaw);

    if (webTrimActive) {
      if (potRawAtWebTrim < 0.0f) potRawAtWebTrim = filteredPotRaw;
      // Require a real turn (about 1.6% of ADC travel) to take over from the web.
      if (fabsf(filteredPotRaw - potRawAtWebTrim) >= 64.0f) webTrimActive = false;
    }
  }
  if (webTrimActive) return webTrimPct / 100.0f;
  if (!potOk) return 1.0f;

  const float normalized = filteredPotRaw / 4095.0f;
  const float minFactor = config.trimMinPct / 100.0f;
  const float maxFactor = config.trimMaxPct / 100.0f;
  float result = maxFactor + (minFactor - maxFactor) * normalized;
  if (fabsf(filteredPotRaw - 2048.0f) < 35.0f) result = 1.0f;
  return result;
}

static float shaftRevPerHour(float motorUsps) {
  return motorUsps * 3600.0f / (STEPS_PER_REV * MICROSTEPS * GEAR_RATIO);
}

static void setTimerSpeed(float usps) {
  if (usps < 0.1f) usps = 0.1f;
  uint64_t intervalUs = (uint64_t)(1000000.0f / usps + 0.5f);
  if (intervalUs < 12) intervalUs = 12;
  portENTER_CRITICAL(&timerMux);
  timerAlarmWrite(stepTimer, intervalUs, true);
  portEXIT_CRITICAL(&timerMux);
}

static void applyDriverProfile() {
  static uint8_t previous = 255;
  uint8_t profile = 0;
  if (mode == MODE_TRACKING || mode == MODE_JOG) profile = 1;
  if (mode == MODE_HOMING) profile = homePhase == HOME_SLOW ? 3 : 2;
  if (profile == previous) return;
  previous = profile;

  if (profile == 1) {
    driver.rms_current(config.currentTrackingMa);
  } else if (profile == 2) {
    driver.rms_current(config.currentHomingFastMa);
  } else if (profile == 3) {
    driver.rms_current(config.currentHomingSlowMa);
  }
}

static void startHoming() {
  timerRunning = false;
  mode = MODE_HOMING;
  homePhase = HOME_FAST;
  homeForward = !trackForward;
  homingDirectionFlipped = false;
  homingStartedMs = millis();
  backoffRemaining = 0;
  setEvent("Homing gestartet");
}

static void stopMotion(const String &reason) {
  mode = MODE_IDLE;
  homePhase = HOME_IDLE;
  timerRunning = false;
  digitalWrite(PIN_EN, HIGH);
  setEvent(reason);
}

static bool readButtonPressed() {
  static uint8_t stable = HIGH;
  static uint8_t last = HIGH;
  static uint32_t changedAt = 0;
  uint8_t value = digitalRead(PIN_HOME_BTN);
  if (value != last) {
    last = value;
    changedAt = millis();
  }
  if (millis() - changedAt > 25) stable = last;
  return stable == LOW;
}

static void handlePhysicalControls() {
  static uint8_t previousSwitch = 255;
  bool north = digitalRead(PIN_DIR_N) == LOW;
  bool south = digitalRead(PIN_DIR_S) == LOW;
  uint8_t switchState = north && !south ? 0 : (!north && south ? 1 : 2);

  // Only a physical change overrides the selection made in the web app.
  if (previousSwitch == 255) previousSwitch = switchState;
  if (switchState != previousSwitch && mode != MODE_HOMING) {
    previousSwitch = switchState;
    if (switchState == 0) {
      trackForward = true;
      trackRate = RATE_SIDEREAL;
      setEvent("Richtung: Nord (Schalter)");
    } else if (switchState == 1) {
      trackForward = false;
      trackRate = RATE_SIDEREAL;
      setEvent("Richtung: Süd (Schalter)");
    } else {
      trackRate = RATE_LUNAR;
      setEvent("Mondrate (Schalter Mitte)");
    }
  }

  static bool wasPressed = false;
  static bool longHandled = false;
  static uint32_t pressedAt = 0;
  bool pressed = readButtonPressed();
  if (pressed && !wasPressed) {
    pressedAt = millis();
    longHandled = false;
  }
  if (pressed && !longHandled && millis() - pressedAt >= 800) {
    startHoming();
    longHandled = true;
  }
  if (!pressed && wasPressed && !longHandled && mode != MODE_HOMING && mode != MODE_JOG) {
    mode = mode == MODE_TRACKING ? MODE_IDLE : MODE_TRACKING;
    setEvent(mode == MODE_TRACKING ? "Nachführung gestartet" : "Nachführung gestoppt");
  }
  wasPressed = pressed;
}

static const char WEB_APP[] PROGMEM = R"HTML(
<!doctype html><html lang="de"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>EQ Plattform</title><style>
:root{--ink:#edf7f5;--muted:#8ea7a3;--panel:#102321;--line:#23403c;--mint:#56e0b1;--blue:#77aaff;--danger:#ff7979;--bg:#07110f}*{box-sizing:border-box}body{margin:0;background:radial-gradient(circle at 80% 0,#173733 0,transparent 34%),var(--bg);color:var(--ink);font:15px/1.45 system-ui,-apple-system,sans-serif}.wrap{max-width:980px;margin:auto;padding:24px 18px 50px}header{display:flex;justify-content:space-between;align-items:center;margin-bottom:22px}.brand{display:flex;gap:12px;align-items:center}.logo{display:grid;place-items:center;width:46px;height:46px;border:1px solid #47736b;border-radius:14px;background:#132a27;font-size:23px}.eyebrow{color:var(--mint);font-size:11px;letter-spacing:.16em;text-transform:uppercase}.brand h1{font-size:20px;margin:2px 0}.online{display:flex;align-items:center;gap:7px;color:var(--muted);font-size:13px}.dot{width:8px;height:8px;border-radius:50%;background:var(--mint);box-shadow:0 0 12px var(--mint)}.grid{display:grid;grid-template-columns:1.35fr .65fr;gap:16px}.card{background:linear-gradient(145deg,#112623ee,#0c1d1bee);border:1px solid var(--line);border-radius:20px;padding:20px;box-shadow:0 16px 50px #0004}.hero{grid-row:span 2}.label{color:var(--muted);font-size:12px;text-transform:uppercase;letter-spacing:.1em}.mode{font-size:34px;font-weight:650;margin:7px 0 20px}.metrics{display:grid;grid-template-columns:repeat(3,1fr);gap:10px}.metric{padding:13px;background:#081816;border:1px solid #1d3834;border-radius:14px}.metric b{display:block;font-size:20px;margin-top:5px}.controls{display:flex;gap:10px;margin-top:18px}.btn{border:0;border-radius:12px;padding:12px 15px;background:#203b37;color:var(--ink);font-weight:650;cursor:pointer}.btn.primary{background:var(--mint);color:#082019;flex:1}.btn.danger{color:#ffc1c1}.btn:active{transform:translateY(1px)}.seg{display:grid;grid-template-columns:repeat(3,1fr);gap:5px;padding:5px;background:#081816;border-radius:13px;margin-top:10px}.seg button{border:0;border-radius:9px;background:transparent;color:var(--muted);padding:10px 4px;cursor:pointer}.seg button.active{background:#24453f;color:white}.stoprow{display:flex;gap:9px;margin-top:12px}.stop{flex:1;padding:10px;border-radius:12px;background:#081816;color:var(--muted)}.stop.on{color:#ffaeae;border:1px solid #753b3b}.event{color:var(--muted);margin-top:14px;font-size:13px}.settings{grid-column:1/-1}.settings-head{display:flex;justify-content:space-between;align-items:center}.settings h2{font-size:18px;margin:0}.form{display:grid;grid-template-columns:repeat(3,1fr);gap:14px;margin-top:18px}.group{padding:15px;border:1px solid var(--line);border-radius:15px}.group h3{font-size:14px;margin:0 0 13px}.field{margin:10px 0}.field label{display:flex;justify-content:space-between;color:var(--muted);font-size:13px}.field input[type=number]{width:100%;margin-top:6px;background:#071512;border:1px solid #294942;color:white;padding:10px;border-radius:9px}.switch{display:flex;gap:9px;align-items:center;color:var(--muted)}.save{margin-top:14px;background:var(--blue);color:#071221}.toast{position:fixed;right:18px;bottom:18px;background:#e9fff8;color:#09231d;padding:12px 16px;border-radius:12px;opacity:0;transform:translateY(10px);transition:.2s}.toast.show{opacity:1;transform:none}@media(max-width:720px){.grid{grid-template-columns:1fr}.hero{grid-row:auto}.settings{grid-column:auto}.form{grid-template-columns:1fr}.metrics{grid-template-columns:1fr 1fr}.mode{font-size:29px}}
#trimSlider{width:100%;accent-color:var(--mint)}
[data-jog]{touch-action:none;user-select:none}
.wifi-form{display:grid;gap:10px;max-width:400px}.wifi-form select,.wifi-form input{width:100%;padding:10px;border-radius:9px;border:1px solid #294942;background:#071512;color:var(--ink)}
</style></head>
<body><div class="wrap"><header><div class="brand"><div class="logo">✦</div><div><div class="eyebrow">Observatory control</div><h1>EQ Plattform</h1></div></div><div class="online"><i class="dot"></i><span id="connection">Verbunden</span></div></header>
<main class="grid"><section class="card hero"><div class="label">Aktueller Modus</div><div class="mode" id="mode">–</div><div class="metrics"><div class="metric"><span class="label">Richtung</span><b id="direction">–</b></div><div class="metric"><span class="label">Trim</span><b id="trim">–</b></div><div class="metric"><span class="label">Welle</span><b id="speed">–</b></div></div><div class="controls"><button class="btn primary" id="tracking" type="button" data-control="toggle">Nachführung starten</button><button class="btn" type="button" data-control="home">Homing</button><button class="btn danger" type="button" data-control="stop">Stopp</button></div>
<div class="controls" role="group" aria-label="Motor manuell verfahren">
<button type="button" class="btn" aria-label="Motor nach links" data-jog="left">←</button>
<button type="button" class="btn" aria-label="Motor nach rechts" data-jog="right">→</button>
<label class="switch">Geschwindigkeit
<select id="jogSpeed" class="btn">
<option value="slow">Langsam</option>
<option value="normal" selected>Normal</option>
<option value="fast">Schnell</option>
</select></label>
</div>
<div class="field">
<label for="trimSlider">Trim-Regler <output id="trimValue" for="trimSlider">100 %</output></label>
<input id="trimSlider" type="range" min="80" max="120" step="0.1" value="100" disabled>
<div class="event" id="trimSource"></div>
<div class="event" id="trimError" role="status" hidden></div>
</div>
<div class="event" id="controlStatus" role="status" hidden></div>
<div class="event" id="jogStatus" role="status"></div>
<div class="event" id="event">–</div></section>
<section class="card"><div class="label">Nachführmodus</div><div class="seg"><button id="north" type="button" data-control="direction" data-value="north">Nord</button><button id="lunar" type="button" data-control="direction" data-value="lunar">Mond</button><button id="south" type="button" data-control="direction" data-value="south">Süd</button></div></section>
<section class="card"><div class="label">Endschalter</div><div class="stoprow"><div class="stop" id="stopHome">Home frei</div><div class="stop" id="stopEnd">Ende frei</div></div><div class="event"><span id="clients">0</span> Gerät(e) im Hotspot · <span id="uptime">0 min</span></div></section>
<section class="card settings"><div class="settings-head"><div><div class="label">Konfiguration</div><h2>Feinabstimmung</h2></div><span class="label">wird im ESP32 gespeichert</span></div><form id="settings" class="form"><div class="group"><h3>Potentiometer</h3><label class="switch"><input name="pot" type="checkbox"> physischen Poti verwenden</label><div class="field"><label>Minimum <span>%</span></label><input name="trimMin" type="number" min="50" max="100" step="0.5"></div><div class="field"><label>Maximum <span>%</span></label><input name="trimMax" type="number" min="100" max="150" step="0.5"></div></div><div class="group"><h3>Motorstrom (RMS)</h3><div class="field"><label>Tracking <span>mA</span></label><input name="curTrack" type="number" min="100" max="2000" step="50"></div><div class="field"><label>Homing schnell <span>mA</span></label><input name="curFast" type="number" min="100" max="2000" step="50"></div><div class="field"><label>Homing langsam <span>mA</span></label><input name="curSlow" type="number" min="100" max="2000" step="50"></div></div><div class="group"><h3>Homing-Geschwindigkeit</h3><div class="field"><label>Schnellfahrt <span>× siderisch</span></label><input name="homeFast" type="number" min="20" max="500" step="5"></div><div class="field"><label>Freifahren <span>× siderisch</span></label><input name="homeBack" type="number" min="20" max="300" step="5"></div><div class="field"><label>Feinsuche <span>× siderisch</span></label><input name="homeSlow" type="number" min="5" max="150" step="5"></div></div></form><button class="btn save" onclick="saveSettings()">Einstellungen speichern</button></section>
<section class="card settings"><h2>Heim-WLAN</h2>
<p id="wifiStatus" role="status">WLAN-Status wird geladen …</p>
<p id="wifiHint">Zum Einrichten mit dem Hotspot EQ-Plattform verbinden.</p>
<button class="btn" id="wifiScan" type="button" hidden>Nach WLAN suchen</button>
<form id="wifiForm" hidden>
<div class="wifi-form">
<label>WLAN auswählen <select name="ssid" required></select></label>
<label id="wifiPassword" hidden>WLAN-Passwort <input name="password" type="password" maxlength="64" autocomplete="new-password"></label>
<button class="btn save" type="submit">Verbinden</button>
</div>
</form>
<p id="wifiMessage" role="status"></p><p id="wifiError" role="status"></p>
</section>
<section class="card settings"><h2>Firmware aktualisieren</h2>
<form id="firmware">
<p>Plattform anhalten und eine ESP32-OTA-Datei (.bin) auswählen.</p>
<input name="image" type="file" accept=".bin" aria-label="ESP32-OTA-Datei" required>
<button class="btn save" type="submit">Update installieren</button>
</form>
<p id="firmwareStatus" role="status"></p></section></main></div><div class="toast" id="toast">Gespeichert</div>
<script>
const $=id=>document.getElementById(id), form=$('settings');let hydrated=false;
const wifiForm = $('wifiForm'), wifiScan = $('wifiScan');
const wifiNetwork = wifiForm.elements.ssid, wifiPassword = wifiForm.elements.password;
let wifiBusy = false;
function showWifi(wifi) {
  wifiScan.hidden = !wifi.setup;
  $('wifiHint').hidden = wifi.setup;
  const status = $('wifiStatus');
  status.textContent = wifi.connecting ? 'Verbindung wird aufgebaut …' :
    wifi.ip ? 'Heimnetz-Adresse: ' : 'Kein Heim-WLAN verbunden.';
  if (wifi.ip && !wifi.connecting) {
    const link = document.createElement('a');
    link.href = link.textContent = 'http://' + wifi.ip;
    status.append(link);
  }
  $('wifiError').textContent = wifi.error;
}
async function wifiPost(path, body = new URLSearchParams()) {
  const response = await fetch(path, {method: 'POST', headers: {'X-EQ-Wifi': '1'}, body});
  if (!response.ok) throw Error(await response.text());
  return response.json();
}
wifiScan.addEventListener('click', async () => {
  wifiBusy = wifiScan.disabled = true;
  wifiForm.hidden = true;
  $('wifiMessage').textContent = 'Suche läuft …';
  try {
    await wifiPost('/api/wifi/scan');
    for (let attempt = 0; attempt < 35; attempt++) {
      await new Promise(resolve => setTimeout(resolve, 450));
      const result = await api('/api/wifi/scan', {cache: 'no-store'});
      if (result.scanning) continue;
      wifiNetwork.replaceChildren(new Option('Bitte auswählen', ''));
      for (const network of result.networks) {
        if ([...wifiNetwork.options].some(option => option.value === network.ssid)) continue;
        const option = new Option(network.ssid + ' (' + network.rssi + ' dBm)', network.ssid);
        option.dataset.secure = network.secure ? '1' : '0';
        wifiNetwork.add(option);
      }
      wifiNetwork.dispatchEvent(new Event('change'));
      wifiForm.hidden = !result.networks.length;
      $('wifiMessage').textContent = result.networks.length ? '' : 'Keine 2,4-GHz-Netze gefunden.';
      return;
    }
    throw Error('WLAN-Suche hat zu lange gedauert.');
  } catch (error) {
    $('wifiMessage').textContent = error.message;
  } finally {
    wifiBusy = wifiScan.disabled = false;
  }
});
wifiNetwork.addEventListener('change', () => {
  const secure = wifiNetwork.selectedOptions[0]?.dataset.secure === '1';
  $('wifiPassword').hidden = wifiPassword.disabled = !secure;
  wifiPassword.required = secure;
  wifiPassword.value = '';
});
wifiForm.addEventListener('submit', async event => {
  event.preventDefault();
  if (wifiBusy) return;
  const button = wifiForm.querySelector('button');
  wifiBusy = wifiScan.disabled = button.disabled = true;
  try {
    await wifiPost('/api/wifi/connect', new URLSearchParams(new FormData(wifiForm)));
    wifiPassword.value = '';
    wifiForm.hidden = true;
    $('wifiMessage').textContent = '';
  } catch (error) {
    $('wifiMessage').textContent = error.message;
  } finally {
    wifiBusy = wifiScan.disabled = button.disabled = false;
  }
});
async function api(path,options){const r=await fetch(path,options);if(!r.ok)throw Error(await r.text());return r.json()}
const trimSlider = $('trimSlider');
let pendingTrim = null, sendingTrim = false, trimRevision = 0;

function showTrim(status, revision) {
  if (sendingTrim || revision !== trimRevision) return;
  trimSlider.min = status.settings.trimMin;
  trimSlider.max = status.settings.trimMax;
  trimSlider.value = status.trimTarget;
  trimSlider.disabled = false;
  $('trimValue').textContent = trimSlider.value + ' %';
  $('trimSource').textContent = status.trimSource === 'web' ? 'Web-Regler aktiv' :
    status.trimSource === 'pot' ? 'Physisches Poti aktiv' : 'Standardwert aktiv';
}

trimSlider.addEventListener('input', async () => {
  pendingTrim = trimSlider.value;
  trimRevision++;
  $('trimValue').textContent = pendingTrim + ' %';
  $('trimError').hidden = true;
  if (sendingTrim) return;
  sendingTrim = true;
  try {
    while (pendingTrim !== null) {
      const value = pendingTrim;
      pendingTrim = null;
      await api('/api/trim', {method: 'POST', body: new URLSearchParams({value})});
      $('trimSource').textContent = 'Web-Regler aktiv';
    }
  } catch {
    pendingTrim = null;
    $('trimError').textContent = 'Trim konnte nicht übertragen werden.';
    $('trimError').hidden = false;
  } finally {
    sendingTrim = false;
    trimRevision++;
    refresh();
  }
});

function selected(id,on){$(id).classList.toggle('active',on)}
async function refresh(){const revision=trimRevision;try{const s=await api('/api/status');showTrim(s,revision);showWifi(s.wifi);$('connection').textContent='Verbunden';$('mode').textContent=s.mode+(s.phase!=='–'?' · '+s.phase:'');$('direction').textContent=s.direction;$('trim').textContent=s.trim.toFixed(1)+' %';$('speed').textContent=s.revPerHour.toFixed(2)+' rev/h';$('tracking').textContent=s.tracking?'Nachführung stoppen':'Nachführung starten';$('event').textContent=s.event;$('clients').textContent=s.clients;$('uptime').textContent=Math.floor(s.uptime/60)+' min';$('stopHome').textContent=s.homeStop?'Home aktiv':'Home frei';$('stopEnd').textContent=s.endStop?'Ende aktiv':'Ende frei';$('stopHome').classList.toggle('on',s.homeStop);$('stopEnd').classList.toggle('on',s.endStop);selected('north',s.rate==='sidereal'&&s.direction==='Nord');selected('south',s.rate==='sidereal'&&s.direction==='Süd');selected('lunar',s.rate==='lunar');if(!hydrated){Object.entries(s.settings).forEach(([k,v])=>{if(form.elements[k])form.elements[k].type==='checkbox'?form.elements[k].checked=v:form.elements[k].value=v});hydrated=true}}catch(e){$('connection').textContent='Verbindung getrennt'}}
async function requestControl(action, fields = {}, keepalive = false) {
  const response = await fetch('/api/control', {
    method: 'POST',
    keepalive,
    body: new URLSearchParams({action, ...fields})
  });
  if (!response.ok) {
    const result = await response.json();
    throw Error(result.error || 'Befehl konnte nicht ausgeführt werden.');
  }
}
for (const button of document.querySelectorAll('[data-control]')) {
  button.addEventListener('click', async () => {
    const status = $('controlStatus');
    status.hidden = true;
    try {
      const {control, value} = button.dataset;
      await requestControl(control, value ? {value} : {});
      await refresh();
    } catch (error) {
      status.textContent = error instanceof TypeError ? 'Keine Verbindung zum ESP32.' : error.message;
      status.hidden = false;
    }
  });
}

let activeJog = null;
function jogRequest(action, jog) {
  return requestControl(action, {owner: jog.owner, value: jog.side, speed: jog.speed}, true);
}
function endJog() {
  const jog = activeJog;
  if (!jog) return;
  activeJog = null;
  clearTimeout(jog.timer);
  $('jogStatus').textContent = '';
  // Send stop after any in-flight start or renewal, so it cannot overtake it.
  jog.request.finally(() => jogRequest('jogStop', jog)).catch(() => {});
}
function startJog(button, input) {
  if (activeJog) return;
  const jog = {button, input, side: button.dataset.jog, speed: $('jogSpeed').value,
    owner: Array.from(crypto.getRandomValues(new Uint32Array(2)), n => n.toString(16).padStart(8, '0')).join('')};
  activeJog = jog;
  $('jogStatus').textContent = 'Motor fährt …';
  function send(action) {
    jog.request = jogRequest(action, jog).then(() => {
      if (activeJog === jog) jog.timer = setTimeout(() => send('jogKeep'), 120);
    }).catch(error => {
      if (activeJog === jog) {
        endJog();
        $('jogStatus').textContent = error instanceof TypeError ? 'Keine Verbindung zum ESP32.' : error.message;
      }
    });
  }
  send('jogStart');
}
for (const button of document.querySelectorAll('[data-jog]')) {
  button.addEventListener('pointerdown', event => {
    if (event.button !== 0 || activeJog) return;
    button.setPointerCapture(event.pointerId);
    startJog(button, event.pointerId);
  });
  for (const type of ['pointerup', 'pointercancel', 'lostpointercapture']) {
    button.addEventListener(type, event => {
      if (activeJog?.button === button && activeJog.input === event.pointerId) endJog();
    });
  }
  button.addEventListener('contextmenu', event => event.preventDefault());
  button.addEventListener('keydown', event => {
    if (event.key !== ' ' && event.key !== 'Enter') return;
    event.preventDefault();
    if (!event.repeat) startJog(button, event.key);
  });
  button.addEventListener('keyup', event => {
    if (activeJog?.button === button && activeJog.input === event.key) endJog();
  });
  button.addEventListener('blur', () => { if (activeJog?.button === button) endJog(); });
}
window.addEventListener('blur', endJog);
window.addEventListener('pagehide', endJog);
document.addEventListener('visibilitychange', () => { if (document.hidden) endJog(); });
async function saveSettings(){const data=new URLSearchParams(new FormData(form));if(!form.elements.pot.checked)data.set('pot','0');await api('/api/settings',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:data});const t=$('toast');t.classList.add('show');setTimeout(()=>t.classList.remove('show'),1800);refresh()}
const firmwareForm = $('firmware');
let firmwareUpdating = false;
firmwareForm.addEventListener('submit', async event => {
  event.preventDefault();
  const file = firmwareForm.elements.image.files[0];
  if (firmwareUpdating || !file) return;

  const button = firmwareForm.querySelector('button');
  const status = $('firmwareStatus');
  firmwareUpdating = button.disabled = true;
  try {
    const current = await api('/api/status');
    if (current.mode !== 'Bereit') {
      status.textContent = 'Nachführung oder Homing vor dem Update stoppen.';
      firmwareUpdating = button.disabled = false;
      return;
    }
    status.textContent = 'Firmware wird übertragen. ESP32 eingeschaltet lassen.';
    const response = await fetch('/api/firmware', {
      method: 'POST',
      headers: {'X-Firmware-Size': String(file.size)},
      body: new FormData(firmwareForm)
    });
    const message = await response.text();
    status.textContent = message;
    if (response.ok) {
      firmwareForm.reset();
      setTimeout(() => {
        firmwareUpdating = button.disabled = false;
        hydrated = false;
        refresh();
      }, 5000);
      return;
    }
  } catch {
    status.textContent = 'Verbindung unterbrochen. Nach dem Wiederverbinden den Status prüfen.';
  }
  firmwareUpdating = button.disabled = false;
});
refresh();setInterval(() => { if (!firmwareUpdating) refresh(); },1000);
</script></body></html>)HTML";

static void appendJsonString(String &json, const String &value) {
  json += '"';
  for (unsigned char c : value) {
    if (c == '"' || c == '\\') {
      json += '\\';
      json += static_cast<char>(c);
    } else if (c < 32) {
      char escaped[7];
      snprintf(escaped, sizeof(escaped), "\\u%04x", static_cast<unsigned>(c));
      json += escaped;
    } else {
      json += static_cast<char>(c);
    }
  }
  json += '"';
}

static bool viaHotspot() {
  return server.client().localIP() == WiFi.softAPIP();
}

static bool allowWifiSetup(bool write) {
  if (viaHotspot() && (!write || server.header("X-EQ-Wifi") == "1")) return true;
  server.send(403, "text/plain; charset=utf-8", "WLAN-Einrichtung nur über den Plattform-Hotspot.");
  return false;
}

static void startWifiConnection(const WifiProfile &profile) {
  WiFi.disconnect();
  wifiAttemptMs = millis();
  if (profile.ssid[0]) WiFi.begin(profile.ssid, profile.password);
}

static void updateWifiConnection() {
  if (pendingWifi.ssid[0]) {
    if (WiFi.status() == WL_CONNECTED && WiFi.SSID() == pendingWifi.ssid &&
        WiFi.localIP() != IPAddress(0, 0, 0, 0)) {
      Preferences storage;
      if (storage.begin("eq-wifi", false) &&
          storage.putBytes("profile", &pendingWifi, sizeof(pendingWifi)) == sizeof(pendingWifi)) {
        savedWifi = pendingWifi;
      } else {
        wifiError = "Verbunden, aber das WLAN-Profil konnte nicht gespeichert werden.";
      }
      pendingWifi = {};
    } else if (millis() - wifiAttemptMs >= WIFI_CONNECT_TIMEOUT_MS) {
      pendingWifi = {};
      wifiError = "Verbindung fehlgeschlagen. Passwort und WLAN-Empfang prüfen.";
      startWifiConnection(savedWifi);
    }
  } else if (savedWifi.ssid[0] && WiFi.status() != WL_CONNECTED &&
             millis() - wifiAttemptMs >= WIFI_RETRY_MS && WiFi.scanComplete() != WIFI_SCAN_RUNNING) {
    startWifiConnection(savedWifi);
  }
}

static void startWifiScan() {
  if (!allowWifiSetup(true)) return;
  if (pendingWifi.ssid[0]) {
    server.send(409, "text/plain; charset=utf-8", "Bitte den Verbindungsversuch abwarten.");
    return;
  }
  wifiError = "";
  if (WiFi.scanNetworks(true) != WIFI_SCAN_RUNNING) {
    server.send(503, "text/plain; charset=utf-8", "WLAN-Suche konnte nicht gestartet werden. Bitte erneut versuchen.");
    return;
  }
  server.send(202, "application/json", "{}");
}

static void sendWifiScan() {
  if (!allowWifiSetup(false)) return;
  const int16_t count = WiFi.scanComplete();
  if (count == WIFI_SCAN_RUNNING) {
    server.send(200, "application/json", "{\"scanning\":true}");
    return;
  }
  if (count < 0) {
    server.send(503, "text/plain; charset=utf-8", "WLAN-Suche fehlgeschlagen.");
    return;
  }
  String json = "{\"networks\":[";
  for (int16_t i = 0; i < count; ++i) {
    if (i) json += ',';
    json += F("{\"ssid\":");
    appendJsonString(json, WiFi.SSID(i));
    json += F(",\"rssi\":"); json += WiFi.RSSI(i);
    json += F(",\"secure\":"); json += WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "false}" : "true}";
  }
  json += F("]}");
  server.send(200, "application/json", json);
}

static void handleWifiConnect() {
  if (!allowWifiSetup(true)) return;
  if (pendingWifi.ssid[0] || WiFi.scanComplete() == WIFI_SCAN_RUNNING) {
    server.send(409, "text/plain; charset=utf-8", "Bitte die laufende WLAN-Anfrage abwarten.");
    return;
  }
  const String ssid = server.arg("ssid");
  const String password = server.arg("password");
  if (!ssid.length() || ssid.length() >= sizeof(pendingWifi.ssid) ||
      strlen(ssid.c_str()) != ssid.length() ||
      password.length() >= sizeof(pendingWifi.password) ||
      strlen(password.c_str()) != password.length() ||
      (password.length() && password.length() < 8)) {
    server.send(400, "text/plain; charset=utf-8", "Ungültiger WLAN-Name oder ungültige Passwortlänge.");
    return;
  }
  ssid.toCharArray(pendingWifi.ssid, sizeof(pendingWifi.ssid));
  password.toCharArray(pendingWifi.password, sizeof(pendingWifi.password));
  wifiError = "";
  server.send(202, "application/json", "{}");
  startWifiConnection(pendingWifi);
}

static void sendJsonStatus() {
  Endstops stops = readEndstops();
  String json;
  json.reserve(850);
  json += F("{\"mode\":\""); json += modeName();
  json += F("\",\"phase\":\""); json += phaseName();
  json += F("\",\"tracking\":"); json += mode == MODE_TRACKING ? "true" : "false";
  json += F(",\"direction\":\""); json += trackForward ? "Nord" : "Süd";
  json += F("\",\"rate\":\""); json += trackRate == RATE_LUNAR ? "lunar" : "sidereal";
  json += F("\",\"trim\":"); json += String(smoothedTrim * 100.0f, 1);
  json += F(",\"revPerHour\":"); json += String(shaftRevPerHour(targetUsps), 3);
  json += F(",\"homeStop\":"); json += stops.home ? "true" : "false";
  json += F(",\"endStop\":"); json += stops.end ? "true" : "false";
  json += F(",\"trimTarget\":"); json += String(trimTarget * 100.0f, 1);
  json += F(",\"trimSource\":\"");
  json += webTrimActive ? "web" : (config.potEnabled && potOk ? "pot" : "default");
  json += '"';
  json += F(",\"potOk\":"); json += potOk ? "true" : "false";
  json += F(",\"clients\":"); json += WiFi.softAPgetStationNum();
  json += F(",\"uptime\":"); json += millis() / 1000UL;
  json += F(",\"event\":\""); json += lastEvent;
  json += F("\",\"settings\":{\"pot\":"); json += config.potEnabled ? "true" : "false";
  json += F(",\"trimMin\":"); json += String(config.trimMinPct, 1);
  json += F(",\"trimMax\":"); json += String(config.trimMaxPct, 1);
  json += F(",\"curTrack\":"); json += config.currentTrackingMa;
  json += F(",\"curFast\":"); json += config.currentHomingFastMa;
  json += F(",\"curSlow\":"); json += config.currentHomingSlowMa;
  json += F(",\"homeFast\":"); json += String(config.homeFastMultiplier, 0);
  json += F(",\"homeBack\":"); json += String(config.homeBackoffMultiplier, 0);
  json += F(",\"homeSlow\":"); json += String(config.homeSlowMultiplier, 0);
  json += F("},\"wifi\":{\"setup\":"); json += viaHotspot() ? "true" : "false";
  json += F(",\"connecting\":"); json += pendingWifi.ssid[0] ? "true" : "false";
  json += F(",\"ip\":");
  appendJsonString(json, WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "");
  json += F(",\"error\":"); appendJsonString(json, wifiError);
  json += F("}}");
  server.send(200, "application/json", json);
}

static void handleControl() {
  String action = server.arg("action");
  if (action == "jogStop") {
    if (mode == MODE_JOG && jogOwner == server.arg("owner")) stopMotion("Manuelles Verfahren gestoppt");
  } else if (action == "jogKeep") {
    const TickType_t now = xTaskGetTickCount();
    if (mode != MODE_JOG || jogEndstopHit || jogOwner != server.arg("owner") ||
        (int32_t)(now - jogDeadline) >= 0) {
      server.send(409, "application/json", "{\"ok\":false,\"error\":\"Manuelles Verfahren beendet.\"}");
      return;
    }
    jogDeadline = now + JOG_TIMEOUT_TICKS;
  } else if (action == "jogStart") {
    if (mode != MODE_IDLE) {
      server.send(409, "application/json", "{\"ok\":false,\"error\":\"Plattform zuerst stoppen.\"}");
      return;
    }
    const String owner = server.arg("owner");
    if (owner.length() != 16) {
      server.send(400, "application/json", "{\"ok\":false}");
      return;
    }

    const String side = server.arg("value");
    const String speed = server.arg("speed");
    if ((side != "left" && side != "right") ||
        (speed != "slow" && speed != "normal" && speed != "fast")) {
      server.send(400, "application/json", "{\"ok\":false,\"error\":\"Ungültige Richtung oder Geschwindigkeit.\"}");
      return;
    }

    const bool forward = side == "right";
    const bool towardHome = forward == homeForward;
    const Endstops stops = readEndstops();
    if (towardHome ? stops.home : stops.end) {
      server.send(409, "application/json", "{\"ok\":false,\"error\":\"Endschalter in dieser Richtung aktiv.\"}");
      return;
    }

    jogOwner = owner;
    jogForward = forward;
    jogEndstopHit = false;
    jogMultiplier = speed == "slow" ? 25.0f : (speed == "fast" ? 200.0f : 100.0f);
    jogDeadline = xTaskGetTickCount() + JOG_TIMEOUT_TICKS;
    mode = MODE_JOG;
    setEvent(forward ? "Motor fährt nach rechts" : "Motor fährt nach links");
  } else if (action == "toggle" && mode != MODE_HOMING && mode != MODE_JOG) {
    mode = mode == MODE_TRACKING ? MODE_IDLE : MODE_TRACKING;
    setEvent(mode == MODE_TRACKING ? "Nachführung per Web gestartet" : "Nachführung per Web gestoppt");
  } else if (action == "stop") {
    stopMotion("Stopp per Web");
  } else if (action == "home" && mode != MODE_JOG) {
    startHoming();
  } else if (action == "direction" && mode != MODE_HOMING) {
    String value = server.arg("value");
    if (value == "north") {
      trackForward = true; trackRate = RATE_SIDEREAL; setEvent("Richtung: Nord (Web)");
    } else if (value == "south") {
      trackForward = false; trackRate = RATE_SIDEREAL; setEvent("Richtung: Süd (Web)");
    } else if (value == "lunar") {
      trackRate = RATE_LUNAR; setEvent("Mondrate (Web)");
    } else {
      server.send(400, "application/json", "{\"ok\":false}"); return;
    }
  } else {
    server.send(409, "application/json", "{\"ok\":false,\"error\":\"Befehl nicht möglich\"}"); return;
  }
  server.send(200, "application/json", "{\"ok\":true}");
}

static float argFloat(const char *name, float fallback) {
  return server.hasArg(name) ? server.arg(name).toFloat() : fallback;
}

static int argInt(const char *name, int fallback) {
  return server.hasArg(name) ? server.arg(name).toInt() : fallback;
}

static void handleTrim() {
  const String value = server.arg("value");
  char *end = nullptr;
  const float percent = strtof(value.c_str(), &end);
  if (!value.length() || end != value.c_str() + value.length() || !isfinite(percent)) {
    server.send(400, "application/json", "{\"ok\":false}");
    return;
  }
  webTrimPct = clampFloat(percent, config.trimMinPct, config.trimMaxPct);
  potRawAtWebTrim = filteredPotRaw;
  webTrimActive = true;
  trimTarget = webTrimPct / 100.0f;
  server.send(200, "application/json", "{\"ok\":true}");
}

static void handleSettings() {
  config.potEnabled = server.arg("pot") != "0";
  config.trimMinPct = clampFloat(argFloat("trimMin", config.trimMinPct), 50.0f, 100.0f);
  config.trimMaxPct = clampFloat(argFloat("trimMax", config.trimMaxPct), 100.0f, 150.0f);
  config.currentTrackingMa = clampCurrent(argInt("curTrack", config.currentTrackingMa));
  config.currentHomingFastMa = clampCurrent(argInt("curFast", config.currentHomingFastMa));
  config.currentHomingSlowMa = clampCurrent(argInt("curSlow", config.currentHomingSlowMa));
  config.homeFastMultiplier = clampFloat(argFloat("homeFast", config.homeFastMultiplier), 20.0f, 500.0f);
  config.homeBackoffMultiplier = clampFloat(argFloat("homeBack", config.homeBackoffMultiplier), 20.0f, 300.0f);
  config.homeSlowMultiplier = clampFloat(argFloat("homeSlow", config.homeSlowMultiplier), 5.0f, 150.0f);
  webTrimPct = clampFloat(webTrimPct, config.trimMinPct, config.trimMaxPct);
  saveSettings();
  setEvent("Einstellungen gespeichert");
  uint16_t activeCurrent = config.currentTrackingMa;
  if (mode == MODE_HOMING) {
    activeCurrent = homePhase == HOME_SLOW
      ? config.currentHomingSlowMa
      : config.currentHomingFastMa;
  }
  driver.rms_current(activeCurrent);
  server.send(200, "application/json", "{\"ok\":true}");
}

// Multipart callbacks run synchronously inside handleClient(), before the POST handler.
static constexpr char FIRMWARE_SIZE_HEADER[] = "X-Firmware-Size";
static size_t firmwareSize = 0;
static int firmwareStatus = 400;
static const char *firmwareMessage = "ESP32-OTA-Datei fehlt.";

static void failFirmwareUpload(int status, const char *message) {
  if (Update.isRunning()) Update.abort();
  firmwareStatus = status;
  firmwareMessage = message;
}

static void handleFirmwareUpload() {
  HTTPUpload &upload = server.upload();
  switch (upload.status) {
    case UPLOAD_FILE_START: {
      if (mode != MODE_IDLE) {
        // Stop reading the request so motion and endstop handling resume immediately.
        server.send(409, "text/plain; charset=utf-8",
                    "Nachführung oder Homing vor dem Update stoppen.");
        shutdown(server.client().fd(), SHUT_RDWR);
        return;
      }
      if (Update.isRunning()) {
        failFirmwareUpload(400, "Bitte genau eine OTA-Datei senden.");
        return;
      }
      // The custom header prevents cross-origin HTML forms from starting an update.
      const long size = server.header(FIRMWARE_SIZE_HEADER).toInt();
      if (size <= 0 || upload.name != "image" || !upload.filename.endsWith(".bin")) {
        failFirmwareUpload(400, "Eine ESP32-OTA-Datei (.bin) mit gültiger Größe auswählen.");
        return;
      }
      firmwareSize = static_cast<size_t>(size);
      if (!Update.begin(firmwareSize, U_FLASH)) {
        Update.printError(Serial);
        failFirmwareUpload(400, "Datei zu groß oder OTA-Partition nicht verfügbar.");
        return;
      }
      stopMotion("Firmware-Update läuft");
      firmwareStatus = 400;
      firmwareMessage = "Firmware-Upload unvollständig.";
      break;
    }
    case UPLOAD_FILE_WRITE:
      if (Update.isRunning() && Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
        failFirmwareUpload(400, "Ungültige OTA-Datei oder Schreibfehler.");
      }
      break;
    case UPLOAD_FILE_END:
      if (Update.isRunning() && upload.totalSize == firmwareSize) {
        firmwareStatus = 200;
      }
      break;
    case UPLOAD_FILE_ABORTED:
      failFirmwareUpload(400, "Firmware-Upload abgebrochen.");
      break;
  }
}

static void finishFirmwareUpload() {
  if (firmwareStatus == 200 && !Update.end()) {
    Update.printError(Serial);
    failFirmwareUpload(400, "Firmware konnte nicht aktiviert werden.");
  }
  if (firmwareStatus == 200) {
    server.send(200, "text/plain; charset=utf-8",
                "Firmware installiert. Der ESP32 startet neu. Bei Bedarf erneut mit EQ-Plattform verbinden.");
    delay(500);
    ESP.restart();
    return;
  }
  server.send(firmwareStatus, "text/plain; charset=utf-8", firmwareMessage);
}

static void setupWebServer() {
  const char *headers[] = {FIRMWARE_SIZE_HEADER, "X-EQ-Wifi"};
  server.collectHeaders(headers, 2);
  server.on("/api/wifi/scan", HTTP_POST, startWifiScan);
  server.on("/api/wifi/scan", HTTP_GET, sendWifiScan);
  server.on("/api/wifi/connect", HTTP_POST, handleWifiConnect);
  server.on("/api/firmware", HTTP_POST, finishFirmwareUpload, handleFirmwareUpload);
  server.on("/", HTTP_GET, []() { server.send_P(200, "text/html; charset=utf-8", WEB_APP); });
  server.on("/api/status", HTTP_GET, sendJsonStatus);
  server.on("/api/control", HTTP_POST, handleControl);
  server.on("/api/settings", HTTP_POST, handleSettings);
  server.on("/api/trim", HTTP_POST, handleTrim);
  server.onNotFound([]() {
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
  });
  server.begin();
}

static void updateOled() {
  static uint32_t previousMs = 0;
  if (millis() - previousMs < 300) return;
  previousMs = millis();
  Endstops stops = readEndstops();
  char line[17];
  oled.drawString(0, 0, " EQ Platform    ");
  snprintf(line, sizeof(line), " %-14s", modeName()); oled.drawString(0, 1, line);
  snprintf(line, sizeof(line), " Dir:%-2s %s", trackForward ? "N" : "S", trackRate == RATE_LUNAR ? "LUNAR" : "SID"); oled.drawString(0, 2, line);
  snprintf(line, sizeof(line), " Trim:%3d%%%s", (int)(smoothedTrim * 100.0f + 0.5f), potOk ? "    " : " FIX"); oled.drawString(0, 3, line);
  snprintf(line, sizeof(line), " Stops H:%d E:%d", stops.home, stops.end); oled.drawString(0, 4, line);
  oled.drawString(0, 5, " IP 192.168.4.1 ");
  const String homeIp = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "";
  snprintf(line, sizeof(line), "%-16s", homeIp.c_str()); oled.drawString(0, 6, line);
}

void setup() {
  Serial.begin(115200);
  loadSettings();

  pinMode(PIN_STEP, OUTPUT);
  pinMode(PIN_DIR, OUTPUT);
  pinMode(PIN_EN, OUTPUT);
  pinMode(PIN_POT, INPUT);
  pinMode(PIN_DIR_N, INPUT_PULLUP);
  pinMode(PIN_DIR_S, INPUT_PULLUP);
  pinMode(PIN_HOME_BTN, INPUT_PULLUP);
  pinMode(PIN_ENDSTOP_HOME, INPUT_PULLUP);
  pinMode(PIN_ENDSTOP_END, INPUT_PULLUP);
  digitalWrite(PIN_EN, HIGH);
  digitalWrite(PIN_STEP, LOW);

  analogReadResolution(12);
  TMC_SERIAL.begin(115200, SERIAL_8N1, PIN_TMC_RX, PIN_TMC_TX);
  driver.begin();
  driver.pdn_disable(true);
  driver.I_scale_analog(false);
  driver.toff(4);
  driver.blank_time(24);
  driver.mstep_reg_select(true);
  driver.microsteps((uint16_t)MICROSTEPS);
  driver.intpol(true);
  driver.en_spreadCycle(false);  // Use StealthChop for tracking, jog and homing.
  driver.pwm_autoscale(true);
  driver.TPOWERDOWN(10);
  driver.rms_current(config.currentTrackingMa);

  baseUsps = (STEPS_PER_REV * MICROSTEPS / SIDEREAL_SEC)
           * (R_MM / ROLLER_R_MM) * GEAR_RATIO;

  stepTimer = timerBegin(0, 80, true); // 1 MHz
  timerAttachInterrupt(stepTimer, &onStepTimer, true);
  timerAlarmWrite(stepTimer, 100000, true);
  timerAlarmEnable(stepTimer);

  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  Wire.setClock(400000);
  oled.begin();
  oled.setI2CAddress(0x3C << 1);
  oled.setFont(u8x8_font_chroma48medium8_r);
  oled.clear();
  oled.drawString(0, 0, " EQ Platform");
  oled.drawString(0, 2, " WLAN startet...");

  WiFi.persistent(false);
  WiFi.mode(WIFI_AP_STA);
  WiFi.setAutoReconnect(false);
  WiFi.setSleep(false);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  dnsServer.start(53, "*", WiFi.softAPIP());
  setupWebServer();
  Preferences storage;
  if (storage.begin("eq-wifi", true)) {
    storage.getBytes("profile", &savedWifi, sizeof(savedWifi));
    if (savedWifi.ssid[0]) startWifiConnection(savedWifi);
  }

  Serial.println(F("=== EQ Platform ESP32 ==="));
  Serial.print(F("Hotspot: ")); Serial.println(AP_SSID);
  Serial.print(F("Web-App: http://")); Serial.println(WiFi.softAPIP());
  setEvent("Hotspot und Web-App bereit");
}

void loop() {
  dnsServer.processNextRequest();
  firmwareStatus = 400;
  firmwareMessage = "ESP32-OTA-Datei fehlt.";
  server.handleClient();
  // A malformed or interrupted request may never reach the POST handler.
  if (Update.isRunning()) Update.abort();
  updateWifiConnection();
  handlePhysicalControls();

  const uint32_t nowMs = millis();
  Endstops stops = readEndstops();
  bool forward = mode == MODE_JOG ? jogForward : trackForward;
  bool towardHome = forward == homeForward;
  bool blocked = towardHome ? stops.home : stops.end;

  const bool jogBlocked = mode == MODE_JOG && (blocked || jogEndstopHit);
  if (mode == MODE_JOG && (jogBlocked || (int32_t)(xTaskGetTickCount() - jogDeadline) >= 0)) {
    stopMotion(jogBlocked ? "Endschalter erreicht" : "Manuelles Verfahren beendet");
  }

  static bool endWasPressed = false;
  if (mode == MODE_TRACKING && stops.end && !endWasPressed) stopMotion("Endschalter erreicht");
  endWasPressed = stops.end;

  bool approach = mode == MODE_HOMING && homePhase != HOME_BACKOFF;
  bool grace = mode == MODE_HOMING && nowMs - homingStartedMs < HOMING_END_GRACE_MS;
  bool unexpectedEnd = approach && stops.end && !grace;
  if (mode == MODE_HOMING) {
    if (unexpectedEnd) {
      if (!homingDirectionFlipped) {
        timerRunning = false;
        homingDirectionFlipped = true;
        homeForward = !homeForward;
        homePhase = HOME_FAST;
        homingStartedMs = nowMs;
        setEvent("Homing-Richtung korrigiert");
      } else {
        stopMotion("Homing abgebrochen: Endschalter");
      }
    } else if (homePhase == HOME_FAST && stops.home) {
      timerRunning = false;
      homePhase = HOME_BACKOFF;
      backoffRemaining = HOME_BACKOFF_USTEPS;
      backoffDeadline = xTaskGetTickCount() + pdMS_TO_TICKS(HOME_BACKOFF_MAX_MS);
      setEvent("Home gefunden, Plattform fährt frei");
    } else if (homePhase == HOME_BACKOFF) {
      const bool backoffDone = backoffRemaining == 0 ||
        (int32_t)(xTaskGetTickCount() - backoffDeadline) >= 0;
      if (backoffDone && !stops.home) {
        timerRunning = false;
        homePhase = HOME_SLOW;
        setEvent("Homing-Feinsuche");
      } else if (backoffDone) {
        stopMotion("Homing abgebrochen: Schalter bleibt aktiv");
      }
    } else if (homePhase == HOME_SLOW && stops.home) {
      stopMotion("Homing abgeschlossen");
    }
  }

  if (mode == MODE_HOMING) {
    towardHome = homePhase != HOME_BACKOFF;
    forward = towardHome ? homeForward : !homeForward;
    blocked = towardHome ? stops.home : stops.end;
  }
  digitalWrite(PIN_DIR, motorDirectionLevel(forward));

  trimTarget = readTrim();
  smoothedTrim += 0.18f * (trimTarget - smoothedTrim);
  if (mode == MODE_HOMING) {
    if (homePhase == HOME_BACKOFF) targetUsps = baseUsps * config.homeBackoffMultiplier;
    else if (homePhase == HOME_SLOW) targetUsps = baseUsps * config.homeSlowMultiplier;
    else targetUsps = baseUsps * config.homeFastMultiplier;
  } else if (mode == MODE_JOG) {
    targetUsps = baseUsps * jogMultiplier;
  } else {
    targetUsps = baseUsps * (trackRate == RATE_LUNAR ? LUNAR_RATE_FACTOR : 1.0f) * smoothedTrim;
  }

  timerBlocked = blocked;
  jogEndstopMask = mode == MODE_JOG ? 1UL << (towardHome ? PIN_ENDSTOP_HOME : PIN_ENDSTOP_END) : 0;
  homingEndstopMask = mode == MODE_HOMING ? 1UL << (towardHome ? PIN_ENDSTOP_HOME : PIN_ENDSTOP_END) : 0;
  timerHomeBackoff = mode == MODE_HOMING && homePhase == HOME_BACKOFF;
  const bool useTimer = mode != MODE_IDLE;
  if (useTimer && !timerRunning) timerWrite(stepTimer, 0);
  if (useTimer) setTimerSpeed(targetUsps);
  digitalWrite(PIN_EN, mode == MODE_IDLE || blocked ? HIGH : LOW);
  applyDriverProfile();
  timerRunning = useTimer;

  updateOled();
  delay(1);
}

#endif // ARDUINO_ARCH_ESP32
