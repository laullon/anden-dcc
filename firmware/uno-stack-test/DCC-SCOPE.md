# Timed DCC scope test on bridge A

ESP / Uno **0.4.0** adds **DCC · 3 s** under **Setup → Motor shield bench test**.
This is a no-load waveform check. Keep trains, rails and other loads disconnected.
The existing A + / A − buttons remain three-second DC polarity tests.

## Connections and capture

Use the [verified 15 V supply / VIN isolation setup](POWER-TEST.md). D12 reaches
the shield through the normal stacking header: **no jumper is needed**. Do not
connect D10 to D12; both are outputs. D10 remains a 5 V hardware-timed reference.

Use an appropriate scope measurement setup for the bridge output. Neither A+
nor A− is logic ground; avoid grounding a driven output through a scope, USB
connection or a second probe. The generic USB logic analyzer is not a rail probe.

1. Start with both output enables LOW and the synchronized roster visible.
2. Set a timebase around **100 µs/div** and a voltage range suitable for the
   differential swing (about **5 V/div**, if the probe attenuation is matched).
3. Arm the scope, then press **DCC · 3 s**. Press again only after it finishes.
   **Stop test** cancels it earlier.
4. Inspect steady pulses in the middle of the burst. The final packet can be
   truncated by the bounded stop; it is not a continuous track mode.

| Measurement | Expected observation with 15 V motor VIN, no load |
| --- | --- |
| Across A+ / A−, suitable differential/floating measurement | Roughly +15 / −15 V, about 30 V peak-to-peak, less bridge losses |
| One output relative to shield GND | Roughly 0 / 15 V on that bridge leg |
| A “1” bit | Two nominal 58 µs halves, total 116 µs |
| A “0” bit | Two nominal 100 µs halves, total 200 µs |

The waveform carries the current short-address F0 roster in round-robin order,
or idle packets when empty. It does not send speed/startup-stop commands and
is not ready for locomotive use. F1–F4 are OFF in these function-group packets.

## Routing and cutoff

Timer1 / OC1B still toggles **D10** directly in hardware. For this test, the ISR
copies its level to **D12** at a fixed timer phase: a 40-tick / 20 µs target after
each compare, plus the short instruction path that writes the pin. This fixed
offset preserves nominal half-bit widths while absorbing normal interrupt-entry
variation. D12 is updated before variable packet bookkeeping.

If the sampled phase is more than two ticks / 1 µs late, the firmware disables A
and stops the timer, incrementing the timing-fault counter. This is an experimental
implementation checked in a timer model and on the Uno; **actual bridge timing
and waveform shape require a scope measurement**. No standards compliance is
inferred from the model or a zero fault counter.

A begins at a full packet boundary with a complete preamble. Its deadline is
3,000 ms from the request, so the actual enabled time can be a few milliseconds
shorter while it waits for that boundary. The raw A0 cutoff, manual Stop,
interrupt backup timeout and default-off behavior from the DC test still apply.
B stays off. All tests reject attempts to reverse, replace or extend an active
burst. Roster/LED controls pause while a bridge output is enabled.

On completion or Stop, the route is disconnected and D12 is LOW again. A timing
fault requires an Uno restart; remove motor VIN before resetting/uploading.
Current sensing remains uncalibrated; loaded protection, hardware failure
behavior and continuous train control are not validated by this no-load test.

## Protocol and diagnostics

- New command: `AD1,id,DCCTEST` (fixed three-second duration).
- `BENCHSTOP` stops either DC or DCC.
- The state response adds a seventeenth field, `benchMode`: 0 = DC reverse,
  1 = DC forward, 2 = DCC. The existing bench state and remaining time remain.
- The ESP accepts older 8/14/16-field replies, but DCC scope controls require
  the 17-field capability and a fresh, synchronized roster.
- `uno.bench.dccSupported`, `mode` and `active` identify the test. Top-level and
  `dcc.trackPower` are true only for an enabled DCC burst. They report the command
  state, not a measurement of motor VIN or output voltage.

## Validation

- Uno 0.4.0: 8,212 bytes flash / 591 bytes static RAM. Direct-port routing is
  restricted at compile time to the 16 MHz ATmega328P Uno.
- ESP 0.4.0: 394,768-byte image / 30,972 bytes static RAM.
- The actual ISR runs in a host register/timer simulation. Checks cover packet-
  boundary enable, 58/100 µs duration preservation with varied ISR entry delays,
  DC isolation, Stop, the interrupt backup cutoff, timer wrap and late-phase
  fail-off. The model does not emulate analog switching or AVR instruction times.
- With motor VIN disconnected, checked USB telemetry observed DCC arm and
  automatic off after 3.001 seconds, zero phase/deadline faults under sustained
  UART traffic, rejected replacement/extension, Stop and the existing DC mode.
- Browser checks cover capability/readiness guards, DCC mode display, Stop,
  roster locks and both themes/mobile layouts.

On 2026-11-03, with 15 V motor VIN and no load, the user confirmed the bridge-A
DCC waveform looked correct on the scope. Checked telemetry reported DCC mode,
normal completion, both enables LOW and zero timing faults afterward. This was
a visual check: measured pulse widths, rise/fall times and DCC voltage levels
were not recorded, so quantitative timing verification remains open.

If the scope shows steady +15 V, check that **DCC · 3 s** was selected rather
than **A + · 3 s**. After an OTA update, reload an already-open panel to get the
new controls.

References: [NMRA electrical standard, bit timing and differential signal](https://www.nmra.org/sites/default/files/standards/sandrp/DCC/S/s-9.1_electrical_standards_for_digital_command_control.pdf),
[DCC-EX standard shield pin mapping](https://github.com/DCC-EX/CommandStation-EX/blob/master/MotorDrivers.h),
[Arduino shield schematic](https://www.arduino.cc/en/uploads/Main/arduino_MotorShield_Rev3-schematic.pdf).
