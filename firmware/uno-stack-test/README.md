# Uno DCC controller

**Uno 0.6.1**, paired with ESP13 0.6.5. Supported MCU: **ATmega328P, 16 MHz,
5 V Arduino Uno R3**. It generates DCC, routes bridge A, checks the UART protocol
and owns the output timeouts. Speed stays zero. Bridge B remains disabled.

[Wiring and BOM](../../docs/hardware/README.md) ·
[First locomotive / protocol](FIRST-LOCO-TEST.md) ·
[15 V no-load power check](POWER-TEST.md) · [DCC scope check](DCC-SCOPE.md)

## Build and USB upload

```sh
arduino-cli core install arduino:avr@1.8.7
tools/build-uno-stack.sh
arduino-cli upload --fqbn arduino:avr:uno --port UNO_PORT \
  --input-file build/uno-stack-test/uno-stack-test.ino.hex
```

Before uploading: motor VIN and track disconnected, Uno USB connected, both
ESP13 UART switches OFF. Afterward turn both switches ON for normal communication.
The ESP OTA updater cannot update the Uno.

## Signals and output behavior

D10 / OC1B is the Timer1 hardware reference. D12 mirrors it at a fixed phase for
the shield's direction A input during DCC. D3 enables A at a packet boundary
for DCC and immediately for a DC pulse. D9 brake A
stays LOW. D11 enable B and D8 brake B stay LOW; D13 direction B also drives the
Uno L LED test and can blink only when both outputs are disabled. A0/A1 provide
raw shield current readings; cutoff is uncalibrated.

All five function groups covering F0–F28 and zero-speed/direction packets refresh
for up to eight short addresses. Turning track power on starts with 32 broadcast
stop packets. Double-buffered
snapshots publish at complete packet boundaries. Packet bytes are prepared in
the main loop; time-critical interrupt code selects prebuilt bytes.

Continuous track operation requires an explicit start and checked KEEP renewals. The independent
two-second Uno lease expires locally; late KEEP never restarts it. Current/timing
faults and an interrupt backup disable A. Three-second no-load DC/DCC scope tests
remain available under Setup. None of these checks establishes loaded protection.

## UART

115200 baud, checked ASCII `@BODY*CRC16\n`, maximum 144-character body,
CRC-16/CCITT-FALSE over the body with initial value 0xFFFF. ESP boot noise and corrupt/partial frames are ignored.
Commands carry a sequence number; replies acknowledge the matching request.

- `CONTROLX` supplies the complete 29-bit function-mask roster and directions.
- `TRACKON` explicitly starts output; `KEEP` renews existing track output only.
- `BENCHSTOP` disables it immediately.
- `FIND` selects address-search packets without a large SRAM bitmap.
- `PING`, `STATUS`, `LED`, `PULSE`, `DCCTEST` provide diagnostics.

Older ROSTER/CONTROL messages remain accepted for compatibility. The eighteenth
STATE field is capability `2` for F0–F28. Details and measured limits are in the
[first-locomotive guide](FIRST-LOCO-TEST.md) and [validation record](../../docs/VALIDATION.md).
