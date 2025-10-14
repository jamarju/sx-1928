#include "receiver.h"
#include <avr/io.h>
#include <avr/interrupt.h>

// Pin assignments with logical names
static const uint8_t STEERING_PIN = 48;      // ICP5 (Timer5) - Input Capture
static const uint8_t THROTTLE_PIN = 49;      // ICP4 (Timer4) - Input Capture  
static const uint8_t REVERSE_PIN = 2;        // INT4 - External Interrupt
static const uint8_t TAKEOVER_PIN = 3;       // INT5 - External Interrupt
static const uint8_t MAX_THROTTLE_PIN = 18;  // INT3 - External Interrupt

// PWM signal loss detection (configurable timeout)
static const uint16_t PWM_TIMEOUT_MS = 100;  // Signal loss timeout in milliseconds

// Steering: Timer5 Input Capture (pin 48)
volatile uint16_t steering_t_rise = 0;
volatile unsigned long steering_last_activity = 0;

// Throttle: Timer4 Input Capture (pin 49)  
volatile uint16_t throttle_t_rise = 0;
volatile unsigned long throttle_last_activity = 0;

// Reverse: External interrupt (pin 2)
volatile uint16_t reverse_t_rise = 0;
volatile unsigned long reverse_last_activity = 0;

// Takeover: External interrupt (pin 3)
volatile uint16_t takeover_t_rise = 0;
volatile unsigned long takeover_last_activity = 0;

// Max Throttle: External interrupt (pin 18)
volatile uint16_t max_throttle_t_rise = 0;
volatile unsigned long max_throttle_last_activity = 0;

// Final pulse width values in microseconds (written by ISRs, read by API)
volatile uint16_t steering_us = 1500, throttle_us = 1500, reverse_us = 1500;
volatile uint16_t takeover_us = 1500, max_throttle_us = 1500;

// Timer4 Input Capture ISR (Throttle)
ISR(TIMER4_CAPT_vect) {
  throttle_last_activity = millis();    // Update activity timestamp
  uint16_t t = ICR4;                    // latched timestamp at edge
  if (TCCR4B & _BV(ICES4)) {            // was capturing RISING
    throttle_t_rise = t;                // remember rising time
    TCCR4B &= ~_BV(ICES4);              // next: capture FALLING
  } else {                              // captured FALLING
    uint16_t counts = (uint16_t)(t - throttle_t_rise); // auto handles wrap
    throttle_us = (counts + 1) >> 1;    // Convert to microseconds immediately
    TCCR4B |= _BV(ICES4);               // next: capture RISING
  }
}

// Timer5 Input Capture ISR (Steering)
ISR(TIMER5_CAPT_vect) {
  steering_last_activity = millis();    // Update activity timestamp
  uint16_t t = ICR5;                    // latched timestamp at edge
  if (TCCR5B & _BV(ICES5)) {            // was capturing RISING
    steering_t_rise = t;                // remember rising time
    TCCR5B &= ~_BV(ICES5);              // next: capture FALLING
  } else {                              // captured FALLING
    uint16_t counts = (uint16_t)(t - steering_t_rise); // auto handles wrap
    steering_us = (counts + 1) >> 1;    // Convert to microseconds immediately
    TCCR5B |= _BV(ICES5);               // next: capture RISING
  }
}

// External interrupt ISR for Reverse (INT4)
ISR(INT4_vect) {
  reverse_last_activity = millis();     // Update activity timestamp
  uint16_t now = TCNT1;                 // Use Timer1 for timestamp
  
  if ((EICRB & (_BV(ISC41) | _BV(ISC40))) == (_BV(ISC41) | _BV(ISC40))) {  // was configured for RISING
    reverse_t_rise = now;
    // Switch to falling edge
    EICRB &= ~(_BV(ISC41) | _BV(ISC40));
    EICRB |= _BV(ISC41);  // falling edge (10)
  } else {                              // was configured for FALLING
    uint16_t counts = (uint16_t)(now - reverse_t_rise);
    reverse_us = (counts + 1) >> 1;     // Convert to microseconds immediately
    // Switch back to rising edge
    EICRB &= ~(_BV(ISC41) | _BV(ISC40));
    EICRB |= _BV(ISC41) | _BV(ISC40);  // rising edge (11)
  }
}

