# Uno + motor shield + ESP13 wiring

The supported assembly uses the three boards' normal Uno headers:
**Uno R3 / ATmega328P 16 MHz, Deek-Robot L298P motor shield and DOIT ESP13**.
The motor shield must use the Rev3-style pinout below. Check clone markings.

The [cab board wiring and power plan](../../firmware/uno-stack-test/CAB.md)
uses one USB-C PD input and an MP1584 for the 5 V logic rail.

![Cab board controls and power connections](cab-schematic.svg)

The original USB-powered bench arrangement is shown below.

![USB-powered bench stack connection diagram](schematic.svg)

## Power

- On the cab board, PD positive feeds motor shield VIN and MP1584 IN+;
  PD negative connects to shield GND and MP1584 IN−. Set and measure
  MP1584 OUT+/OUT− at **5.0 V** before
  connecting the stack or OLED.
- MP1584 OUT+ feeds the ESP13 shield's Arduino-side **5V** header and OLED VCC;
  OUT− feeds their GND. Confirm continuity to the Uno 5V/GND headers first.
  Use separate labelled **15 V MOTOR** and **5 V LOGIC** connectors.
- All grounds are common. The ESP13 shield regulates its own logic supply.
  The original bench arrangement uses Uno USB for 5 V instead of the MP1584.
- A compatible USB-C PD charger/trigger supplies **verified 15 V DC** to the motor
  shield VIN/GND screw terminals. Measure voltage and polarity before connection.
- **Open VIN CONNECT on the motor shield.** With all power removed, verify its
  VIN screw terminal is isolated from the Uno VIN header; GND remains common.
- Leave CD4077 installed. Keep the shield's logic at 5 V; no external shifter is
  needed with the Uno's 5 V outputs.
- Feed A+ and A− to the two tracks in parallel. Neither rail is GND. Leave B+/B−
  unconnected. Both locos share the signal and respond to their decoder addresses.

The cab power plan still needs physical validation. Existing bench measurements
used separate USB logic power and PD motor power. Never feed 15 V into the 5 V
header. The regulated 5 V feed bypasses the Uno's onboard regulator.

Before connecting Uno USB, unplug the PD charger and disconnect the cab's
**5 V LOGIC** feed. Disconnect Uno USB before restoring cab power. The complete
setup and upload sequence is in the [cab guide](../../firmware/uno-stack-test/CAB.md).

## Header signals

| Uno pin | Connected function |
| --- | --- |
| D0 / RX | ESP13 TX through shield switches and level interface |
| D1 / TX | ESP13 RX through shield switches and level interface |
| D3 | Motor enable A |
| D12 | Motor direction A / routed DCC |
| D9 | Brake A, held LOW |
| A0 | Motor shield current-sense A, raw uncalibrated ADC |
| D11 | Motor enable B, held LOW |
| D13 | Motor direction B / Uno L LED; blink only with outputs off |
| D8 | Brake B, held LOW |
| A1 | Current-sense B, telemetry only |
| D10 | DCC hardware reference for analyzer; not a rail connection |

The optional [encoder cab](../../firmware/uno-stack-test/CAB.md) uses D4–D6 for the encoder, D2 for STOP, D7 for the movement LED and A4/A5 for the OLED. See the [cab wiring schematic](cab-schematic.svg) for those connections. The main stack connections above still apply.

## Upload and first test

Disconnect cab power, including its 5 V logic feed, and the track before either serial upload. Both ESP13 blue
switches OFF isolate its UART for Uno USB upload; ON connects Uno ↔ ESP for
normal use. Do not connect a separate USB-UART transmitter while the stack UART
is enabled. A raw ESP UART adapter must use **3.3 V logic**, not Uno logic levels.

First verify USB-only Uno ping and LED, then use the
[power test](../../firmware/uno-stack-test/POWER-TEST.md),
[DCC scope guide](../../firmware/uno-stack-test/DCC-SCOPE.md) and
[first-locomotive guide](../../firmware/uno-stack-test/FIRST-LOCO-TEST.md).
The output timeout and uncalibrated current cutoff do not replace loaded fault
and thermal validation.

[BOM](BOM.md) · [CSV BOM](bom.csv) · [Connections](connections.csv) ·
[Print diagram](print.html)
