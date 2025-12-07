## Product notes

From: https://aliexpress.com/item/1005005925917904.html

**100A DC motor drive Module High Power motor governor speed Control Dual H-bridge power supply 12V~48V 36V 24V optocoupler isolation**

This module uses a 10M high-speed optocoupler to isolate the input signal, effectively preventing the interference signal of the drive board from interfering with the control board, making the system more stable and reliable.

This module can be used to drive general DC motors with a maximum current of up to 100A . It is unmatched by general integrated motor drives. Therefore, it is very suitable for robot competitions, chariot competitions, and Freescale competitions.


Of course, this motor drive module is also very suitable for novices who are learning motor control, because its control method is very simple, see the following description for details

Features of drive module:

1. The basic components of the drive module body are patches, high integration, well-designed board layout, very beautiful, small size, onboard two high-power DC motor drive, the size of the drive module is only 80mm*70mm;
2. The large heat sink can effectively dissipate heat for the drive module when the current is large, and maintain the stable performance of the module;
3. When driving the motor, the maximum rated current of the module can reach 100A and the on-resistance is only 0.0015 ohm;
4. The switching frequency is high, up to 60KHZ, which effectively avoids the unpleasantness caused by the low frequency of the commissioning motor;
5. The control interface is very simple:
   - When A1.A2=0.0, it is brake;
   - When A1.A2=1.0, it is forward rotation;
   - A1.A2=0.1 is reverse;
   - PA is PWM wave input (motor speed adjustment);
   - G is the common ground pin with the control board (B road is the same control);
6. Both 3.3V and 5V microcontrollers can control this module, and only one motor power supply (12V~48V)

Driver board wiring diagram

![Driver board wiring diagram](driver_chinoso.webp)

## Notes from different seller

https://aliexpress.com/item/1005003370023227.html

Please read the description before order!


this product can not connect RC Receiver( remote control)!  The PWM signal requirement of our driver is that the frequency is above 10K, and the duty cycle is 0-98% (the PWM frequency of the general aeromodelling controller is below 1K)


Because different customers have different use conditions, we usually recommend that the 100A drive board use a single channel with power below 500W (continuous). If it exceeds 500W, the heat dissipation capacity of the drive plate may not keep up with that of the drive plate. Generally, the greater the current, the greater the heat on the main circuit, and the higher the temperature rise. At this time, the temperature exceeds the solder melting point on the drive plate, which is easy to burn.

On the other hand, the heating of the drive plate is related to the load and the frequency of the drive signal. The switch tube operates in the hard switching mode, and the higher the frequency, the easier the heating. At the same time, if the load is reactive (capacitive or inductive load), the higher the temperature (switch loss) will be. These are all need to be carefully considered when selecting the drive plate. It is better to leave enough margin, instead of selecting the 100A drive plate for the 100A load.


other note:

The reverse electromotive force of the inductive load. If the current is large, the load will impact the switch tube of the drive board. Although there is TVS tube protection, if the load of the TVS tube is exceeded, the TVS tube will also be broken down (failure) and finally the switch tube will be broken down (at this time, the drive board will be burned!) These are related to the current size and load If you mind, please do not buy.

## My notes:

- The driver hates 100% PWM duty cycle, it will drop to ~0 volts output. So I limit the max speed to 254.
- I think that limit should be even lower at higher PWM frequencies, ej ~31.4 KHz (16 MHz / 510). So I prescaled the PWM to 8 for 3.92 KHz.
- A1=A2=PA=1 is hi-Z mode.