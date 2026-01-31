#include <Arduino.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

#include <Wire.h>
#include <U8g2lib.h>

#include <TMCStepper.h>
#include <SoftwareSerial.h>

// ================================
//  Hardware / Pins
// ================================

// 3-position direction switch (ON-OFF-ON):
// - COMMON -> GND
// - North contact -> D5
// - South contact -> D4
// Middle position leaves both contacts open.
static constexpr uint8_t PIN_DIR_N = 5;   // active LOW
static constexpr uint8_t PIN_DIR_S = 4;   // active LOW

static constexpr uint8_t PIN_STEP = 2;    // UNO D2 (PD2)
static constexpr uint8_t PIN_DIR  = 3;
static constexpr uint8_t PIN_EN   = 8;    // active LOW (TMC enable)
static constexpr uint8_t PIN_POT  = A0;

// Button + Endstops (NO switches wired to GND with INPUT_PULLUP)
static constexpr uint8_t PIN_HOME_BTN     = 9;
static constexpr uint8_t PIN_ENDSTOP_END  = 10;
static constexpr uint8_t PIN_ENDSTOP_HOME = 11;

// Endstop polarity:
//  - NO to GND + INPUT_PULLUP => pressed == LOW
static constexpr bool ENDSTOP_ACTIVE_LOW = true;

// ================================
//  TMC2209 (UART config, STEP/DIR motion)
// ================================
static constexpr float   R_SENSE  = 0.11f;
static constexpr uint8_t TMC_ADDR = 0;

static constexpr uint8_t PIN_TMC_RX = 6;
static constexpr uint8_t PIN_TMC_TX = 7;

SoftwareSerial  TMC_SERIAL(PIN_TMC_RX, PIN_TMC_TX);
TMC2209Stepper  driver(&TMC_SERIAL, R_SENSE, TMC_ADDR);

// If your switch orientation is reversed, flip this.
static constexpr bool DIR_LEFT_IS_FORWARD = true; // true=North, false=South

// ================================
//  Mechanics / Tracking
// ================================
static constexpr float STEPS_PER_REV = 200.0f;
static constexpr float MICROSTEPS    = 32.0f;
static constexpr float ROLLER_R_MM   = 10.0f;     // Ø20mm roller
static constexpr float R_MM          = 572.561f;  // platform geometry
static constexpr float SIDEREAL_SEC  = 86164.0f;

// Multiplier for bench tests (1.0 for real tracking)
static constexpr float SPEED_MULT    = 1.0f;

// Pot trim
static constexpr float TRIM_MIN      = 0.90f;
static constexpr float TRIM_MAX      = 1.10f;
static constexpr float TRIM_FALLBACK = 1.00f;
static bool g_pot_ok = true;

// ================================
//  OLED
// ================================
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
static char g_buf[12];

// ================================
//  Motion state
// ================================
enum MotionMode : uint8_t { MODE_IDLE = 0, MODE_TRACKING, MODE_HOMING };
enum HomingPhase : uint8_t { HOME_PHASE_IDLE = 0, HOME_PHASE_FAST_APPROACH, HOME_PHASE_BACKOFF, HOME_PHASE_SLOW_APPROACH };

static MotionMode  g_mode       = MODE_IDLE;
static HomingPhase g_home_phase = HOME_PHASE_IDLE;

// Last selected direction from the switch
static bool g_track_forward   = true;
static bool g_track_dir_valid = false;

// Fallback home direction (only used before a valid track direction was ever read)
static constexpr bool HOME_DIR_FORWARD_FALLBACK = false; // false=South, true=North

// ================================
//  Homing config (tuning)
// ================================
// Direction selection:
//  - If you tracked North => home goes South (and vice versa)
static constexpr bool HOME_DIR_IS_OPPOSITE_OF_TRACK = true;

// Speeds are derived from BASE_USPS (sidereal) using multipliers.
static constexpr float HOME_FAST_MULT    = 150.0f;
static constexpr float HOME_BACKOFF_MULT = 60.0f;
static constexpr float HOME_SLOW_MULT    = 12.0f;

