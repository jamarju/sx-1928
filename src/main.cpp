#include <Arduino.h>
#include <avr/wdt.h>
#include "receiver.h"
#include "motors.h"
#include "debug.h"
#include "main.h"

// Kid control hardware pins (with internal pullups)
static const uint8_t PIN_REV_PEDAL = 26;    // LOW = REV + pedal pressed
static const uint8_t PIN_FWD_PEDAL = 27;    // LOW = FWD + pedal pressed
static const uint8_t PIN_SPEED_LOW = 28;     // LOW = HI speed, HIGH = LO speed

// Kid control speed settings
static const int16_t SPEED_HI = 150;
static const int16_t SPEED_LO = 75;

// Global state (non-static so debug.cpp can access)
ControlMode control_mode = ARMING_REMOTE_CONTROL;  // Start in remote arming
static bool last_takeover_state = false;           // Track takeover changes

// Disable watchdog at boot to prevent reset loop
void wdt_init(void) __attribute__((naked)) __attribute__((section(".init3")));
void wdt_init(void) {
  MCUSR = 0;
  wdt_disable();
}

void setup() {
  // Initialize debug serial output
  setup_debug();
  
  // Initialize the PWM receiver system
  setup_receiver();
  
  // Initialize motor control system  
  setup_motors();
  
  // Initialize kid control input pins with pullups
  pinMode(PIN_REV_PEDAL, INPUT_PULLUP);
  pinMode(PIN_FWD_PEDAL, INPUT_PULLUP);
  pinMode(PIN_SPEED_LOW, INPUT_PULLUP);
  
  // Enable watchdog timer - 500ms timeout
  // If loop() doesn't call wdt_reset() within 500ms, MCU will reset
  wdt_enable(WDTO_500MS);
}

void loop() {
  wdt_reset();  // Pet the watchdog
  
  // Read all inputs
  uint8_t steering = get_steering();
  uint8_t throttle = get_throttle();
  bool takeover_active = get_takeover();
  bool reverse_switch = get_reverse();
  int16_t throttle_sign = reverse_switch ? -1 : 1;
  uint8_t ramped_speed = get_ramped_speed();
  
  bool rev_pedal = !digitalRead(PIN_REV_PEDAL);  // Active LOW
  bool fwd_pedal = !digitalRead(PIN_FWD_PEDAL);  // Active LOW
  bool speed_low = !digitalRead(PIN_SPEED_LOW);    // Active LOW
  
  // Detect takeover change and initiate mode switch
  if (takeover_active != last_takeover_state) {
    last_takeover_state = takeover_active;
    control_mode = takeover_active ? SWITCHING_TO_REMOTE_CONTROL : SWITCHING_TO_KID_CONTROL;
  }

  // Control state machine
  switch (control_mode) {
    case SWITCHING_TO_REMOTE_CONTROL:
      ramp_motors(0);
      if (ramped_speed == 0) {
        control_mode = ARMING_REMOTE_CONTROL;
      }
      break;
    case SWITCHING_TO_KID_CONTROL:
      ramp_motors(0);
      if (ramped_speed == 0) {
        control_mode = ARMING_KID_CONTROL;
      }
      break;
    case ARMING_REMOTE_CONTROL:
      ramp_motors(0);
      if (throttle == 0 && ramped_speed == 0 && !reverse_switch) {
        control_mode = REMOTE_CONTROL;
      }
      break;
    case ARMING_KID_CONTROL:
      ramp_motors(0);
      if (!rev_pedal && !fwd_pedal && ramped_speed == 0) {
        control_mode = KID_CONTROL;
      }
      break;
    case REMOTE_CONTROL:
      ramp_motors(throttle * throttle_sign);
      update_steering(steering);
      break;
    case KID_CONTROL:
      disable_steering();
      if (!fwd_pedal && !rev_pedal) ramp_motors(0);
      else if (fwd_pedal && speed_low) ramp_motors(SPEED_LO);
      else if (fwd_pedal && !speed_low) ramp_motors(SPEED_HI);
      else if (!fwd_pedal && speed_low) ramp_motors(-SPEED_LO);
      else if (!fwd_pedal && !speed_low) ramp_motors(-SPEED_HI);
      break;
  }
  
  // Debug output
  print_debug_status();
}
