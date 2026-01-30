#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

#include <TMCStepper.h>
#include <SoftwareSerial.h>

// OLED: 128x64 I2C SSD1306 (U8g2 page buffer mode -> MUCH lower SRAM on UNO)
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// --- TMC2209 (UART configured, still STEP/DIR driven) ---
static constexpr float R_SENSE = 0.11f;      // common value on many SilentStepStick boards (verify on your module)
static constexpr uint8_t TMC_ADDR = 0;       // MS1/MS2 address (0..3), usually 0

// --- Pins ---
// 3-position direction switch (ON-OFF-ON):
// - Switch COMMON -> GND
// - North contact  -> D5 Schalter links
// - South contact  -> D4 Schalter rechts
// Middle position leaves both contacts open => OFF
const int PIN_DIR_N = 5;              // North contact (active LOW)
const int PIN_DIR_S = 4;              // South contact (active LOW)

const int PIN_STEP  = 2;
const int PIN_DIR   = 3;
const int PIN_EN    = 8;
const int PIN_POT   = A0;             // 10k Poti

// --- Endstops / Buttons (NO endstops to GND, use INPUT_PULLUP) ---
const int PIN_HOME_BTN     = 9;   // Home button (momentary to GND)
const int PIN_ENDSTOP_END  = 10;  // End endstop (NO to GND)
const int PIN_ENDSTOP_HOME = 11;  // Home endstop (NO to GND)

// Which motor direction moves *towards* the HOME endstop?
// If homing runs the wrong way, flip this.
static constexpr bool HOME_DIR_FORWARD = false; // false=South, true=North

// --- TMC2209 UART (UNO/Nano uses SoftwareSerial) ---
const int PIN_TMC_RX = 6;             // Arduino RX  <- TMC2209 UART pin
const int PIN_TMC_TX = 7;             // Arduino TX  -> TMC2209 UART pin (recommend 1k in series)

SoftwareSerial TMC_SERIAL(PIN_TMC_RX, PIN_TMC_TX); // RX, TX
TMC2209Stepper driver(&TMC_SERIAL, R_SENSE, TMC_ADDR);

#define DIR_LEFT_IS_FORWARD  true     // true = North, false = South

// --- Mechanik/Geometrie ---
const float STEPS_PER_REV = 200.0f;   // NEMA17
const float MICROSTEPS    = 32.0f;    // TMC2209 set via UART (driver.microsteps())
const float ROLLER_R_MM   = 10.0f;     // Ø20 mm -> r=10 mm. Adjust to your shaft diameter!
const float R_MM          = 572.561f; // Pivot->contact radius (mm). Adjust to your geometry! (distance center south bearing to center of arc)

// Sternzeit
const float SIDEREAL_SEC  = 86164.0f;

// Debug / bench test: multiply tracking speed to make motion visible.
// Set to 1000.0f for a quick functional test, then put back to 1.0f for real tracking.
const float SPEED_MULT = 1.0f;

// --- Runtime values ---
float BASE_USPS = 0.0f; // µSteps/s bei Poti-Mitte

// If the potentiometer is missing/floating, we fall back to a fixed trim (1.00 = 100%).
const float TRIM_FALLBACK = 1.00f;
static bool g_pot_ok = true;   // updated by readTrimFactor()

// Trim (±10%)
const float TRIM_MIN = 0.90f;
const float TRIM_MAX = 1.10f;

// Step-Timing
const unsigned int STEP_PULSE_US = 3;
unsigned long lastStepMicros     = 0;
unsigned long stepIntervalMicros = 1000000UL;

// Small shared buffer to avoid large stack frames in drawing code
static char g_buf2[12];

// Helpers
static inline float clampf(float x, float a, float b) {
  return (x < a) ? a : (x > b) ? b : x;
}

enum MotionMode : uint8_t {
  MODE_IDLE = 0,
  MODE_TRACKING,
  MODE_HOMING
};

static MotionMode g_mode = MODE_IDLE;

enum HomingPhase : uint8_t {
  HOME_PHASE_IDLE = 0,
  HOME_PHASE_FAST_APPROACH,
  HOME_PHASE_BACKOFF,
  HOME_PHASE_SLOW_APPROACH
};

static HomingPhase g_home_phase = HOME_PHASE_IDLE;

