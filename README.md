# Futaba R617FS Receiver Interface

This project provides a high-precision PWM signal capture interface for the **Futaba R617FS** receiver using the **Robotdyn Mega 2560 Pro** Arduino board.

## Hardware Setup

### Receiver: Futaba R617FS
- **Model**: R617FS 2.4GHz FASST Compatible Receiver
- **Channels**: 7-channel PWM output
- **Signal**: Standard RC PWM (1000-2000μs pulse width, ~50Hz)
- **Power**: 3.3V-8.4V (connect to Arduino 5V)

### Arduino: Robotdyn Mega 2560 Pro
- **MCU**: ATmega2560
- **Input Capture Units**: Timer4 (ICP4), Timer5 (ICP5)
- **External Interrupts**: INT3, INT4, INT5
- **Timing Resolution**: 0.5μs per tick (16MHz/8 prescaler)

## Pin Mapping

### Receiver Input Pins

| Receiver Channel | Function      | Arduino Pin | Method           | Signal Type | Range/Logic |
|------------------|---------------|-------------|------------------|-------------|-------------|
| **CH1**          | Steering      | **Pin 48**  | Input Capture    | Analog      | 1100-1900μs → 0-255 |
| **CH3**          | Throttle      | **Pin 49**  | Input Capture    | Analog      | 1100-1900μs → 255-0 (inverted) |
| **CH5**          | Reverse       | **Pin 2**   | External Int     | Digital     | >1500μs = ON, ≤1500μs = OFF |
| **CH6**          | Max Throttle  | **Pin 18**  | External Int     | Analog      | 1100-1900μs → 0-255 |
| **CH7**          | Takeover      | **Pin 3**   | External Int     | Digital     | ≤1500μs = ON, >1500μs = OFF |

### Motor Control Output Pins

#### Drive Motor

| Function | Arduino Pin | AVR Pin | Timer/PWM | Notes |
|----------|-------------|---------|-----------|-------|
| **A1** | **Pin 22** | PA0 | - | Direction control bit 1 (digital) |
| **A2** | **Pin 23** | PA1 | - | Direction control bit 2 (digital) |
| **PA_PWM** | **Pin 10** | PB4 | Timer2 OC2A | Motor speed PWM (~3.9kHz Phase Correct) |

**Drive Direction Control:**
- `A1=0, A2=0` → Brake
- `A1=1, A2=0` → Forward
- `A1=0, A2=1` → Reverse

#### Steering Motor

| Function | Arduino Pin | AVR Pin | Timer/PWM | Notes |
|----------|-------------|---------|-----------|-------|
| **B1** | **Pin 24** | PA2 | - | Direction control bit 1 (digital) |
| **B2** | **Pin 25** | PA3 | - | Direction control bit 2 (digital) |
| **PB_PWM** | **Pin 9** | PH6 | Timer2 OC2B | Steering PWM (~3.9kHz Phase Correct) |

**Steering Direction Control:**
- `B1=0, B2=0` → Center (brake)
- `B1=1, B2=0` → Right
- `B1=0, B2=1` → Left

### Wiring Connections

#### Receiver Connections
```
Futaba R617FS    →    Robotdyn Mega 2560 Pro
──────────────────────────────────────────────
VCC (Red)        →    5V
GND (Black)      →    GND
CH1 (White)      →    Pin 48 (ICP5)
CH3 (White)      →    Pin 49 (ICP4)
CH5 (White)      →    Pin 2  (INT4)
CH6 (White)      →    Pin 18 (INT3)  
CH7 (White)      →    Pin 3  (INT5)
```

#### Motor Driver Connections

**Drive Motor:**
```
Motor Driver    →    Robotdyn Mega 2560 Pro
──────────────────────────────────────────────
A1              →    Pin 22 (Digital Out)
A2              →    Pin 23 (Digital Out)
PA (PWM)        →    Pin 10 (Timer2 OC2A)
VCC             →    5V (logic supply)
GND             →    GND
```

**Steering Motor:**
```
Motor Driver    →    Robotdyn Mega 2560 Pro
──────────────────────────────────────────────
B1              →    Pin 24 (Digital Out)
B2              →    Pin 25 (Digital Out)
PB_PWM          →    Pin 9 (Timer2 OC2B)
VCC             →    5V (logic supply)
GND             →    GND
```

## Channel Functions

### Analog Channels (0-255 range)
- **Steering**: Vehicle steering control, mapped from 1100-1900μs to 0-255
- **Throttle**: Main throttle control, mapped from 1100-1900μs to 255-0 (inverted)  
- **Max Throttle**: Speed limiter for onboard pedal, mapped from 1100-1900μs to 0-255

### Digital Channels (ON/OFF)
- **Reverse**: Reverse gear enable (ON = >1500μs, OFF = ≤1500μs)
- **Takeover**: Remote control override (ON = ≤1500μs, OFF = >1500μs)
  - When ON: Inhibits onboard pedal, allows only remote control
  - When OFF: Normal operation with onboard pedal active

### Motor Control Functions

#### Drive Motor
- **A1/A2**: Direction control pins (digital HIGH/LOW)
  - `A1=0, A2=0`: Brake mode (safe state)
  - `A1=1, A2=0`: Forward direction
  - `A1=0, A2=1`: Reverse direction
