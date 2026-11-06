# Bill of materials

| Qty | Item | Notes |
| --- | --- | --- |
| 1 | Arduino Uno R3-compatible, ATmega328P, 16 MHz, 5 V | USB cable for its connector |
| 1 | Deek-Robot L298P motor shield, Rev3-style pinout | CD4077 installed; VIN CONNECT opened and checked |
| 1 | DOIT ESP13 / ESP-WROOM-02 Uno Wi-Fi shield | Soldered stacking headers, both UART switches |
| 1 | USB logic supply/cable | Powers Uno and shield logic; adequate for Wi-Fi bursts |
| 1 | USB-C PD charger, compatible cable and voltage trigger | Must offer the selected 15 V profile; verify output |
| As needed | Track wire and terminal connections | A+/A− to both tracks in parallel; B unused |
| 1, setup only | USB-UART adapter with 3.3 V logic | First ESP flash / recovery; not required after OTA setup |
| 1, testing | Multimeter | Check polarity, voltage and VIN isolation |

Board clones vary. Verify pinout, available USB power and actual supply behavior.
The bridge's practical current/thermal limit is not established by this project;
choose power protection for the actual wiring and test load. No external logic
buffer or extra controller is part of this stack.
