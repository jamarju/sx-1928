#include "debug.h"
#include "receiver.h"
#include "motors.h"
#include "main.h"
#include "version.h"
#include <avr/io.h>

// Timing for periodic prints
static unsigned long last_print = 0;
static const unsigned long PRINT_INTERVAL = 100; // Print every 100ms

// Debug toggle bitfield
static struct {
  uint8_t control_mode : 1;
  uint8_t throttle : 1;
  uint8_t steering : 1;
  uint8_t ch1 : 1;
  uint8_t ch3 : 1;
  uint8_t ch5 : 1;
  uint8_t ch6 : 1;
  uint8_t ch7 : 1;
} debug_flags = {0, 0, 0, 0, 0, 0, 0, 0};

// Master pause flag
static bool debug_paused = false;

// MOSTERRAK ASCII art logo in PROGMEM
const char MOSTERRAK_LOGO[] PROGMEM = 
  "• ▌ ▄ ·.       .▄▄ · ▄▄▄▄▄▄▄▄ .▄▄▄  ▄▄▄   ▄▄▄· ▄ •▄\n"
  "·██ ▐███▪▪     ▐█ ▀. •██  ▀▄.▀·▀▄ █·▀▄ █·▐█ ▀█ █▌▄▌▪\n"
  "▐█ ▌▐▌▐█· ▄█▀▄ ▄▀▀▀█▄ ▐█.▪▐▀▀▪▄▐▀▀▄ ▐▀▀▄ ▄█▀▀█ ▐▀▀▄·\n"
  "██ ██▌▐█▌▐█▌.▐▌▐█▄▪▐█ ▐█▌·▐█▄▄▌▐█•█▌▐█•█▌▐█ ▪▐▌▐█.█▌\n"
  "▀▀  █▪▀▀▀ ▀█▄▀▪ ▀▀▀▀  ▀▀▀  ▀▀▀ .▀  ▀.▀  ▀ ▀  ▀ ·▀  ▀\n"
  "\n";

// Version string with build info in PROGMEM
const char VERSION_INFO_STR[] PROGMEM = 
  "Version: " FW_GIT_VERSION " (" FW_BUILD_DATE " " FW_BUILD_TIME ")";

void setup_debug() {
  Serial.begin(115200);
}


void print_help() {
  Serial.print(FPSTR(MOSTERRAK_LOGO));
  Serial.println(FPSTR(VERSION_INFO_STR));
  Serial.print(F("\n"
    "Debug help:\n"
    "\n"
    "c - Toggle control mode display\n"
    "t - Toggle throttle info (target, current, A1, A2, OCR2A)\n"
    "s - Toggle steering info\n"
    "1 - Toggle CH1 (steer) receiver info\n"
    "3 - Toggle CH3 (throttle) receiver info\n"
    "5 - Toggle CH5 (reverse) receiver info\n"
    "6 - Toggle CH6 (max throttle) receiver info\n"
    "7 - Toggle CH7 (takeover) receiver info\n"
    "SPACE - Pause/resume debug output\n"
    "h - Show this help\n"
  ));
}

void process_debug_input() {
  if (!Serial.available()) return;
  
  char cmd = Serial.read();
  switch (cmd) {
    case 'c': debug_flags.control_mode = !debug_flags.control_mode; break;
    case 't': debug_flags.throttle = !debug_flags.throttle; break;
    case 's': debug_flags.steering = !debug_flags.steering; break;
    case '1': debug_flags.ch1 = !debug_flags.ch1; break;
    case '3': debug_flags.ch3 = !debug_flags.ch3; break;
    case '5': debug_flags.ch5 = !debug_flags.ch5; break;
    case '6': debug_flags.ch6 = !debug_flags.ch6; break;
    case '7': debug_flags.ch7 = !debug_flags.ch7; break;
    case ' ': debug_paused = !debug_paused; break;
    case 'h':
    case 'H':
    case '?':
      print_help();
      break;
  }
}