- **PA_PWM**: Speed control PWM at ~3.9kHz Phase Correct (Timer2)
  - 0-254 duty cycle range for speed control (driver doesn't handle 255)
  - Brake mode automatically activated when takeover is inactive or throttle is 0
- **Speed Ramping**: Smooth acceleration/deceleration using floating-point math
  - Ramp up rate: 51.0 units/sec (0 to 255 in 5 seconds)
  - Ramp down rate: 102.0 units/sec (255 to 0 in 2.5 seconds)

#### Steering Motor
- **B1/B2**: Direction control pins (digital HIGH/LOW)
  - `B1=0, B2=0`: Center/brake (no steering)
  - `B1=1, B2=0`: Right turn
  - `B1=0, B2=1`: Left turn
- **PB_PWM**: Steering PWM at ~3.9kHz Phase Correct (Timer2 OC2B)
  - Full power: 254 (99.6%) when actively turning
  - Sustain power: 13 (~5%) after 2 seconds at endstop
- **State Machine**: 5-state system for endstop protection
  - **CENTER**: Deadzone (96-160), motor off
  - **RIGHT**: Full power right turn (>160)
  - **RIGHT_SUSTAIN**: Low power at right endstop (after 2s)
  - **LEFT**: Full power left turn (<96)
  - **LEFT_SUSTAIN**: Low power at left endstop (after 2s)

## Technical Implementation

### Hybrid Capture Method
- **Input Capture** (Pins 48, 49): Hardware-precise timing with automatic edge detection
  - Zero race conditions
  - Noise canceller enabled (4-sample filter)
  - Timer4/Timer5 with 0.5μs resolution
  
- **External Interrupts** (Pins 2, 3, 18): Software timing with GPT-5 edge detection logic
  - Direct hardware state checking
  - Uses Timer1 as reference
  - Same 0.5μs resolution

### Signal Loss Detection
- **Timeout**: 100ms (configurable in `receiver.cpp`)
- **Detection**: Tracks activity on both rising and falling edges
- **Response**: Returns "N/A" for channels with no signal

## API Usage

### Initialization
```cpp
#include "receiver.h"
#include "motors.h"

void setup() {
    Serial.begin(115200);
    setup_receiver();  // Initialize PWM capture system
    setup_motors();    // Initialize motor control system
}
```

### Reading Raw Values (microseconds)
```cpp
uint16_t steering_us = get_raw_steering();    // 1100-1900μs
uint16_t throttle_us = get_raw_throttle();    // 1100-1900μs
uint16_t reverse_us = get_raw_reverse();      // PWM pulse width
uint16_t takeover_us = get_raw_takeover();    // PWM pulse width
uint16_t max_throttle_us = get_raw_max_throttle(); // 1100-1900μs
```

### Reading Processed Values
```cpp
// Analog channels (0-255)
uint8_t steering = get_steering();       // 0 = full left, 255 = full right
uint8_t throttle = get_throttle();       // 0 = min, 255 = max
uint8_t max_throttle = get_max_throttle(); // 0 = min limit, 255 = max limit

// Digital channels (true/false)
bool reverse = get_reverse();            // true = reverse gear ON
bool takeover = get_takeover();          // true = remote control active
```

### Checking Signal Status
```cpp
if (is_steering_active()) {
    // Steering signal is present and valid
    uint8_t steer_val = get_steering();
} else {
    // No steering signal (receiver off or disconnected)
}
```

### Main Loop
```cpp
void loop() {
    // Update motor outputs based on receiver data
    update_motors(get_throttle(), get_takeover());
    
    // Your application code here...
    if (is_throttle_active()) {
        uint8_t throttle = get_throttle();
        // Use throttle value...
    }
}
```

## Serial Output Format

The example `main.cpp` outputs all channels in one line:
```
Steer: 1500us/127 | Throttle: 1200us/63 | Reverse: 1400us/ON | Takeover: 1600us/OFF | MaxThrottle: 1800us/221
```

Format: `Channel: RAWus/PROCESSED`
- **Analog**: `1500us/127` (raw microseconds / 0-255 value)
- **Digital**: `1400us/ON` (raw microseconds / ON or OFF)
- **Missing**: `N/A` (signal timeout)

## Timing Specifications

- **PWM Frequency**: ~62.3Hz (16ms period)
- **Pulse Width Range**: 1000-2000μs (typical RC standard)
- **Analog Mapping**: 1100-1900μs → 0-255 (dead band on edges)
- **Digital Threshold**: 1500μs (≤1500 = ON, >1500 = OFF)
- **Resolution**: 0.5μs (2MHz timer clock)
- **Timeout**: 100ms (6+ missed pulses)

## File Structure

```
src/
├── main.cpp          # Application code and example usage
├── receiver.h        # PWM receiver API interface
├── receiver.cpp      # PWM capture implementation
├── motors.h          # Motor control API interface
└── motors.cpp        # H-bridge motor control implementation
```

## Performance Notes

- **Input Capture channels** (Steering, Throttle) have the highest precision
- **External Interrupt channels** have excellent precision with minimal overhead
- All channels support signal loss detection
- System handles concurrent PWM signals on all 5 channels
- No floating-point math used - all integer operations for speed

## Troubleshooting

### No Signal Detected
1. Check wiring connections
2. Verify receiver power (5V, GND)
3. Ensure transmitter is bound and powered on
4. Confirm correct channel mapping

### Incorrect Values
1. Calibrate transmitter endpoints (1100-1900μs range)
2. Check signal polarity for digital channels
3. Verify PWM frequency (~50Hz standard)

### Signal Dropout
1. Check for interference (WiFi, other 2.4GHz devices)
2. Verify adequate power supply to receiver
3. Check antenna orientation and range

---

**Project**: Mosterrak Futaba PWM Interface  
**Hardware**: Futaba R617FS + Robotdyn Mega 2560 Pro  
**Author**: Advanced PWM Capture System  
**Version**: 1.0
