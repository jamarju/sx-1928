#ifndef MOTORS_H
#define MOTORS_H

#include <Arduino.h>

void setup_motors();
void ramp_motors(int16_t speed);
void update_steering(uint8_t steering);
uint16_t get_ramped_speed();
void update_motors(int16_t speed);
void disable_motors();
void disable_steering();

#endif // MOTORS_H