void print_debug_status() {
  unsigned long now = millis();
  
  // Check if paused
  if (debug_paused) return;
  
  // Only print at specified interval
  if (now - last_print < PRINT_INTERVAL) return;
  last_print = now;
  
  // Check if any debug output is enabled
  if (!debug_flags.control_mode && !debug_flags.throttle && !debug_flags.steering &&
      !debug_flags.ch1 && !debug_flags.ch3 && !debug_flags.ch5 && 
      !debug_flags.ch6 && !debug_flags.ch7) {
    return;
  }
  
  char buf[40];  // Reusable small buffer
  bool need_separator = false;
  
  // Control mode
  if (debug_flags.control_mode) {
    const char* mode_str = "UNKNOWN";
    switch (control_mode) {
      case WAIT_TX: mode_str = "WAIT_TX"; break;
      case ARMING_REMOTE_CONTROL: mode_str = "ARM_RC "; break;
      case ARMING_KID_CONTROL: mode_str = "ARM_KID"; break;
      case SWITCHING_TO_REMOTE_CONTROL: mode_str = "SW_RC  "; break;
      case SWITCHING_TO_KID_CONTROL: mode_str = "SW_KID "; break;
      case REMOTE_CONTROL: mode_str = "RC     "; break;
      case KID_CONTROL: mode_str = "KID    "; break;
    }
    Serial.print(F("C:"));
    Serial.print(mode_str);
    need_separator = true;
  }
  
  // Throttle info
  if (debug_flags.throttle) {
    if (need_separator) Serial.print(F(" | "));
    int16_t ramped = get_ramped_speed();
    uint8_t throttle_target = get_throttle();
    bool reverse = get_reverse();
    int16_t target_signed = reverse ? -throttle_target : throttle_target;
    uint8_t a1 = digitalRead(22);
    uint8_t a2 = digitalRead(23);
    sprintf(buf, "T:tgt=%4d cur=%4d A%d%d OCR2A=%3d", 
            target_signed, ramped, a1, a2, OCR2A);
    Serial.print(buf);
    need_separator = true;
  }
  
  // Steering info
  if (debug_flags.steering) {
    if (need_separator) Serial.print(F(" | "));
    uint8_t steer_in = get_steering();
    uint8_t b1 = digitalRead(24);
    uint8_t b2 = digitalRead(25);
    sprintf(buf, "S:in=%3d B%d%d OCR2B=%3d", steer_in, b1, b2, OCR2B);
    Serial.print(buf);
    need_separator = true;
  }
  
  // TX status and channels
  bool tx_on = is_tx_on();
  
  // CH1 - Steering
  if (debug_flags.ch1) {
    if (need_separator) Serial.print(F(" | "));
    if (tx_on) {
      sprintf(buf, "1:STEER %4uus (%3d)", get_raw_steering(), get_steering());
    } else {
      sprintf(buf, "1:STEER %4uus (N/A)", get_raw_steering());
    }
    Serial.print(buf);
    need_separator = true;
  }
  
  // CH3 - Throttle
  if (debug_flags.ch3) {
    if (need_separator) Serial.print(F(" | "));
    if (tx_on) {
      sprintf(buf, "3:THROT %4uus (%3d)", get_raw_throttle(), get_throttle());
    } else {
      sprintf(buf, "3:THROT %4uus (N/A)", get_raw_throttle());
    }
    Serial.print(buf);
    need_separator = true;
  }
  
  // CH5 - Reverse
  if (debug_flags.ch5) {
    if (need_separator) Serial.print(F(" | "));
    if (tx_on) {
      sprintf(buf, "5:REV   %4uus (%s)", get_raw_reverse(), get_reverse() ? "ON " : "OFF");
    } else {
      sprintf(buf, "5:REV   %4uus (N/A)", get_raw_reverse());
    }
    Serial.print(buf);
    need_separator = true;
  }
  
  // CH6 - Max Throttle
  if (debug_flags.ch6) {
    if (need_separator) Serial.print(F(" | "));
    if (tx_on) {
      sprintf(buf, "6:MAXTH %4uus (%3d)", get_raw_max_throttle(), get_max_throttle());
    } else {
      sprintf(buf, "6:MAXTH %4uus (N/A)", get_raw_max_throttle());
    }
    Serial.print(buf);
    need_separator = true;
  }
  
  // CH7 - Takeover
  if (debug_flags.ch7) {
    if (need_separator) Serial.print(F(" | "));
    if (tx_on) {
      sprintf(buf, "7:TAKEO %4uus (%s)", get_raw_takeover(), get_takeover() ? "RC " : "KID");
    } else {
      sprintf(buf, "7:TAKEO %4uus (N/A)", get_raw_takeover());
    }
    Serial.print(buf);
  }
  
  Serial.println();
}