// Backoff limits
static constexpr uint32_t HOME_BACKOFF_USTEPS = 1600UL;        // 0.25 motor rev @ 200*32=6400 uSteps/rev
static constexpr unsigned long HOME_BACKOFF_MAX_MS = 2000UL;

// Grace period after starting homing during which END pressed is tolerated
static constexpr unsigned long HOMING_END_GRACE_MS = 300UL;

// ================================
//  Homing runtime state
// ================================
static bool g_home_dir_forward    = HOME_DIR_FORWARD_FALLBACK;
static bool g_homing_dir_flipped  = false;
static unsigned long g_homing_started_ms = 0;

static uint32_t g_backoff_remaining = 0;
static unsigned long g_backoff_started_ms = 0;

// Homing speeds (uSteps/s), computed in setup()
static float BASE_USPS = 800.0f;
static float HOMING_FAST_USPS    = 0.0f;
static float HOMING_BACKOFF_USPS = 0.0f;
static float HOMING_SLOW_USPS    = 0.0f;

// ================================
//  Timer1 step generator (TRACKING only)
// ================================
static constexpr uint16_t STEP_PULSE_US = 5;
static constexpr uint32_t T1_HZ = 2000000UL; // 2 MHz (prescaler 8)

// Shared ISR state
volatile uint32_t g_interval_ticks = 2000000UL;
volatile bool     g_step_run       = false;
volatile bool     g_step_blocked   = false;
volatile bool     g_step_high      = false;
volatile uint32_t g_pause_ticks    = 0; // 0.5us ticks
volatile uint32_t g_wait_ticks     = 0; // 0.5us ticks

static inline uint32_t usToTicks(uint32_t us) { return us * 2UL; }

// Fast STEP pin (UNO D2 = PD2)
#define STEP_PORT PORTD
#define STEP_BIT  2
static inline void stepHighFast() { STEP_PORT |=  (1 << STEP_BIT); }
static inline void stepLowFast()  { STEP_PORT &= ~(1 << STEP_BIT); }

ISR(TIMER1_COMPA_vect) {
  // 1) Pause (DIR-change guard)
  if (g_pause_ticks > 0) {
    stepLowFast();
    g_step_high = false;
    uint32_t chunk = g_pause_ticks;
    if (chunk > 65535UL) chunk = 65535UL;
    g_pause_ticks -= chunk;
    OCR1A = (uint16_t)chunk;
    return;
  }

  // 2) Not running / blocked
  if (!g_step_run || g_step_blocked) {
    stepLowFast();
    g_step_high = false;
    g_wait_ticks = 0;
    OCR1A = 4000; // 2ms heartbeat
    return;
  }

  // 3) Waiting between steps (STEP low)
  if (!g_step_high && g_wait_ticks > 0) {
    uint32_t chunk = g_wait_ticks;
    if (chunk > 65535UL) chunk = 65535UL;
    g_wait_ticks -= chunk;
    OCR1A = (uint16_t)chunk;
    return;
  }

  const uint16_t pulseTicks = (uint16_t)usToTicks(STEP_PULSE_US);
  uint32_t interval = g_interval_ticks;
  if (interval < (uint32_t)pulseTicks * 2UL) interval = (uint32_t)pulseTicks * 2UL;

  if (!g_step_high) {
    stepHighFast();
    g_step_high = true;
    OCR1A = pulseTicks;
  } else {
    stepLowFast();
    g_step_high = false;

    // Remaining low time
    g_wait_ticks = interval - (uint32_t)pulseTicks;
    if (g_wait_ticks == 0) g_wait_ticks = 1;

    uint32_t first = g_wait_ticks;
    if (first > 65535UL) first = 65535UL;
    g_wait_ticks -= first;
    OCR1A = (uint16_t)first;
  }
}

static void initTimer1() {
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  TCCR1B |= (1 << WGM12);   // CTC
  TCCR1B |= (1 << CS11);    // prescaler 8 => 2MHz
  OCR1A   = 40000;          // 20ms initial
  TIMSK1 |= (1 << OCIE1A);
  sei();
}

// ================================
//  Helpers
// ================================
struct EndstopState {
  bool home;
  bool end;
  uint8_t homeRaw;
  uint8_t endRaw;
};

