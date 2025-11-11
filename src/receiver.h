#ifndef RECEIVER_H
#define RECEIVER_H

#include <Arduino.h>

// Initialize the 5-channel PWM receiver system
void setup_receiver();

// Raw pulse width functions (returns microseconds)
uint16_t get_raw_steering();    // Pin 48 - CH1 analog
uint16_t get_raw_throttle();    // Pin 49 - CH3 analog  
uint16_t get_raw_reverse();     // Pin 2  - CH5 digital
uint16_t get_raw_max_throttle();// Pin 18 - CH6 analog
uint16_t get_raw_takeover();    // Pin 3  - CH7 digital

// Processed data functions
uint8_t get_steering();         // 0-255 (1100-1900us mapped)
uint8_t get_throttle();         // 0-255 (1100-1900us mapped, inverted)
bool get_reverse();             // true if >1500us, false if <=1500us
uint8_t get_max_throttle();     // 0-255 (1100-1900us mapped)
bool get_takeover();            // true if <1600us (RC mode), false if >=1600us (kids mode)

// Channel status (returns true if signal present, false if N/A)
bool is_steering_active();
bool is_throttle_active();
bool is_reverse_active();
bool is_max_throttle_active();
bool is_takeover_active();

#endif // RECEIVER_H

