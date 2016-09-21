# MiniSHARC-Arduino
A library for interfacing an Arduino with the MiniDSP MiniSHARC DSP. It has the following features:

* Read current DSP volume
* Set DSP volume
* Increase/decrease volume in .5dB steps (or multiples thereof)
* Mute (this doesn't work because it relies on specific IR codes. We don't yet know the real I2C codes for mute)
* Read current DSP configuration (1-4)
* Switch to DSP configuration 1-4

See MiniSHARC-Arduino.ino for an example of how to use this project.

## Hooking it up
Use 3 of the 5 pins on the MiniSHARC's VOL-FP header:

* Pin 1: I2C SDA
* Pin 2: I2C SCL
* Pin 3: No connection
* Pin 4: No connection
* Pin 5: GND

Connect them to the corresponding pins on an Arduino. It should just work!
