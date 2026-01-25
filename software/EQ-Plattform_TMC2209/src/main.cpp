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
// - North contact  -> D5
// - South contact  -> D4
// Middle position leaves both contacts open => OFF
const int PIN_DIR_N = 5;              // North contact (active LOW)
const int PIN_DIR_S = 4;              // South contact (active LOW)

const int PIN_STEP  = 2;
const int PIN_DIR   = 3;
const int PIN_EN    = 8;
const int PIN_POT   = A0;             // 10k Poti

// --- TMC2209 UART (UNO/Nano uses SoftwareSerial) ---
const int PIN_TMC_RX = 6;             // Arduino RX  <- TMC2209 UART pin
const int PIN_TMC_TX = 7;             // Arduino TX  -> TMC2209 UART pin (recommend 1k in series)

SoftwareSerial TMC_SERIAL(PIN_TMC_RX, PIN_TMC_TX); // RX, TX
TMC2209Stepper driver(&TMC_SERIAL, R_SENSE, TMC_ADDR);

#define DIR_LEFT_IS_FORWARD  true     // true = North, false = South

// --- Mechanik/Geometrie ---
const float STEPS_PER_REV = 200.0f;   // NEMA17
const float MICROSTEPS    = 16.0f;    // TMC2209 set via UART (driver.microsteps())
const float ROLLER_R_MM   = 10.0f;     // Ø18 mm -> r=9 mm. Adjust to your shaft diameter!
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

// Trim
const float TRIM_MIN = 0.80f;
const float TRIM_MAX = 1.20f;

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
void updateOled(bool tracking, bool forward, float smoothedTrim, float targetUsps, bool potOk) {
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

    // rev/h (two decimals)
    u8g2.setCursor(10, 55);
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
  DirState st = readDirState();
  bool tracking = st.tracking;
  unsigned long nowMs = millis();
  static bool lastTracking = false;
  static unsigned long lastTrackingChangeMs = 0;
  if (tracking != lastTracking && (nowMs - lastTrackingChangeMs) >= 2000) {
    if (tracking) {
      Serial.println(F("Tracking: ON"));
    } else {
      Serial.println(F("Tracking: OFF"));
    }
    lastTracking = tracking;
    lastTrackingChangeMs = nowMs;
  }
  // TMC2209 EN pin is typically active-low (same as A4988)
  digitalWrite(PIN_EN, tracking ? LOW : HIGH); // LOW = Driver active

  bool forward = st.forward;
  digitalWrite(PIN_DIR, forward ? HIGH : LOW);
  static bool lastForward = false;
  static unsigned long lastDirChangeMs = 0;
  static unsigned long dirChangedAt = 0;
  if (tracking && forward != lastForward && (nowMs - lastDirChangeMs) >= 2000) {
    if (forward) {
      Serial.println(F("Direction: North"));
    } else {
      Serial.println(F("Direction: South"));
    }
    lastForward = forward;
    lastDirChangeMs = nowMs;
    dirChangedAt = micros();
  }

  // Read potentiometer -> trim factor and resulting speed
  float trimFactor = readTrimFactor();

  // Exponential smoothing (EMA)
  static float smoothedTrim = -1.0f;
  if (smoothedTrim < 0.0f) smoothedTrim = trimFactor;  // Initialize on first run
  const float ALPHA = 0.2f;                             // 0..1, lower = smoother
  smoothedTrim = smoothedTrim + ALPHA * (trimFactor - smoothedTrim);

  float targetUsps = BASE_USPS * smoothedTrim;
  applySpeedToInterval(targetUsps);

  updateOled(tracking, forward, smoothedTrim, targetUsps, g_pot_ok);

  // Debug output: trim factor and effective speed every 2 seconds
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 2000) {
    Serial.print(F("Trim=")); Serial.print(smoothedTrim, 3);
    Serial.print(F("  uSteps/s=")); Serial.println(targetUsps, 3);
    lastPrint = millis();
  }

  if (!tracking) return;

  // ensure at least 500 µs after a DIR change before a step occurs
  if ((long)(micros() - dirChangedAt) < 500) {
    return;
  }

  unsigned long now = micros();
  if ((long)(now - lastStepMicros) >= (long)stepIntervalMicros) {
    digitalWrite(PIN_STEP, HIGH);
    delayMicroseconds(STEP_PULSE_US);
    digitalWrite(PIN_STEP, LOW);

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