static inline EndstopState readEndstops() {
  EndstopState s;
  s.homeRaw = (uint8_t)digitalRead(PIN_ENDSTOP_HOME);
  s.endRaw  = (uint8_t)digitalRead(PIN_ENDSTOP_END);

  if (ENDSTOP_ACTIVE_LOW) {
    s.home = (s.homeRaw == LOW);
    s.end  = (s.endRaw  == LOW);
  } else {
    s.home = (s.homeRaw == HIGH);
    s.end  = (s.endRaw  == HIGH);
  }
  return s;
}

// Debounced button state (active LOW)
static bool readHomeButtonPressed() {
  static uint8_t stable = HIGH;
  static uint8_t last   = HIGH;
  static unsigned long lastChange = 0;

  uint8_t r = (uint8_t)digitalRead(PIN_HOME_BTN);
  if (r != last) {
    last = r;
    lastChange = millis();
  }
  if ((millis() - lastChange) > 25) stable = last;
  return (stable == LOW);
}

// short press = toggle tracking, long press = start homing
static void readButtonEvents(bool &shortPress, bool &longPress) {
  shortPress = false;
  longPress  = false;

  static bool lastPressed = false;
  static unsigned long pressedAtMs = 0;
  static bool longFired = false;

  bool pressed = readHomeButtonPressed();

  if (pressed && !lastPressed) {
    pressedAtMs = millis();
    longFired = false;
  }

  const unsigned long LONG_MS = 800;
  if (pressed && !longFired && (millis() - pressedAtMs) >= LONG_MS) {
    longPress = true;
    longFired = true;
  }

  if (!pressed && lastPressed && !longFired) {
    shortPress = true;
  }

  lastPressed = pressed;
}

struct DirState { bool selected; bool forward; };

static DirState readDirState() {
  pinMode(PIN_DIR_N, INPUT_PULLUP);
  pinMode(PIN_DIR_S, INPUT_PULLUP);

  bool north = (digitalRead(PIN_DIR_N) == LOW);
  bool south = (digitalRead(PIN_DIR_S) == LOW);

  if (!north && !south) return {false, true};
  if ( north &&  south) return {false, true};

  bool forward = north; // north => forward
  if (!DIR_LEFT_IS_FORWARD) forward = !forward;
  return {true, forward};
}

static float readTrimFactor() {
  // average + spread detect floating
  uint32_t acc = 0;
  int mn = 1023, mx = 0;
  for (int i = 0; i < 8; ++i) {
    int v = analogRead(PIN_POT);
    acc += (uint32_t)v;
    if (v < mn) mn = v;
    if (v > mx) mx = v;
  }

  float raw = acc / 8.0f;

  const int FLOAT_SPREAD = 60;
  if ((mx - mn) > FLOAT_SPREAD || raw < 2.0f || raw > 1021.0f) {
    g_pot_ok = false;
    return TRIM_FALLBACK;
  }
  g_pot_ok = true;

  // deadband
  const int CENTER_ADC = 512;
  const int DEADBAND   = 8;
  if (raw > (CENTER_ADC - DEADBAND) && raw < (CENTER_ADC + DEADBAND)) raw = CENTER_ADC;

  float t = raw / 1023.0f;
  return TRIM_MIN + (TRIM_MAX - TRIM_MIN) * t;
}

static void applySpeedToTimer(float ustepsPerSecond) {
  if (ustepsPerSecond < 0.0001f) ustepsPerSecond = 0.0001f;
  uint32_t ticks = (uint32_t)((float)T1_HZ / ustepsPerSecond);
  if (ticks < 20) ticks = 20;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { g_interval_ticks = ticks; }
}