// External interrupt ISR for Takeover (INT5) 
ISR(INT5_vect) {
  takeover_last_activity = millis();    // Update activity timestamp
  uint16_t now = TCNT1;                 // Use Timer1 for timestamp
  
  if ((EICRB & (_BV(ISC51) | _BV(ISC50))) == (_BV(ISC51) | _BV(ISC50))) {  // was configured for RISING
    takeover_t_rise = now;
    // Switch to falling edge
    EICRB &= ~(_BV(ISC51) | _BV(ISC50));
    EICRB |= _BV(ISC51);  // falling edge (10)
  } else {                              // was configured for FALLING
    uint16_t counts = (uint16_t)(now - takeover_t_rise);
    takeover_us = (counts + 1) >> 1;    // Convert to microseconds immediately
    // Switch back to rising edge  
    EICRB &= ~(_BV(ISC51) | _BV(ISC50));
    EICRB |= _BV(ISC51) | _BV(ISC50);  // rising edge (11)
  }
}

// External interrupt ISR for Max Throttle (INT3) 
ISR(INT3_vect) {
  max_throttle_last_activity = millis(); // Update activity timestamp
  uint16_t now = TCNT1;                  // Use Timer1 for timestamp
  
  if ((EICRA & (_BV(ISC31) | _BV(ISC30))) == (_BV(ISC31) | _BV(ISC30))) {  // was configured for RISING
    max_throttle_t_rise = now;
    // Switch to falling edge
    EICRA &= ~(_BV(ISC31) | _BV(ISC30));
    EICRA |= _BV(ISC31);  // falling edge (10)
  } else {                              // was configured for FALLING
    uint16_t counts = (uint16_t)(now - max_throttle_t_rise);
    max_throttle_us = (counts + 1) >> 1; // Convert to microseconds immediately
    // Switch back to rising edge  
    EICRA &= ~(_BV(ISC31) | _BV(ISC30));
    EICRA |= _BV(ISC31) | _BV(ISC30);  // rising edge (11)
  }
}

void setup_receiver() {
  // Configure all input pins
  pinMode(STEERING_PIN, INPUT);
  pinMode(THROTTLE_PIN, INPUT);  
  pinMode(REVERSE_PIN, INPUT);
  pinMode(TAKEOVER_PIN, INPUT);
  pinMode(MAX_THROTTLE_PIN, INPUT);
  
  // Initialize activity timestamps
  unsigned long now = millis();
  steering_last_activity = now;
  throttle_last_activity = now;
  reverse_last_activity = now;
  takeover_last_activity = now;
  max_throttle_last_activity = now;
  
  // Timer1: Reference timer for external interrupts (prescaler 8)
  TCCR1A = 0;
  TCCR1B = _BV(CS11);
  TCNT1 = 0;
  
  // Timer4: Input Capture for Throttle (prescaler 8, noise canceller)
  TCCR4A = 0;
  TCCR4B = _BV(CS41) | _BV(ICES4) | _BV(ICNC4);
  TCNT4 = 0;
  TIFR4 |= _BV(ICF4);
  TIMSK4 |= _BV(ICIE4);
  
  // Timer5: Input Capture for Steering (prescaler 8, noise canceller)
  TCCR5A = 0;
  TCCR5B = _BV(CS51) | _BV(ICES5) | _BV(ICNC5);  
  TCNT5 = 0;
  TIFR5 |= _BV(ICF5);
  TIMSK5 |= _BV(ICIE5);
  
  // External interrupts for remaining channels
  // INT4 (pin 2) - Reverse
  EICRB &= ~(_BV(ISC41) | _BV(ISC40));
  EICRB |= _BV(ISC41) | _BV(ISC40);  // rising edge
  EIFR = _BV(INTF4);
  EIMSK |= _BV(INT4);
  
  // INT5 (pin 3) - Takeover  
  EICRB &= ~(_BV(ISC51) | _BV(ISC50));
  EICRB |= _BV(ISC51) | _BV(ISC50);  // rising edge
  EIFR = _BV(INTF5);
  EIMSK |= _BV(INT5);
  
  // INT3 (pin 18) - Max Throttle
  EICRA &= ~(_BV(ISC31) | _BV(ISC30));
  EICRA |= _BV(ISC31) | _BV(ISC30);  // rising edge
  EIFR = _BV(INTF3);
  EIMSK |= _BV(INT3);
  
  sei();
}

