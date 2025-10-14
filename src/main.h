#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>

// Control mode state machine
enum ControlMode {
  ARMING_REMOTE_CONTROL,      // Arming for remote control mode
  ARMING_KID_CONTROL,         // Arming for kid control mode
  SWITCHING_TO_REMOTE_CONTROL,// Transitioning to remote control
  SWITCHING_TO_KID_CONTROL,   // Transitioning to kid control
  REMOTE_CONTROL,             // Active remote control
  KID_CONTROL                 // Active kid control
};

// Global control mode state (accessible from debug.cpp)
extern ControlMode control_mode;

#endif // MAIN_H