// ================================
//  OLED
// ================================
static void updateOled(bool forward, float smoothedTrim, float ustepsPerSecond, EndstopState es) {
  float revPerHour = (ustepsPerSecond * 3600.0f) / (STEPS_PER_REV * MICROSTEPS);

  static unsigned long lastDraw = 0;
  unsigned long now = millis();
  unsigned long minPeriod = (g_mode == MODE_IDLE) ? 500UL : 2000UL;
  if (now - lastDraw < minPeriod) return;
  lastDraw = now;

  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_5x8_tf);

    u8g2.setCursor(5, 10);
    u8g2.print(F("EQ Platform"));

    u8g2.setCursor(5, 20);
    u8g2.print(F("Mode:"));
    if (g_mode == MODE_HOMING) {
      u8g2.print(F("HOME-"));
      if      (g_home_phase == HOME_PHASE_FAST_APPROACH) u8g2.print(F("FAST"));
      else if (g_home_phase == HOME_PHASE_BACKOFF)      u8g2.print(F("BACK"));
      else if (g_home_phase == HOME_PHASE_SLOW_APPROACH)u8g2.print(F("SLOW"));
      else                                              u8g2.print(F("IDLE"));
    } else if (g_mode == MODE_TRACKING) {
      u8g2.print(F("TRACK"));
    } else {
      u8g2.print(F("IDLE"));
    }

    u8g2.setCursor(5, 30);
    u8g2.print(F("Dir:"));
    u8g2.print(forward ? F("N") : F("S"));

    int pct = (int)(smoothedTrim * 100.0f + 0.5f);
    u8g2.setCursor(5, 40);
    u8g2.print(F("Trim:"));
    u8g2.print(pct);
    u8g2.print('%');
    if (!g_pot_ok) u8g2.print(F(" FIX"));

    u8g2.setCursor(5, 50);
    u8g2.print(F("Stops H:"));
    u8g2.print(es.home ? F("1") : F("0"));
    u8g2.print(F(" E:"));
    u8g2.print(es.end ? F("1") : F("0"));
    u8g2.print(F(" "));
    u8g2.print(es.homeRaw == LOW ? F("L") : F("H"));
    u8g2.print(es.endRaw  == LOW ? F("L") : F("H"));

    u8g2.setCursor(5, 60);
    u8g2.print(F("rev/h:"));
    dtostrf(revPerHour, 5, 2, g_buf);
    u8g2.print(g_buf);
  } while (u8g2.nextPage());
}

// ================================
//  HOMING (blocking slice stepping)
// ================================
static void stepMotorSlice(uint32_t ustepsPerSecond, uint32_t durationMs, bool towardHome) {
  if (ustepsPerSecond < 1 || durationMs == 0) return;

  uint32_t stepsThisSlice = (ustepsPerSecond * durationMs) / 1000UL;
  if (stepsThisSlice < 1) stepsThisSlice = 1;

  uint32_t intervalUs = 1000000UL / ustepsPerSecond;
  if (intervalUs <= STEP_PULSE_US + 1) intervalUs = STEP_PULSE_US + 2;
  uint32_t restUs = intervalUs - STEP_PULSE_US;

  const uint8_t POLL_N = 32;

  for (uint32_t i = 0; i < stepsThisSlice; ++i) {
    if ((i % POLL_N) == 0) {
      EndstopState es = readEndstops();
      bool blockedNow = (towardHome && es.home) || (!towardHome && es.end);
      if (blockedNow) break;
    }

    stepHighFast();
    delayMicroseconds(STEP_PULSE_US);
    stepLowFast();

    if (g_home_phase == HOME_PHASE_BACKOFF && g_backoff_remaining > 0) {
      g_backoff_remaining--;
    }

    if (restUs) delayMicroseconds(restUs);
  }
}

static void startHoming(unsigned long nowMs) {
  g_mode = MODE_HOMING;
  g_home_phase = HOME_PHASE_FAST_APPROACH;
  g_backoff_remaining = 0;
  g_homing_started_ms = nowMs;

  // Auto-select direction from last tracking direction
  if (g_track_dir_valid) {
    g_home_dir_forward = HOME_DIR_IS_OPPOSITE_OF_TRACK ? !g_track_forward : g_track_forward;
  } else {
    g_home_dir_forward = HOME_DIR_FORWARD_FALLBACK;
  }
  g_homing_dir_flipped = false;

  Serial.println(F("Mode: HOMING"));
}

