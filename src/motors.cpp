#include "motors.h"
#include <avr/io.h>

// Drive motor control pin assignments
// Direction control: A1=0,A2=0 (brake), A1=1,A2=0 (fwd), A1=0,A2=1 (rev)
static const uint8_t A1_PIN = 22;            // PA0 - Direction control bit 1
static const uint8_t A2_PIN = 23;            // PA1 - Direction control bit 2
static const uint8_t PA_PWM_PIN = 10;        // Timer2 OC2A - PWM speed control

// Steering motor control pin assignments
// Direction control: B1=0,B2=0 (brake), B1=1,B2=0 (right), B1=0,B2=1 (left)
static const uint8_t B1_PIN = 24;            // PA2 - Direction control bit 1
static const uint8_t B2_PIN = 25;            // PA3 - Direction control bit 2
static const uint8_t PB_PWM_PIN = 9;         // Timer2 OC2B - PWM steering control

// Drive motor ramping constants (units per second)
// RAMP_UP_RATE: 0 to 255 in 5.0 seconds -> 255 / 5.0 = 51.0 units/sec
// RAMP_DN_RATE: 255 to 0 in 1.0 seconds -> 255 / 1.0 = 255.0 units/sec
static const float RAMP_UP_RATE = 51.0;
static const float RAMP_DN_RATE = 255.0;

// Drive motor ramping state
static float current_speed = 0.0;          // Current speed (0.0 to 255.0)
static unsigned long last_update_time = 0; // Last update timestamp in milliseconds

// Steering motor constants
static const uint8_t STEER_CENTER_VALUE = 128;     // Center position value
static const uint8_t STEER_DEADZONE = 16;          // Deadzone radius around center
static const uint8_t STEER_FULL_PWM = 254;         // Full power PWM (driver doesn't handle 255)
static const uint8_t STEER_HOLD_PWM = 13;          // Hold power PWM (~5%)
static const unsigned long STEER_HOLD_TIME = 2000; // Time before switching to hold (ms)

// Start of steering
static unsigned long steering_start_time = 0;

// Steering state machine
enum SteeringStickPosition {
  STEER_CENTER,
  STEER_LEFT,
  STEER_RIGHT,
};

static SteeringStickPosition prev_steer_state = STEER_CENTER;

void setup_motors() {
  // Configure drive motor control pins
  pinMode(A1_PIN, OUTPUT);
  pinMode(A2_PIN, OUTPUT); 
  pinMode(PA_PWM_PIN, OUTPUT);
  
  // Initialize to safe state (brake: A1=0, A2=0, PWM=0)
  digitalWrite(A1_PIN, LOW);
  digitalWrite(A2_PIN, LOW);
  
  // Setup Timer2 for Phase Correct PWM mode at ~3.9kHz (16MHz / (8 * 2 * 256))
  // Pin 10 = PB4 = OC2A (drive motor), Pin 9 = PH6 = OC2B (steering motor)
  
  // Configure Timer2 for Phase Correct PWM mode, prescaler 8
  TCCR2A = _BV(COM2A1) | _BV(COM2B1) | _BV(WGM20); // Phase Correct PWM mode, both OC2A and OC2B
  TCCR2B = _BV(CS21);  // Prescaler 8: CS22=0, CS21=1, CS20=0
  
  // Initialize PWM duty cycles to 0 (stopped)
  OCR2A = 0;  // PA_PWM_PIN (pin 10) - drive motor
  OCR2B = 0;  // PB_PWM_PIN (pin 9) - steering motor
  
  // Initialize ramping state
  current_speed = 0.0;
  last_update_time = millis();
  
  // Configure steering motor control pins
  pinMode(B1_PIN, OUTPUT);
  pinMode(B2_PIN, OUTPUT);
  pinMode(PB_PWM_PIN, OUTPUT);
  
  // Initialize steering to safe state (high-Z mode: B1=1, B2=1, PWM=255)
  disable_steering();
}

