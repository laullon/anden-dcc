# Bill of materials

**Revision A — quantities for the proposed two-track protected assembly.**

The Pico/analyzer stage is already measured. The power stage is not built or
validated, and firmware 0.4.0 does not drive its enable/arm/ADC pins. Treat this
as a planning BOM; confirm the actual shield and component availability before
ordering the protection parts. Prices and supplier stock are intentionally not assumed.

| Reference | Qty | Part / specification | Status | Notes |
| --- | ---: | --- | --- | --- |
| M1 | 1 | Raspberry Pi Pico W | Existing | Original RP2040 Pico W; headers if needed |
| M2 | 1 | Deek-Robot L298P motor shield | Ordered | Verify delivered clone, sense gain and CD4077 marking |
| M3 | 1 | USB-C PD trigger, DIP selectable | Ordered | 15 V profile, at least 3 A; verify supported profile and polarity |
| M4 | 1 | MP1584 adjustable buck module | Existing | Set 5.00 V before connection; logic supply only |
| PS1 | 1 | USB-C PD charger | Check availability | Explicit 15 V / 3 A output; 45 W or more alone is insufficient |
| CAB1 | 1 | USB-C to USB-C cable | Check availability | 3 A rated; appropriate charger/cable negotiation |
| U1 | 1 | SN74AHCT125N, PDIP-14 | To source | TTL-compatible 5 V buffer; not plain HC125 |
| U2 | 1 | SN74HCT74N, PDIP-14 | To source | Dual D flip-flop; asynchronous clear; unused inputs tied |
| U3 | 1 | LM393 dual comparator, PDIP-8 | To source | Open collector; verify selected manufacturer pinout |
| U4 | 1 | MCP120-485DI/TO | To source | 4.725 V nominal supervisor; open drain; verify D bondout |
| Q1 | 1 | 2N3904 NPN transistor | To source | Check manufacturer E/B/C pinout |
| D1 | 1 | 1N5819 Schottky diode | To source | Through-hole 1 A / 40 V; anode +5V, cathode Pico VSYS |
| R1–R16 | 16 | 10 kΩ, 1%, ≥0.125 W | To source | 4 input down, 2 EN down, 1 CLK down, 1 base series, 6 divider, 1 VREF bottom, 1 OE up |
| R17 | 1 | 4.7 kΩ, 1%, ≥0.125 W | To source | CLEAR_N pull-up to 5 V |
| R18 | 1 | 100 kΩ, 1%, ≥0.125 W | To source | Q1 base to emitter pull-down |
| R19 | 1 | 40.2 kΩ, 1%, ≥0.125 W | To source | VREF top resistor; nominal 1.21 A trip with reference sense gain |
| C1 | 1 | 100 µF / 25 V electrolytic | To source | Across shield motor VIN/GND, observe polarity |
| C2 | 1 | 47 µF / ≥10 V electrolytic | To source | 5 V logic bulk decoupling |
| C3–C9 | 7 | 100 nF / ≥25 V ceramic | To source | 4 IC supplies, shield logic, VSYS, VREF |
| C10–C12 | 3 | 1 nF / ≥25 V ceramic | To source | Three divided ADC nodes |
| F1 | 1 | 3 A DC fuse + holder | To source | DC voltage rating ≥32 V; wiring protection, not the fast track trip |
| S1 | 1 | Emergency stop, 1 NC + 1 NO linked contacts | To source | NC power contact ≥30 V DC / 3 A; verify explicit DC rating |
| PCB1 | 1 | Soldered prototyping board + IC sockets | To source | Logic assembly only; socket 2×14-pin, 1×8-pin; U4 per package |
| WIRE1 | 1 | Hookup wire assortment | To source | Power paths about 0.5 mm² for short runs; logic about 0.2 mm² |
| COOL1 | 1 | L298 heatsink and enclosure airflow provision | To source | Measure temperature; fit 5 V fan if required |
| TP1–TP8 | 8 | Test points / header loops | Optional | PD, 5 V, VSYS, GP2, SNSA, SNSB, CLEAR_N, BUFFER_DISABLE |
| LA1 | 1 | 8-channel logic analyzer + leads | Existing | This unit verified with Logic 2 2.4.48 MCP |
| DMM1 | 1 | Digital multimeter | Check availability | Needed for regulator, polarity, dividers and continuity checks |

## Resistor allocation

R1–R4: U1 inputs to ground. R5/R6: shield ENA/ENB to ground.
R7: U2 ARM clock to ground. R8: GP5 to Q1 base.
R9/R10: SNSA divider. R11/R12: SNSB divider. R13/R14: latch feedback divider.
R15: VREF to ground. R16: U1 OE pull-up. R17: CLEAR_N pull-up.
R18: Q1 base to emitter. R19: 5 V to VREF.

A generic "2 A" or "3 A" product title is not a continuous current guarantee.
The 5 V buck powers logic; it does not supply train current. The main fuse and
USB source do not replace the proposed per-track hardware trip.

See [connections.csv](connections.csv) and [the wiring notes](README.md).
