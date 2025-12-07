#ifndef ONBOARD_H
#define ONBOARD_H

#include <Arduino.h>

// Initialize onboard kid control hardware
void setup_onboard();

// Read onboard control states
bool get_rev_pedal();    // Returns true if reverse pedal is pressed
bool get_fwd_pedal();    // Returns true if forward pedal is pressed
bool get_speed_low();    // Returns true if low speed mode is active

#endif // ONBOARD_H