void ramp_motors(int16_t target_speed) {
  // Get elapsed time since last update
  unsigned long now = millis();
  unsigned long elapsed_ms = now - last_update_time;
  last_update_time = now;
  
  // Clamp to 254 max - driver doesn't handle 255 correctly
  if (target_speed < -254) target_speed = -254;
  if (target_speed > 254) target_speed = 254;

  // Convert target to float and calculate elapsed time in seconds
  float target_speed_f = (float)target_speed;
  float elapsed_sec = elapsed_ms / 1000.0;
  
  // Apply ramping
  if (current_speed < target_speed_f) {
    if (current_speed > 0) {
      // We are speeding up
      current_speed += RAMP_UP_RATE * elapsed_sec;
    } else {
      // We are slowing down
      current_speed += RAMP_DN_RATE * elapsed_sec;
    }
    if (current_speed > target_speed_f) {
      current_speed = target_speed_f;  // Clamp to target
    }
  }
  else if (current_speed > target_speed_f) {
    if (current_speed > 0) {
      // We are slowing down
      current_speed -= RAMP_DN_RATE * elapsed_sec;
    } else {
      // We are speeding up (in reverse)
      current_speed -= RAMP_UP_RATE * elapsed_sec;
    }
    if (current_speed < target_speed_f) {
      current_speed = target_speed_f;  // Clamp to target
    }
  }
  // else: already at target, no change needed
  
  // Apply to motors using the rounded ramped speed
  update_motors(get_ramped_speed());
}

uint16_t get_ramped_speed() {
  // Convert float speed to int16_t with rounding
  if (current_speed < 0) return (int16_t)(current_speed - 0.5);
  else return (int16_t)(current_speed + 0.5);
}

void disable_motors() {
  digitalWrite(A1_PIN, HIGH);
  digitalWrite(A2_PIN, HIGH);
  OCR2A = 255;
}

void update_motors(int16_t speed) {
  // Simply apply the requested speed and direction - no business logic
  
  if (speed == 0) {
    // Brake: A1=0, A2=0 (regardless of reverse flag)
    digitalWrite(A1_PIN, LOW);
    digitalWrite(A2_PIN, LOW);
    OCR2A = 0;
  }
  else if (speed < 0) {
    // Reverse: A1=0, A2=1
    digitalWrite(A1_PIN, LOW);
    digitalWrite(A2_PIN, HIGH);
    OCR2A = -speed;
  }
  else {
    // Forward: A1=1, A2=0
    digitalWrite(A1_PIN, HIGH);
    digitalWrite(A2_PIN, LOW);
    OCR2A = speed;
  }
}

void steer_right(uint8_t pwm_duty) {
  digitalWrite(B1_PIN, HIGH);
  digitalWrite(B2_PIN, LOW);
  OCR2B = pwm_duty;
}

void steer_left(uint8_t pwm_duty) {
  digitalWrite(B1_PIN, LOW);
  digitalWrite(B2_PIN, HIGH);
  OCR2B = pwm_duty;
}

void disable_steering() {
  digitalWrite(B1_PIN, HIGH);
  digitalWrite(B2_PIN, HIGH);
  OCR2B = 255;
}


void update_steering(uint8_t steering) {
  // Calculate deadzone boundaries
  uint8_t center_low = STEER_CENTER_VALUE - STEER_DEADZONE;
  uint8_t center_high = STEER_CENTER_VALUE + STEER_DEADZONE;
  SteeringStickPosition new_steer_state;

  if (steering < center_low) {
    new_steer_state = STEER_LEFT;
  } else if (steering >= center_high) {
    new_steer_state = STEER_RIGHT;
  } else {
    new_steer_state = STEER_CENTER;
  }

  if (new_steer_state != prev_steer_state) {
    prev_steer_state = new_steer_state;
    steering_start_time = millis();
  }

  switch (new_steer_state) {
    case STEER_CENTER:
      disable_steering();
      break;
    case STEER_LEFT:
      if (millis() - steering_start_time < STEER_HOLD_TIME) {
        steer_left(map(steering, center_low, 0, STEER_HOLD_PWM, STEER_FULL_PWM));
      } else {
        steer_left(STEER_HOLD_PWM);
      }
      break;
    case STEER_RIGHT:
      if (millis() - steering_start_time < STEER_HOLD_TIME) {
        steer_right(map(steering, center_high, 255, STEER_HOLD_PWM, STEER_FULL_PWM));
      } else {
        steer_right(STEER_HOLD_PWM);
      }
      break;
  }
}
