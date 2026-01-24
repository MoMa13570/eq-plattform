#ifdef TMC2209_BREADBOARD_TEST
#include <Arduino.h>
#include <TMCStepper.h>
#include <SoftwareSerial.h>

// -------------------- Pins --------------------
const int PIN_STEP = 2;
const int PIN_DIR  = 3;
const int PIN_EN   = 8;

// 3-position direction switch (ON-OFF-ON)
const int PIN_SW_LEFT  = 4;   // active LOW
const int PIN_SW_RIGHT = 5;   // active LOW

// UART (Single Wire via UART-Pin am BTT TMC2209 v1.3)
const int PIN_TMC_RX = 6;   // Arduino RX
const int PIN_TMC_TX = 7;   // Arduino TX (1k in Serie empfohlen)

// -------------------- TMC2209 --------------------
static constexpr float R_SENSE = 0.11f;   // BTT v1.3
static constexpr uint8_t TMC_ADDR = 0;

SoftwareSerial TMC_SERIAL(PIN_TMC_RX, PIN_TMC_TX);
TMC2209Stepper driver(&TMC_SERIAL, R_SENSE, TMC_ADDR);

// Forward declaration
void stepMotor(uint16_t stepsPerSecond, uint32_t durationMs);

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println(F("=== TMC2209 UART TEST ==="));

  // Pins
  pinMode(PIN_STEP, OUTPUT);
  pinMode(PIN_DIR, OUTPUT);
  pinMode(PIN_EN, OUTPUT);
  pinMode(PIN_SW_LEFT, INPUT_PULLUP);
  pinMode(PIN_SW_RIGHT, INPUT_PULLUP);

  digitalWrite(PIN_EN, HIGH); // disable driver initially
  digitalWrite(PIN_DIR, LOW);

  // UART
  TMC_SERIAL.begin(115200);

  // ---- TMC init ----
  driver.begin();
  driver.pdn_disable(true);        // UART mode
  driver.I_scale_analog(false);    // ignore Vref pot
  driver.toff(4);                  // enable driver
  driver.blank_time(24);
  driver.rms_current(500);         // 500 mA RMS (safe start)
  driver.microsteps(32);
  driver.intpol(true);             // interpolate to 256
  driver.en_spreadCycle(false);    // stealthChop (quiet)
  driver.pwm_autoscale(true);

  // Enable driver
  digitalWrite(PIN_EN, LOW);

  // ---- UART sanity check ----
  Serial.print(F("DRV_STATUS: 0x"));
  Serial.println(driver.DRV_STATUS(), HEX);

  Serial.println(F("Motor will step forward for 3s..."));
}

// -------------------- Loop --------------------
void loop() {
  bool left  = (digitalRead(PIN_SW_LEFT)  == LOW);
  bool right = (digitalRead(PIN_SW_RIGHT) == LOW);

  // Middle position or invalid state: motor off
  if ((left && right) || (!left && !right)) {
    digitalWrite(PIN_EN, HIGH);   // disable driver
    return;
  }

  // Enable driver
  digitalWrite(PIN_EN, LOW);

  if (left) {
    digitalWrite(PIN_DIR, LOW);   // rotate left
  } else if (right) {
    digitalWrite(PIN_DIR, HIGH);  // rotate right
  }

  // step while switch is held
  stepMotor(600, 20);  // small time slice for responsive stop
}

// -------------------- Step helper --------------------
void stepMotor(uint16_t stepsPerSecond, uint32_t durationMs) {
  uint32_t intervalUs = 1000000UL / stepsPerSecond;
  uint32_t start = millis();

  while (millis() - start < durationMs) {
    digitalWrite(PIN_STEP, HIGH);
    delayMicroseconds(3);
    digitalWrite(PIN_STEP, LOW);
    delayMicroseconds(intervalUs - 3);
  }
}
#endif // TMC2209_BREADBOARD_TEST