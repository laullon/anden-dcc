# Simple bill of materials

**Rev C proposal.** The four modules stay as chosen. The added logic is one
SN74AHCT125N, five resistors and six capacitors; no programming, latch or comparator
circuitry. Electrical parts match the [single-sheet schematic](schematic.pdf).
Firmware does not enable track power yet.

| Reference | Qty | Part | Status | Notes |
| --- | ---: | --- | --- | --- |
| M1 | 1 | Raspberry Pi Pico W | Existing | Original Pico W; USB remains available |
| M2 | 1 | Deek-Robot L298P shield | Ordered | Only A used; B disabled; check delivered clone |
| M3 | 1 | USB-C PD trigger | Ordered | Set 15 V; charger must offer 15 V / 3 A |
| M4 | 1 | MP1584 buck module | Existing | Set 5.00 V before connecting |
| U1 | 1 | SN74AHCT125N / PDIP-14 | To source | AHCT, not HC; the only added logic IC |
| D1 | 1 | 1N5819 Schottky / 40 V | To source | Anode +5V; cathode/band Pico VSYS |
| R1-R5 | 5 | 10k / 1% / >=0.125 W | To source | R1/R2 input down; R3 enable down; R4/R5 sense divider |
| C1 | 1 | 100 uF / 25 V electrolytic | To source | Motor VIN bulk; observe polarity |
| C2 | 1 | 47 uF / >=10 V electrolytic | To source | 5 V logic bulk |
| C3-C5 | 3 | 100 nF / >=25 V ceramic | To source | At U1, shield 5V and Pico VSYS |
| C6 | 1 | 1 nF / >=25 V ceramic | To source | ADC node to GND |
| F1 | 1 | 3 A DC fuse and holder / >=32 V | To source | Supply wiring protection; not fast decoder cutoff |
| S1 | 1 | SPST track-power switch / >=30 V DC 3 A | To source | Physical cutoff of motor supply |
| J1-J2 | 2 | Two-pole track connectors | As needed | Both pairs wired in parallel to output A |
| PS1 | 1 | USB-C PD charger with 15 V / 3 A | Check existing | Wattage alone does not establish available profiles |
| CAB1 | 1 | USB-C cable / 3 A | Check existing | Compatible with charger and trigger |
| PCB1 | 1 | Small perfboard + 14-pin IC socket | As needed | One IC; no latch/comparator board |
| WIRE1 | 1 | Short hookup wire assortment | As needed | Power about 0.5 mm²; logic about 0.2 mm² |
| COOL1 | 1 | L298 cooling / airflow | After measurement | Establish usable continuous load on the actual board |

Use the [connections list](connections.csv) for exact pins. The two locos share
the current capacity of bridge A. Firmware current cutoff and thermal behavior
still need implementation/calibration and bench validation.
