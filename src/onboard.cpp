#include "onboard.h"

// Kid control hardware pins (with internal pullups)
static const uint8_t PIN_REV_PEDAL = 26;    // LOW = REV + pedal pressed
static const uint8_t PIN_FWD_PEDAL = 27;    // LOW = FWD + pedal pressed
static const uint8_t PIN_SPEED_LOW = 28;    // LOW = HI speed, HIGH = LO speed

void setup_onboard() {
  // Initialize kid control input pins with pullups
  pinMode(PIN_REV_PEDAL, INPUT_PULLUP);
  pinMode(PIN_FWD_PEDAL, INPUT_PULLUP);
  pinMode(PIN_SPEED_LOW, INPUT_PULLUP);
}

bool get_rev_pedal() {
  return !digitalRead(PIN_REV_PEDAL);  // Active LOW
}

bool get_fwd_pedal() {
  return !digitalRead(PIN_FWD_PEDAL);  // Active LOW
}

bool get_speed_low() {
  return !digitalRead(PIN_SPEED_LOW);  // Active LOW
}

