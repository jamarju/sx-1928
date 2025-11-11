# SX-1928 mod

Control board replacement for the SX-1928 ride-on car, aka "Clash" aka "Challenger" based on a **Futaba R617FS** and a **Robotdyn Mega 2560 Pro** Arduino board.

# Hardware Setup

- Receiver: Futaba R617FS (7-channel PWM output)
- Arduino: Robotdyn Mega 2560 Pro
- 100A DC motor drive module: https://aliexpress.com/item/1005005925917904.html
- 1x 24V 40A 4-pin automotive relay: https://www.amazon.es/dp/B0CJHQN6TY
- 1x 24V 40A 5-pin automotive relay: https://www.amazon.es/dp/B07MF5XJP9

Alternatively, 2x 24V 40A 5-pin relays can be used instead of 1x each.

# Pin Mapping

## Receiver Input Pins

| Receiver Channel | Function      | Arduino Pin | Method           | Signal Type | Range/Logic |
|------------------|---------------|-------------|------------------|-------------|-------------|
| **CH1**          | Steering      | **Pin 48**  | Input Capture    | Analog      | 1100-1900μs → 0-255 |
| **CH3**          | Throttle      | **Pin 49**  | Input Capture    | Analog      | 1100-1900μs → 255-0 (inverted) |
| **CH5**          | Reverse       | **Pin 2**   | External Int     | Digital     | >1500μs = ON, ≤1500μs = OFF |
| **CH6**          | Max Throttle  | **Pin 18**  | External Int     | Analog      | 1100-1900μs → 0-255 |
| **CH7**          | Takeover      | **Pin 3**   | External Int     | Digital     | ≤1500μs = ON, >1500μs = OFF |

## Motor Control Output Pins

### Drive Motor

| Function | Arduino Pin | AVR Pin | Timer/PWM | Notes |
|----------|-------------|---------|-----------|-------|
| **A1** | **Pin 22** | PA0 | - | Direction control bit 1 (digital) |
| **A2** | **Pin 23** | PA1 | - | Direction control bit 2 (digital) |
| **PA_PWM** | **Pin 10** | PB4 | Timer2 OC2A | Motor speed PWM (~3.9kHz Phase Correct) |

## Steering Motor

| Function | Arduino Pin | AVR Pin | Timer/PWM | Notes |
|----------|-------------|---------|-----------|-------|
| **B1** | **Pin 24** | PA2 | - | Direction control bit 1 (digital) |
| **B2** | **Pin 25** | PA3 | - | Direction control bit 2 (digital) |
| **PB_PWM** | **Pin 9** | PH6 | Timer2 OC2B | Steering PWM (~3.9kHz Phase Correct) |

# TX configuration

This is the relevant configuration for the Futaba T7C transmitter.

- Hold `Mode/Page` 
  - `E.POINT`
    - Select `CH1`. Move the right stick left and right to set the steering end points between 1100 and 1900μs. Eg. left: 127%, right: 116%.
    - Select `CH3`. Move the left stick up and down to set the throttle end points between 1100 and 1900μs. Eg. up: 128%, down: 117%.
  - `REVERSE`: set all channels to normal `NOR`.
  - `TH-CUT`: you can assign one of the switches to cut the throttle to 0. Or just disable it by seeting it to `INH`.
  - `FAIL SAFE`: make throttle (CH3) go to 0 when the signal is lost: dial up to set `F/S`, then pull throttle stick down to 0, then push dial 1 sec to save the stick position as the failsafe position. Verify that it works by shutting down the transmitter and checking that the throttle goes to 0.

# Safety features

- **Power relay**: the whole car is powered by a 24V 40A 5-pin automotive relay. Turning the switch off will cut power to the car entirely.
- **Brake on power off**: another relay brakes the car by shorting the motor terminals together when the power is turned off. Note the car does not have mechanical brakes. This is a a simple electrical brake that works in relatively flat terrain.
- **Acceleration**: motors ramp up linearly to full speed in 5.0 seconds, and ramp down to 0 speed in 1.0 second to prevent passengers from being thrown forward/backward. Actual ramp rates are definable in `RAMP_UP_RATE` and `RAMP_DN_RATE`.
- **Reversing while moving**: if you accidentally or intentionally reverse while the car is moving, the car will first slow down to a full stop, then speed up in the opposite direction following the above acceleration profile.
- **Kid control disabled by default**: car starts in RC (takeover) mode if until the TX is powered on and the arming sequence is completed.
- **Arming procedure**: the car won't move until the arming sequence is completed:
  - Turn switch B up to take over (RC mode).
  - Pull throttle stick down to 0.
  - Arming is completed, now you can control the car with the transmitter or switch to kid control mode turning switch B down.
- **Signal loss**: the car stops if the receiver signal is lost. Note fail-safe mode must be configured in the transmitter! See instructions above.
- **Steering dead zone**: the steering stick has a dead zone around the center position to prevent motor movement when the stick is in the center position.
- **Steering hold**: the steering motor operates at a speed proportional to the stick position, i.e., the car turns faster the more you move the stick. However, since the steering motor lacks endstop switches or position feedback, the motor switches to "hold" mode after 2 seconds to prevent overheating and mechanical stress. In hold mode, the motor uses only 5% PWM power to maintain position without generating excessive heat.