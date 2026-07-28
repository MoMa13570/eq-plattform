#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

// OLED: 128x64 I2C SSD1306 (U8g2 full buffer mode)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// --- Pins ---
const int PIN_STEP  = 2;
const int PIN_DIR   = 3;
const int PIN_EN    = 8;
const int PIN_TRK   = 4;              // Tracking switch: COM->D4, left->GND, right->+5V
const int PIN_DIRSW = 5;              // Direction switch: COM->D5, left->GND, right->+5V
const int PIN_POT   = A0;             // 10k Poti

#define DIR_LEFT_IS_FORWARD  true     // true = North, false = South

// --- Mechanik/Geometrie ---
const float STEPS_PER_REV = 200.0f;   // NEMA17
const float MICROSTEPS    = 16.0f;    // A4988: MS1/MS2/MS3 = HIGH
const float ROLLER_R_MM   = 9.0f;     // Ø18 mm -> r=9 mm. Adjust to your shaft diameter!
const float R_MM          = 572.561f; // Pivot->contact radius (mm). Adjust to your geometry! (distance center south bearing to center of arc)
constexpr float MOTOR_PULLEY_TEETH = 16.0f;
constexpr float SHAFT_PULLEY_TEETH = 66.0f;
constexpr float GEAR_RATIO = SHAFT_PULLEY_TEETH / MOTOR_PULLEY_TEETH;

// Sternzeit
const float SIDEREAL_SEC  = 86164.0f;

// --- Runtime values ---
float BASE_USPS = 0.0f; // µSteps/s bei Poti-Mitte

// Trim
const float TRIM_MIN = 0.80f;
const float TRIM_MAX = 1.20f;

// Step-Timing
const unsigned int STEP_PULSE_US = 3;
unsigned long lastStepMicros     = 0;
unsigned long stepIntervalMicros = 1000000UL;