// ================================
//  setup / loop
// ================================
void setup() {
  Serial.begin(115200);
  delay(100);

  // UART
  TMC_SERIAL.begin(115200);

  driver.begin();
  driver.pdn_disable(true);
  driver.I_scale_analog(false);
  driver.toff(4);
  driver.blank_time(24);
  driver.rms_current(600);
  driver.microsteps((uint16_t)MICROSTEPS);
  driver.intpol(true);
  driver.pwm_autoscale(true);
  driver.en_spreadCycle(false); // stealthChop
  driver.TPOWERDOWN(10);

  // Rates
  BASE_USPS = (STEPS_PER_REV * MICROSTEPS / SIDEREAL_SEC) * (R_MM / ROLLER_R_MM) * SPEED_MULT;
  HOMING_FAST_USPS    = BASE_USPS * HOME_FAST_MULT;
  HOMING_BACKOFF_USPS = BASE_USPS * HOME_BACKOFF_MULT;
  HOMING_SLOW_USPS    = BASE_USPS * HOME_SLOW_MULT;

  // Pins
  pinMode(PIN_STEP, OUTPUT);
  pinMode(PIN_DIR,  OUTPUT);
  pinMode(PIN_EN,   OUTPUT);

  pinMode(PIN_HOME_BTN, INPUT_PULLUP);
  pinMode(PIN_ENDSTOP_HOME, INPUT_PULLUP);
  pinMode(PIN_ENDSTOP_END,  INPUT_PULLUP);

  digitalWrite(PIN_EN, HIGH); // disabled
  stepLowFast();

  // Seed direction
  DirState st0 = readDirState();
  if (st0.selected) {
    g_track_forward = st0.forward;
    g_track_dir_valid = true;
  }
  digitalWrite(PIN_DIR, g_track_forward ? HIGH : LOW);

  // Start timer for tracking
  applySpeedToTimer(BASE_USPS * readTrimFactor());
  initTimer1();

  // I2C + OLED
  Wire.begin();
  u8g2.begin();
  u8g2.setI2CAddress(0x3C << 1);
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setCursor(5, 12); u8g2.print(F("EQ Platform"));
    u8g2.setCursor(5, 28); u8g2.print(F("Boot OK"));
  } while (u8g2.nextPage());

  Serial.println(F("=== EQ Platform ==="));
}

