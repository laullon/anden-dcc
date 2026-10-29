# Hardware sources

Primary sources checked on 29 October 2026. The simple circuit is a proposed
integration of these parts, not a manufacturer-approved or electrically
validated assembly. Rev C uses only one AHCT buffer and bridge A.

| Source | Used for |
| --- | --- |
| [Raspberry Pi Pico W datasheet](https://datasheets.raspberrypi.com/picow/pico-w-datasheet.pdf) | Pinout, 3.3 V GPIO, VSYS supply limits and external Schottky power OR |
| [Arduino Motor Shield reference schematic](https://www.arduino.cc/en/uploads/Main/arduino_MotorShield_Rev3-schematic.pdf) | Direction/enable/brake mapping, 4077 logic and sense circuit; clone must be checked |
| [Arduino Motor Shield](https://docs.arduino.cc/hardware/motor-shield-rev3) | Shield architecture and hardware resources |
| [DCC-EX Deek-Robot notes](https://dcc-ex.com/reference/hardware/motorboards/deek-robot-motor-shield.html) | Clone relationship and practical current/cooling limits |
| [TI CD4077B](https://www.ti.com/lit/ds/symlink/cd4077b.pdf) | Minimum 3.5 V high input at a 5 V supply |
| [TI SN74AHCT125](https://www.ti.com/lit/ds/symlink/sn74ahct125.pdf) | TTL-compatible input, 5 V buffer and active-low output enable |
| [ST L298](https://www.st.com/resource/en/datasheet/l298.pdf) | Supply/input ratings, bridge voltage loss and current limitations |
| [MPS MP1584](https://www.monolithicpower.com/en/products/power-management/switching-converters-controllers/step-down-buck/converters/mp1584.html) | Existing buck module's IC operating range; module thermal capability remains unverified |

The adjustable USB PD trigger is a generic module. Its actual behavior,
current rating, pin polarity and response to an unsupported profile must be
verified from the delivered board. A datasheet for a different USB PD module
cannot establish its rating.
