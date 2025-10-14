#include "debug.h"
#include "receiver.h"
#include "motors.h"
#include "main.h"
#include <avr/io.h>

// Timing for periodic prints
static unsigned long last_print = 0;
static const unsigned long PRINT_INTERVAL = 100; // Print every 100ms

void setup_debug() {
  Serial.begin(115200);
}

void print_debug_status() {
  unsigned long now = millis();
  
  // Only print at specified interval
  if (now - last_print < PRINT_INTERVAL) {
    return;
  }
  last_print = now;
  
  // Print all channels on one line with both raw and processed values
  Serial.print(F("Steer: "));
  if (is_steering_active()) {
    Serial.print(get_raw_steering());
    Serial.print(F("us/"));
    Serial.print(get_steering());
  } else {
    Serial.print(F("N/A"));
  }
  
  Serial.print(F(" | Throttle: "));
  if (is_throttle_active()) {
    Serial.print(get_raw_throttle());
    Serial.print(F("us/"));
    Serial.print(get_throttle());
  } else {
    Serial.print(F("N/A"));
  }
  
  Serial.print(F(" | Reverse: "));
  if (is_reverse_active()) {
    Serial.print(get_raw_reverse());
    Serial.print(F("us/"));
    Serial.print(get_reverse() ? F("ON") : F("OFF"));
  } else {
    Serial.print(F("N/A"));
  }
  
  Serial.print(F(" | Takeover: "));
  if (is_takeover_active()) {
    Serial.print(get_raw_takeover());
    Serial.print(F("us/"));
    Serial.print(get_takeover() ? F("ON") : F("OFF"));
  } else {
    Serial.print(F("N/A"));
  }
  
  Serial.print(F(" | MaxThrottle: "));
  if (is_max_throttle_active()) {
    Serial.print(get_raw_max_throttle());
    Serial.print(F("us/"));
    Serial.print(get_max_throttle());
  } else {
    Serial.print(F("N/A"));
  }
  
  Serial.print(F("/"));
  Serial.print(get_takeover() ? F("EN") : F("DIS"));
  Serial.print(F("/"));
  Serial.print(get_reverse() ? F("REV") : F("FWD"));
  Serial.print(F("/rmp="));
  Serial.print(get_ramped_speed());  // Ramped uint8_t value
  Serial.print(F("/OCR2A="));
  Serial.print(OCR2A);  // Actual PWM register
  
  // Add steering debug info
  Serial.print(F(" | Steer: in="));
  Serial.print(get_steering());  // Steering input value
  Serial.print(F(" OCR2B="));
  Serial.print(OCR2B);
  
  // Print control mode
  Serial.print(F(" | Mode: "));
  switch (control_mode) {
    case ARMING_REMOTE_CONTROL:
      Serial.print(F("ARM_RC"));
      break;
    case ARMING_KID_CONTROL:
      Serial.print(F("ARM_KID"));
      break;
    case SWITCHING_TO_REMOTE_CONTROL:
      Serial.print(F("SWITCH->RC"));
      break;
    case SWITCHING_TO_KID_CONTROL:
      Serial.print(F("SWITCH->KID"));
      break;
    case REMOTE_CONTROL:
      Serial.print(F("RC"));
      break;
    case KID_CONTROL:
      Serial.print(F("KID"));
      break;
  }
  
  Serial.println();
}

