#ifndef RECEIVER_H
#define RECEIVER_H

#include <Arduino.h>

// Initialize the PWM receiver system
void setup_receiver();

// Raw pulse width functions (returns microseconds)
uint16_t get_raw_steering();    // Pin 48 - CH1 analog
uint16_t get_raw_throttle();    // Pin 49 - CH3 analog  
uint16_t get_raw_reverse();     // Pin 2  - CH5 digital
uint16_t get_raw_takeover();    // Pin 3  - CH7 digital

// Processed data functions
uint8_t get_steering();         // 0-255 (1100-1900us mapped)
uint8_t get_throttle();         // 0-255 (1100-1900us mapped, inverted)
bool get_reverse();             // true if >1500us, false if <=1500us
bool get_takeover();            // true if <1600us (RC mode), false if >=1600us (kids mode)

// TX status - returns true only if all 4 control channels are active
bool is_tx_on();

#endif // RECEIVER_H