// Backoff distance after first hit (in microsteps). Tune for your mechanics.
// 32 microsteps per full step @ MICROSTEPS=32.
static constexpr uint32_t HOME_BACKOFF_USTEPS = 32UL * 80UL; // ~80 full steps

// Homing speeds (uSteps/s). Initialized in setup() from BASE_USPS.
static float HOMING_FAST_USPS = 0.0f;
static float HOMING_SLOW_USPS = 0.0f;
static float HOMING_BACKOFF_USPS = 0.0f;

// Backoff step counter (decremented when we actually step)
static uint32_t g_backoff_remaining = 0;

// Latch for the home button: one press starts homing (doesn't need to be held)
static bool g_home_latched = false;

struct EndstopState {
  bool home; // true = pressed
  bool end;  // true = pressed
};

static inline EndstopState readEndstops() {
  // NO switches to GND with INPUT_PULLUP => pressed == LOW
  EndstopState s;
  s.home = (digitalRead(PIN_ENDSTOP_HOME) == LOW);
  s.end  = (digitalRead(PIN_ENDSTOP_END) == LOW);
  return s;
}

// Simple debounce for the home button (active LOW)
// Returns true while pressed.
static bool readHomeButtonPressed() {
  static uint8_t stable = HIGH;
  static uint8_t last   = HIGH;
  static unsigned long lastChange = 0;

  uint8_t r = (uint8_t)digitalRead(PIN_HOME_BTN);
  if (r != last) {
    last = r;
    lastChange = millis();
  }
  if ((millis() - lastChange) > 25) {
    stable = last;
  }
  return (stable == LOW);
}

// Debounced edge: true only once per press
static bool homeButtonPressedEvent() {
  static bool lastStablePressed = false;
  bool pressed = readHomeButtonPressed();
  bool evt = (pressed && !lastStablePressed);
  lastStablePressed = pressed;
  return evt;
}

void applySpeedToInterval(float usps);
float readTrimFactor() {
  // 8x average of ADC (0..1023) + min/max to detect floating input
  uint32_t acc = 0;
  int mn = 1023;
  int mx = 0;
  for (int i = 0; i < 8; ++i) {
    int v = analogRead(PIN_POT);
    acc += (uint32_t)v;
    if (v < mn) mn = v;
    if (v > mx) mx = v;
  }
  float raw = acc / 8.0f; // 0..1023

  // If the pot is not connected, A0 often floats and readings jump around.
  // Detect this by looking at the spread across samples.
  const int FLOAT_SPREAD = 60; // counts; tune if needed
  if ((mx - mn) > FLOAT_SPREAD) {
    g_pot_ok = false;
    return TRIM_FALLBACK;
  }

  // Also treat extreme rails as "no usable trim" (wiring error / short)
  if (raw < 2.0f || raw > 1021.0f) {
    g_pot_ok = false;
    return TRIM_FALLBACK;
  }

  g_pot_ok = true;

  // Deadband around center to avoid jitter
  const int CENTER_ADC = 512;  // adjust (e.g. 505..520)
  const int DEADBAND   = 8;    // ±8 Counts ≈ ±0,8 %
  if (raw > (CENTER_ADC - DEADBAND) && raw < (CENTER_ADC + DEADBAND)) {
    raw = CENTER_ADC;
  }

  // 0..1023 -> TRIM_MIN..TRIM_MAX
  float t = raw / 1023.0f; // 0..1
  return TRIM_MIN + (TRIM_MAX - TRIM_MIN) * t;
}
// --- OLED helper ---
void updateOled(bool tracking, bool forward, float smoothedTrim, float targetUsps, bool potOk, EndstopState es, MotionMode mode, HomingPhase phase) {
  // Compute rev/h from effective speed
  float revPerHour = (targetUsps * 3600.0f) / (STEPS_PER_REV * MICROSTEPS);

  static unsigned long lastDraw = 0;
  unsigned long now = millis();
  if (now - lastDraw < 500) return; // update every 500 ms max
  lastDraw = now;

  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setCursor(10, 10); u8g2.print(F("EQ Platform"));
    u8g2.setCursor(10, 25); u8g2.print(F("Tracking: ")); u8g2.print(tracking ? F("ON") : F("OFF"));
    u8g2.setCursor(10, 30);
    u8g2.print(F("Mode: "));
    if (mode == MODE_HOMING) {
      u8g2.print(F("HOME "));
      if (phase == HOME_PHASE_FAST_APPROACH)      u8g2.print(F("FAST"));
      else if (phase == HOME_PHASE_BACKOFF)       u8g2.print(F("BACK"));
      else if (phase == HOME_PHASE_SLOW_APPROACH) u8g2.print(F("SLOW"));
      else                                        u8g2.print(F("IDLE"));
    } else if (tracking) {
      u8g2.print(F("TRACK"));
    } else {
      u8g2.print(F("IDLE"));
    }

    u8g2.setCursor(10, 35);
    u8g2.print(F("Direction: "));
    u8g2.print(forward ? F("North") : F("South"));

    // Trim as percentage (rounded)
    int pct = (int)(smoothedTrim * 100.0f + 0.5f);
    u8g2.setCursor(10, 45);
    u8g2.print(F("Trim: "));
    u8g2.print(pct);
    u8g2.print('%');
    if (!potOk) {
      u8g2.print(F(" FIX"));
    }

    u8g2.setCursor(10, 50);
    u8g2.print(F("Stops: H="));
    u8g2.print(es.home ? F("1") : F("0"));
    u8g2.print(F(" E="));
    u8g2.print(es.end ? F("1") : F("0"));

    // rev/h (two decimals)
    u8g2.setCursor(10, 62);
    u8g2.print(F("rev/h: "));
    dtostrf(revPerHour, 5, 2, g_buf2);
    u8g2.print(g_buf2);
  } while (u8g2.nextPage());
}