// Helpers
static inline float clampf(float x, float a, float b) {
  return (x < a) ? a : (x > b) ? b : x;
}
static float shaftRevPerHourFromMotorUsps(float motorUstepsPerSecond) {
  return (motorUstepsPerSecond * 3600.0f)
       / (STEPS_PER_REV * MICROSTEPS * GEAR_RATIO);
}
float readTrimFactor() {
  // 8x average of ADC (0..1023)
  uint32_t acc = 0;
  for (int i = 0; i < 8; ++i) {
    acc += analogRead(PIN_POT);
  }
  float raw = acc / 8.0f; // 0..1023

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
void applySpeedToInterval(float usps) {
  if (usps < 0.0001f) usps = 0.0001f;
  stepIntervalMicros = (unsigned long)(1000000.0f / usps);
}

// --- OLED helper ---
void updateOled(bool tracking, bool forward, float smoothedTrim, float targetUsps) {
  // User-facing speed is always the drive-shaft speed.
  float revPerHour = shaftRevPerHourFromMotorUsps(targetUsps);

  static unsigned long lastDraw = 0;
  unsigned long now = millis();
  if (now - lastDraw < 500) return; // update every 500 ms max
  lastDraw = now;

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setCursor(10, 10); u8g2.print(F("EQ Platform"));
  u8g2.setCursor(10, 25); u8g2.print(F("Tracking: ")); u8g2.print(tracking ? F("ON") : F("OFF"));
  u8g2.setCursor(10, 35);
  u8g2.print(F("Direction: "));
  u8g2.print(forward ? F("North") : F("South"));

  // Trim as percentage (rounded)
  int pct = (int)(smoothedTrim * 100.0f + 0.5f);
  u8g2.setCursor(10, 45); u8g2.print(F("Trim: ")); u8g2.print(pct); u8g2.print('%');

  // rev/h (two decimals)
  u8g2.setCursor(10, 55); u8g2.print(F("rev/h: "));
  char buf2[12];
  dtostrf(revPerHour, 5, 2, buf2);
  u8g2.print(buf2);

  u8g2.sendBuffer();
}

bool isTrackingOn() {
  pinMode(PIN_TRK, INPUT_PULLUP);              // SPDT delivers LOW/HIGH, use pull-up to avoid floating
  return (digitalRead(PIN_TRK) == HIGH);        // left/GND=ON
}
bool isForward() {
  pinMode(PIN_DIRSW, INPUT_PULLUP);
  bool leftSelected = (digitalRead(PIN_DIRSW) == HIGH);
  return DIR_LEFT_IS_FORWARD ? leftSelected : !leftSelected;
}

void i2cScan() {
  Serial.println(F("I2C scan:"));
  byte count = 0;
  for (byte addr = 1; addr < 127; ++addr) {
    Wire.beginTransmission(addr);
    byte error = Wire.endTransmission();
    if (error == 0) {
      Serial.print(F("  Found 0x"));
      if (addr < 16) Serial.print('0');
      Serial.print(addr, HEX);
      Serial.println();
      count++;
    }
  }
  if (count == 0) Serial.println(F("  No I2C devices found"));
}

void setup() {
  Serial.begin(115200);
  delay(100);

  BASE_USPS = (STEPS_PER_REV * MICROSTEPS / SIDEREAL_SEC)
            * (R_MM / ROLLER_R_MM)
            * GEAR_RATIO;

  float uStepsPerHour = BASE_USPS * 3600.0f;
  float motorRevPerHour = uStepsPerHour / (STEPS_PER_REV * MICROSTEPS);
  float shaftRevPerHour = motorRevPerHour / GEAR_RATIO;
  float minutesPerRev = 60.0f / shaftRevPerHour;

  Serial.println(F("=== EQ Platform Driver ==="));
  Serial.print(F("R (mm): ")); Serial.println(R_MM, 3);
  Serial.print(F("Roller r (mm): ")); Serial.println(ROLLER_R_MM, 3);
  Serial.print(F("Steps/rev: ")); Serial.println(STEPS_PER_REV, 0);
  Serial.print(F("Microsteps: ")); Serial.println(MICROSTEPS, 0);
  Serial.print(F("Motor pulley teeth: ")); Serial.println(MOTOR_PULLEY_TEETH, 0);
  Serial.print(F("Shaft pulley teeth: ")); Serial.println(SHAFT_PULLEY_TEETH, 0);
  Serial.print(F("Gear ratio: ")); Serial.println(GEAR_RATIO, 3);
  Serial.print(F("Motor base rate (uSteps/s): ")); Serial.println(BASE_USPS, 3);
  Serial.print(F("Motor revolutions per hour: ")); Serial.println(motorRevPerHour, 3);
  Serial.print(F("Shaft revolutions per hour: ")); Serial.println(shaftRevPerHour, 3);
  Serial.print(F("Shaft minutes per revolution: ")); Serial.println(minutesPerRev, 2);
  Serial.print(F("Trim range: ")); Serial.print(TRIM_MIN*100,0);
  Serial.print(F("% .. ")); Serial.print(TRIM_MAX*100,0); Serial.println(F("%"));

  pinMode(PIN_STEP, OUTPUT);
  pinMode(PIN_DIR,  OUTPUT);
  pinMode(PIN_EN,   OUTPUT);

  digitalWrite(PIN_EN, HIGH);                // driver disabled at startup
  digitalWrite(PIN_DIR, isForward() ? HIGH : LOW);

  float usps = BASE_USPS * readTrimFactor();
  applySpeedToInterval(usps);
  lastStepMicros = micros();

  analogReference(DEFAULT);

  Serial.print(F("Start usps: ")); Serial.println(usps, 3);
  Serial.print(F("Step interval (ms): "));
  Serial.println(stepIntervalMicros / 1000.0f, 3);

  // I2C init & scan (helpful if display stays blank)
  Wire.begin();
  Wire.setClock(400000); // fast I2C
  i2cScan();

  // --- OLED init (U8g2) ---
  u8g2.begin();
  u8g2.enableUTF8Print();
  u8g2.setI2CAddress(0x3C << 1);
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setCursor(0, 10); u8g2.print(F("EQ Platform"));
  u8g2.setCursor(0, 25); u8g2.print(F("Init..."));
  u8g2.setFont(u8g2_font_logisoso16_tr);
  u8g2.setCursor(0, 50); u8g2.print(F("OLED OK"));
  u8g2.sendBuffer();
}

void loop() {
  bool tracking = isTrackingOn();
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
  digitalWrite(PIN_EN, tracking ? LOW : HIGH); // LOW/GND=Driver active

  bool forward = isForward();
  digitalWrite(PIN_DIR, forward ? HIGH : LOW);
  static bool lastForward = false;
  static unsigned long lastDirChangeMs = 0;
  static unsigned long dirChangedAt = 0;
  if (forward != lastForward && (nowMs - lastDirChangeMs) >= 2000) {
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

  updateOled(tracking, forward, smoothedTrim, targetUsps);

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
