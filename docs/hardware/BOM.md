# Bill of materials

| Qty | Item | Notes |
| --- | --- | --- |
| 1 | Arduino Uno R3-compatible, ATmega328P, 16 MHz, 5 V | USB cable for its connector |
| 1 | Deek-Robot L298P motor shield, Rev3-style pinout | CD4077 installed; VIN CONNECT opened and checked |
| 1 | DOIT ESP13 / ESP-WROOM-02 Uno Wi-Fi shield | Soldered stacking headers, both UART switches |
| 1, bench / upload | Uno USB cable | Disconnect cab external 5 V feed before connecting |
| 1, cab board | MP1584 step-down module | 15 V input; adjust and verify 5.0 V output for stack and OLED |
| 1, cab board | Mechanical rotary encoder with push button | A/B on D4/D5; select on D6; contacts to GND |
| 1, cab board | Normally open momentary STOP button | D2 to GND |
| 1, optional | LED and 470 Ω series resistor | D7 through resistor to anode; cathode to GND |
| 1, optional | 0.96-inch 128×64 SSD1315 I²C OLED breakout | Address 0x3C; 5 V compatible supply and I²C |
| As needed | Cab board and connectors | Separate labelled 15 V MOTOR and 5 V LOGIC outputs; common GND |
| 1 | USB-C PD charger, compatible cable and voltage trigger | Must offer the selected 15 V profile; verify output |
| As needed | Track wire and terminal connections | A+/A− to both tracks in parallel; B unused |
| 1, setup only | USB-UART adapter with 3.3 V logic | First ESP flash / recovery; not required after OTA setup |
| 1, testing | Multimeter | Check polarity, voltage and VIN isolation |

The PD trigger and MP1584 mount on the cab board alongside the controls.
The cab supply plan needs physical validation, including the 5 V rail during
Wi-Fi activity. Board clones vary. Verify pinout and actual supply behavior.
The bridge's practical current/thermal limit is not established by this project;
choose power protection for the actual wiring and test load. No external logic
buffer or extra controller is part of this stack.