struct DirState {
  bool tracking;
  bool forward; // true = North, false = South
};

DirState readDirState() {
  pinMode(PIN_DIR_N, INPUT_PULLUP);
  pinMode(PIN_DIR_S, INPUT_PULLUP);

  bool northSelected = (digitalRead(PIN_DIR_N) == LOW);
  bool southSelected = (digitalRead(PIN_DIR_S) == LOW);

  // Middle position: neither contact connected -> OFF
  if (!northSelected && !southSelected) {
    return {false, true};
  }

  // Safety: if both are active, treat as OFF
  if (northSelected && southSelected) {
    return {false, true};
  }

  bool forward = northSelected; // north => forward
  if (!DIR_LEFT_IS_FORWARD) {
    forward = !forward;
  }

  return {true, forward};
}

void setup() {
  Serial.begin(115200);
  delay(100);

  // --- TMC2209 UART init ---
  TMC_SERIAL.begin(115200);

  driver.begin();
  driver.pdn_disable(true);        // enable UART control
  driver.I_scale_analog(false);    // use internal current reference (ignore Vref pot)
  driver.toff(4);                  // enable driver
  driver.blank_time(24);
  driver.rms_current(600);         // mA RMS (tune for your motor)
  driver.microsteps((uint16_t)MICROSTEPS);
  driver.intpol(true);             // interpolate to 256
  driver.pwm_autoscale(true);      // stealthChop helper
  driver.en_spreadCycle(false);    // false=stealthChop (quiet), true=spreadCycle (torque)
  driver.TPOWERDOWN(10);

  BASE_USPS = (STEPS_PER_REV * MICROSTEPS / SIDEREAL_SEC) * (R_MM / ROLLER_R_MM) * SPEED_MULT;

  // Homing tuning
  HOMING_FAST_USPS    = BASE_USPS * 8.0f;
  HOMING_BACKOFF_USPS = BASE_USPS * 4.0f;
  HOMING_SLOW_USPS    = BASE_USPS * 1.5f;

  float uStepsPerHour = BASE_USPS * 3600.0f;
  float revPerHour    = uStepsPerHour / (STEPS_PER_REV * MICROSTEPS);
  float minutesPerRev = 60.0f / revPerHour;

  Serial.println(F("=== EQ Platform Driver ==="));
  Serial.print(F("R (mm): ")); Serial.println(R_MM, 3);
  Serial.print(F("Roller r (mm): ")); Serial.println(ROLLER_R_MM, 3);
  Serial.print(F("Steps/rev: ")); Serial.println(STEPS_PER_REV, 0);
  Serial.print(F("Microsteps: ")); Serial.println(MICROSTEPS, 0);
  Serial.print(F("Speed multiplier: ")); Serial.println(SPEED_MULT, 3);
  Serial.print(F("Driver: ")); Serial.println(F("TMC2209 (UART)"));
  Serial.print(F("RMS current (mA): ")); Serial.println(600);
  Serial.print(F("Base rate (uSteps/s): ")); Serial.println(BASE_USPS, 3);
  Serial.print(F("Revolutions per hour: ")); Serial.println(revPerHour, 3);
  Serial.print(F("Minutes per revolution: ")); Serial.println(minutesPerRev, 2);
  Serial.print(F("Trim range: ")); Serial.print(TRIM_MIN*100,0);
  Serial.print(F("% .. ")); Serial.print(TRIM_MAX*100,0); Serial.println(F("%"));

  pinMode(PIN_STEP, OUTPUT);
  pinMode(PIN_DIR,  OUTPUT);
  pinMode(PIN_EN,   OUTPUT);

  pinMode(PIN_HOME_BTN, INPUT_PULLUP);
  pinMode(PIN_ENDSTOP_HOME, INPUT_PULLUP);
  pinMode(PIN_ENDSTOP_END, INPUT_PULLUP);

  digitalWrite(PIN_EN, HIGH);                // driver disabled at startup

  DirState st0 = readDirState();
  digitalWrite(PIN_DIR, st0.forward ? HIGH : LOW);

  float usps = BASE_USPS * readTrimFactor();
  applySpeedToInterval(usps);
  lastStepMicros = micros();

  analogReference(DEFAULT);

  Serial.print(F("Start usps: ")); Serial.println(usps, 3);
  Serial.print(F("Step interval (ms): "));
  Serial.println(stepIntervalMicros / 1000.0f, 3);

  // I2C init
  Wire.begin();

  // --- OLED init (U8g2) ---
  u8g2.begin();
  u8g2.setI2CAddress(0x3C << 1);
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setCursor(0, 12); u8g2.print(F("EQ Platform"));
    u8g2.setCursor(0, 28); u8g2.print(F("Boot OK"));
  } while (u8g2.nextPage());
}