// Raw pulse width functions (returns microseconds - ISRs write directly to these)
uint16_t get_raw_steering() {
  return steering_us;
}

uint16_t get_raw_throttle() {
  return throttle_us;
}

uint16_t get_raw_reverse() {
  return reverse_us;
}

uint16_t get_raw_max_throttle() {
  return max_throttle_us;
}

uint16_t get_raw_takeover() {
  return takeover_us;
}

// Safe mapping function using Arduino's map() with clamping and optional inversion
static uint8_t safe_map_to_255(uint16_t pulse_us, uint16_t min_us, uint16_t max_us, bool invert) {
  long mapped = map((long)pulse_us, min_us, max_us, 0L, 255L);  // Use Arduino's map() for safe arithmetic
  
  // Clamp the result to 0-255 range
  if (mapped < 0) mapped = 0;
  if (mapped > 255) mapped = 255;
  
  // Apply inversion if requested
  if (invert) mapped = 255 - mapped;
  
  return (uint8_t)mapped;
}

// Processed data functions
uint8_t get_steering() {
  return safe_map_to_255(steering_us, 1100, 1900, false);
}

uint8_t get_throttle() {
  if (!is_throttle_active()) {
    return 0;  // Return throttle 0 when no signal
  }
  return safe_map_to_255(throttle_us, 1100, 1900, true);  // Inverted: 1100μs→255, 1900μs→0
}

bool get_reverse() {
  if (!is_reverse_active()) {
    return false;  // Return forward direction when no signal
  }
  return reverse_us > 1500;  // true if >1500us, false if <=1500us (reversed behavior)
}

uint8_t get_max_throttle() {
  return safe_map_to_255(max_throttle_us, 1100, 1900, false);
}

bool get_takeover() {
  if (!is_takeover_active()) {
    return false;  // Return takeover disabled when no signal
  }
  return takeover_us <= 1500;  // true if <=1500us, false if >1500us
}

// Channel status (returns true if signal present, false if N/A)
bool is_steering_active() {
  unsigned long now = millis();
  noInterrupts();
  bool active = (now - steering_last_activity) <= PWM_TIMEOUT_MS;
  interrupts();
  return active;
}

bool is_throttle_active() {
  unsigned long now = millis();
  noInterrupts();
  bool active = (now - throttle_last_activity) <= PWM_TIMEOUT_MS;
  interrupts();
  return active;
}

bool is_reverse_active() {
  unsigned long now = millis();
  noInterrupts();
  bool active = (now - reverse_last_activity) <= PWM_TIMEOUT_MS;
  interrupts();
  return active;
}

bool is_max_throttle_active() {
  unsigned long now = millis();
  noInterrupts();
  bool active = (now - max_throttle_last_activity) <= PWM_TIMEOUT_MS;
  interrupts();
  return active;
}

bool is_takeover_active() {
  unsigned long now = millis();
  noInterrupts();
  bool active = (now - takeover_last_activity) <= PWM_TIMEOUT_MS;
  interrupts();
  return active;
}

