#include <Arduino.h>
#include <avr/wdt.h>
#include "receiver.h"
#include "motors.h"
#include "debug.h"
#include "main.h"
#include "onboard.h"
#include "version.h"

// Global state (non-static so debug.cpp can access)
ControlMode control_mode = WAIT_TX;         // Start in waiting for TX to be powered on
static bool last_takeover_state = false;    // Track takeover changes

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
  
  // Initialize onboard kid control hardware
  setup_onboard();
  
  // Enable watchdog timer - 500ms timeout
  // If loop() doesn't call wdt_reset() within 500ms, MCU will reset
  wdt_enable(WDTO_500MS);

  // Print firmware version
  Serial.print(FPSTR(MOSTERRAK_LOGO));
  Serial.println(FPSTR(VERSION_INFO_STR));
  Serial.println(F("Press 'h' for help"));
}

void loop() {
  wdt_reset();  // Pet the watchdog
  
  // Process debug commands
  process_debug_input();
  
  // Read all inputs
  bool tx_powered_on = is_tx_on();
  uint8_t steering = get_steering();
  uint8_t throttle = get_throttle();
  bool takeover_active = get_takeover();
  bool reverse_switch = get_reverse();
  uint8_t ramped_speed = get_ramped_speed();
  int16_t max_throttle = get_max_throttle();
  
  // Read onboard control states
  bool rev_pedal = get_rev_pedal();
  bool fwd_pedal = get_fwd_pedal();
  bool speed_low = get_speed_low();
  
  // Common state transitions (apply to all states)
  if (!tx_powered_on) {
    // Switch to WAIT_TX state if TX loss, this gets top priority
    control_mode = WAIT_TX;
  } else if (takeover_active != last_takeover_state) {
    // Switch to RC or KID control mode if takeover changed
    last_takeover_state = takeover_active;
    control_mode = takeover_active ? SWITCHING_TO_REMOTE_CONTROL : SWITCHING_TO_KID_CONTROL;
  }
  
  // Control state machine - each case handles its own state transitions
  switch (control_mode) {
    case WAIT_TX:
      ramp_motors(0);
      if (tx_powered_on && takeover_active) {
        control_mode = ARMING_REMOTE_CONTROL;
      }
      break;

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

    case REMOTE_CONTROL: {
      int16_t throttle_sign = reverse_switch ? -1 : 1;
      ramp_motors(throttle * throttle_sign);
      update_steering(steering);
      break;
    }

    case KID_CONTROL:
      update_steering(steering);
      if (!fwd_pedal && !rev_pedal) ramp_motors(0);
      else if (fwd_pedal && speed_low) ramp_motors(max_throttle / 2);
      else if (fwd_pedal && !speed_low) ramp_motors(max_throttle);
      else if (!fwd_pedal && speed_low) ramp_motors(-max_throttle / 2);
      else if (!fwd_pedal && !speed_low) ramp_motors(-max_throttle);
      break;
  }
  
  // Debug output
  print_debug_status();
}