void loop() {
  unsigned long nowMs = millis();

  // Read inputs
  DirState st = readDirState();
  EndstopState es = readEndstops();
  bool homeBtn = readHomeButtonPressed();
  bool homeEvt = homeButtonPressedEvent();

  // Mode transitions
  // One press of the home button starts a full homing sequence (latched).
  if (homeEvt) {
    g_home_latched = true;
    g_mode = MODE_HOMING;
    g_home_phase = HOME_PHASE_FAST_APPROACH;
    g_backoff_remaining = 0;
    Serial.println(F("Mode: HOMING (latched)"));
  }

  // If we are not currently homing, follow the direction switch state.
  if (g_mode != MODE_HOMING) {
    g_mode = st.tracking ? MODE_TRACKING : MODE_IDLE;
  }

  bool tracking = (g_mode == MODE_TRACKING);
  bool forward  = st.forward;

  // In homing mode we override direction depending on phase.
  if (g_mode == MODE_HOMING) {
    if (g_home_phase == HOME_PHASE_BACKOFF) {
      forward = !HOME_DIR_FORWARD; // move away from home switch
    } else {
      forward = HOME_DIR_FORWARD;  // move toward home switch
    }
  }

  // Stop immediately if we are trying to move into a pressed endstop.
  // - If moving toward HOME and home endstop is pressed: stop.
  // - If moving toward END  and end endstop is pressed: stop.
  bool towardHome = (forward == HOME_DIR_FORWARD);
  bool blocked = (towardHome && es.home) || (!towardHome && es.end);

  // Homing phase state machine (Backoff + slow re-approach)
  if (g_mode == MODE_HOMING) {
    switch (g_home_phase) {
      case HOME_PHASE_FAST_APPROACH:
        // When we hit HOME the first time: start backoff.
        if (es.home && towardHome) {
          g_home_phase = HOME_PHASE_BACKOFF;
          g_backoff_remaining = HOME_BACKOFF_USTEPS;
          Serial.println(F("Homing: first hit -> BACKOFF"));
        }
        break;

      case HOME_PHASE_BACKOFF:
        // When we've backed off enough AND the switch is released, do slow approach.
        if (g_backoff_remaining == 0 && !es.home) {
          g_home_phase = HOME_PHASE_SLOW_APPROACH;
          Serial.println(F("Homing: released -> SLOW APPROACH"));
        }
        break;

      case HOME_PHASE_SLOW_APPROACH:
        // Second hit at low speed => final HOME position.
        if (es.home && towardHome) {
          g_home_phase = HOME_PHASE_IDLE;
          g_home_latched = false;
          g_mode = MODE_IDLE;
          digitalWrite(PIN_EN, HIGH);
          Serial.println(F("Homing done: precise HOME reached"));
        }
        break;

      default:
        break;
    }
  }

  // Driver enable
  // Active LOW: LOW = enabled
  if (g_mode == MODE_IDLE || blocked) {
    digitalWrite(PIN_EN, HIGH);
  } else {
    digitalWrite(PIN_EN, LOW);
  }

  // Direction pin
  digitalWrite(PIN_DIR, forward ? HIGH : LOW);

  // Logging (rate-limited)
  static MotionMode lastMode = MODE_IDLE;
  static unsigned long lastModeChangeMs = 0;
  if (g_mode != lastMode && (nowMs - lastModeChangeMs) >= 200) {
    if (g_mode == MODE_TRACKING) Serial.println(F("Mode: TRACKING"));
    else if (g_mode == MODE_IDLE) Serial.println(F("Mode: IDLE"));
    lastMode = g_mode;
    lastModeChangeMs = nowMs;
  }

  static bool lastForward = false;
  static unsigned long lastDirChangeMs = 0;
  static unsigned long dirChangedAt = 0;
  if ((g_mode != MODE_IDLE) && forward != lastForward && (nowMs - lastDirChangeMs) >= 200) {
    Serial.print(F("Direction: "));
    Serial.println(forward ? F("North") : F("South"));
    lastForward = forward;
    lastDirChangeMs = nowMs;
    dirChangedAt = micros();
  }

  float targetUsps = 0.0f;

  // Read potentiometer -> trim factor and resulting speed (tracking mode)
  float trimFactor = readTrimFactor();

  // Exponential smoothing (EMA)
  static float smoothedTrim = -1.0f;
  if (smoothedTrim < 0.0f) smoothedTrim = trimFactor;  // Initialize on first run
  const float ALPHA = 0.2f;                             // 0..1, lower = smoother
  smoothedTrim = smoothedTrim + ALPHA * (trimFactor - smoothedTrim);

  if (g_mode == MODE_HOMING) {
    if (g_home_phase == HOME_PHASE_BACKOFF)      targetUsps = HOMING_BACKOFF_USPS;
    else if (g_home_phase == HOME_PHASE_SLOW_APPROACH) targetUsps = HOMING_SLOW_USPS;
    else                                          targetUsps = HOMING_FAST_USPS;
  } else {
    targetUsps = BASE_USPS * smoothedTrim;
  }

  applySpeedToInterval(targetUsps);

  // Update OLED with endstop/mode info
  updateOled(tracking, forward, smoothedTrim, targetUsps, g_pot_ok, es, g_mode, g_home_phase);

  if (g_mode == MODE_IDLE) return;
  // If we're blocked by an endstop in the current direction, don't step.
  if (blocked) return;

  // ensure at least 500 µs after a DIR change before a step occurs
  if ((long)(micros() - dirChangedAt) < 500) {
    return;
  }

  unsigned long now = micros();
  if ((long)(now - lastStepMicros) >= (long)stepIntervalMicros) {
    digitalWrite(PIN_STEP, HIGH);
    delayMicroseconds(STEP_PULSE_US);
    digitalWrite(PIN_STEP, LOW);

    // Count backoff distance only when we actually step
    if (g_mode == MODE_HOMING && g_home_phase == HOME_PHASE_BACKOFF && g_backoff_remaining > 0) {
      g_backoff_remaining--;
    }

    lastStepMicros += stepIntervalMicros;
    if ((long)(now - lastStepMicros) >= (long)stepIntervalMicros) {
      lastStepMicros = now;
    }
  }
}

void applySpeedToInterval(float usps) {
  if (usps < 0.0001f) usps = 0.0001f;
  stepIntervalMicros = (unsigned long)(1000000.0f / usps);
}