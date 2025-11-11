#ifndef DEBUG_H
#define DEBUG_H

#include <Arduino.h>
#include "version.h"

#define FPSTR(p) (reinterpret_cast<const __FlashStringHelper *>(p))

// PROGMEM logo string
extern const char MOSTERRAK_LOGO[] PROGMEM;

// PROGMEM version string with build info
extern const char VERSION_INFO_STR[] PROGMEM;

void setup_debug();
void process_debug_input();
void print_debug_status();

#endif // DEBUG_H