void loop() {
  const unsigned long nowMs = millis();

  // Inputs
  DirState st = readDirState();
  EndstopState es = readEndstops();

  bool btnShort = false, btnLong = false;
  readButtonEvents(btnShort, btnLong);

  // Remember last valid direction
  if (st.selected) {
    g_track_forward = st.forward;
    g_track_dir_valid = true;
  }

  // END stop during tracking => stop tracking
  static bool lastEndPressed = false;
  if (g_mode == MODE_TRACKING && es.end && !lastEndPressed) {
    Serial.println(F("END endstop hit -> stop tracking"));
    g_mode = MODE_IDLE;
  }
  lastEndPressed = es.end;

  // Button logic
  if (btnLong) {
    startHoming(nowMs);
  } else if (btnShort && g_mode != MODE_HOMING) {
    g_mode = (g_mode == MODE_TRACKING) ? MODE_IDLE : MODE_TRACKING;
    Serial.println(g_mode == MODE_TRACKING ? F("Tracking: ON") : F("Tracking: OFF"));
  }

  // Direction output
  bool forward = g_track_forward;
  if (g_mode == MODE_HOMING) {
    forward = (g_home_phase == HOME_PHASE_BACKOFF) ? !g_home_dir_forward : g_home_dir_forward;
  }
  digitalWrite(PIN_DIR, forward ? HIGH : LOW);

  // Determine if motion is toward HOME (for endstop logic)
  bool towardHome = false;
  if (g_mode == MODE_HOMING) {
    towardHome = (g_home_phase != HOME_PHASE_BACKOFF);
  } else {
    // for tracking, "towardHome" means "moving in the configured home direction"
    towardHome = (forward == g_home_dir_forward);
  }

  // Block if pushing into an endstop
  bool blocked = (towardHome && es.home) || (!towardHome && es.end);

  // Extra homing safety: END should not be hit during FAST/SLOW approach.
  bool homingApproach = (g_mode == MODE_HOMING) && (g_home_phase != HOME_PHASE_BACKOFF);
  bool endGraceActive = (g_mode == MODE_HOMING) && ((nowMs - g_homing_started_ms) < HOMING_END_GRACE_MS);
  bool hitEndUnexpected = homingApproach && es.end && !endGraceActive;
  if (hitEndUnexpected) blocked = true;

  // Homing FSM
  if (g_mode == MODE_HOMING) {
    if (hitEndUnexpected) {
      if (!g_homing_dir_flipped) {
        g_homing_dir_flipped = true;
        g_home_dir_forward = !g_home_dir_forward;
        g_home_phase = HOME_PHASE_FAST_APPROACH;
        g_backoff_remaining = 0;
        g_homing_started_ms = nowMs;
        Serial.println(F("Homing safety: END reached -> flip dir"));
      } else {
        Serial.println(F("Homing error: END reached again, abort"));
        g_mode = MODE_IDLE;
        g_home_phase = HOME_PHASE_IDLE;
      }
    } else {
      switch (g_home_phase) {
        case HOME_PHASE_FAST_APPROACH:
          if (es.home) {
            g_home_phase = HOME_PHASE_BACKOFF;
            g_backoff_remaining = HOME_BACKOFF_USTEPS;
            g_backoff_started_ms = nowMs;
            Serial.println(F("Homing: first hit -> BACKOFF"));
          }
          break;

        case HOME_PHASE_BACKOFF:
          if ((nowMs - g_backoff_started_ms) >= HOME_BACKOFF_MAX_MS) {
            g_backoff_remaining = 0;
          }
          if (g_backoff_remaining == 0 && !es.home) {
            g_home_phase = HOME_PHASE_SLOW_APPROACH;
            Serial.println(F("Homing: released -> SLOW"));
          }
          if (g_backoff_remaining == 0 && es.home) {
            Serial.println(F("Homing error: HOME still pressed after backoff"));
            g_mode = MODE_IDLE;
            g_home_phase = HOME_PHASE_IDLE;
          }
          break;

        case HOME_PHASE_SLOW_APPROACH:
          if (es.home) {
            Serial.println(F("Homing done"));
            g_mode = MODE_IDLE;
            g_home_phase = HOME_PHASE_IDLE;
          }
          break;

        default:
          break;
      }
    }
  }

  // Driver enable (active LOW)
  digitalWrite(PIN_EN, (g_mode == MODE_IDLE || blocked) ? HIGH : LOW);

  // Speed selection
  float trim = readTrimFactor();
  static float smoothedTrim = -1.0f;
  if (smoothedTrim < 0.0f) smoothedTrim = trim;
  smoothedTrim += 0.2f * (trim - smoothedTrim);

  float targetUsps = 0.0f;
  if (g_mode == MODE_HOMING) {
    if      (g_home_phase == HOME_PHASE_BACKOFF)      targetUsps = HOMING_BACKOFF_USPS;
    else if (g_home_phase == HOME_PHASE_SLOW_APPROACH)targetUsps = HOMING_SLOW_USPS;
    else                                              targetUsps = HOMING_FAST_USPS;
  } else {
    targetUsps = BASE_USPS * smoothedTrim;
  }

  // OLED update
  updateOled(forward, smoothedTrim, targetUsps, es);

  // Step control:
  //  - TRACKING uses Timer1
  //  - HOMING uses blocking slices
  g_step_blocked = blocked;
  g_step_run     = (g_mode == MODE_TRACKING);

  // DIR-change guard for timer-driven tracking
  static unsigned long lastDirChangeUs = 0;
  static bool lastForward = false;
  if (g_mode == MODE_TRACKING && forward != lastForward) {
    lastForward = forward;
    lastDirChangeUs = micros();
  }
  if (g_mode == MODE_TRACKING && (long)(micros() - lastDirChangeUs) < 500) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { g_pause_ticks = usToTicks(500); }
  }

  // Update timer interval only matters for tracking
  if (g_mode == MODE_TRACKING) {
    applySpeedToTimer(targetUsps);
  }

  // Homing stepping slice (continuous within slice)
  if (g_mode == MODE_HOMING && !blocked) {
    uint32_t rate = (targetUsps < 1.0f) ? 1UL : (uint32_t)(targetUsps + 0.5f);
    stepMotorSlice(rate, 20, towardHome);
  }
}