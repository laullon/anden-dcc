# Validation

The supported stack is Uno R3 / ATmega328P at 16 MHz, Deek-Robot L298P motor
shield and DOIT ESP13. Tested cores: AVR 1.8.7 and ESP8266 3.1.2.

## Software checks

- Build both boards with `tools/build.sh`; generated ESP assets are checked.
- Native tests cover checked UART frames, corruption/noise, maximum masks,
  F0–F28 packet groups/XOR, zero speed, independent loco state, atomic publication,
  address search, favorite records, LED feedback and browser/output leases.
- Timer1 register modeling checks D12 phase, delayed interrupt entry,
  packet-boundary enable, default-off, backup timeout and late-edge fail-off.
  Modeling does not measure execution time on the AVR.
- Browser checks cover power transitions, scroll-visible power controls, held
  mouse targets during polling, function favorites/drafts, direction, finder,
  themes, setup/OTA, offline behavior and 320 px layouts.

## Build snapshot at `e138437`

The comment update regenerated the embedded web page. Both targets built and
all eight browser suites passed. ESP13 0.6.5 built to 411,776 bytes with 34,204
bytes static RAM. Uno 0.6.1 used 12,588 bytes flash and 1,018 bytes static RAM.
For later builds, check the ESP manifest and the Uno compiler output.

## Full review · ESP13 0.6.5 / Uno 0.6.1, 6 November 2026

Both firmware targets compile and all eight browser suites pass. New regressions
exercise the actual Uno command parser, scheduler/ISR, streaming image validator,
HTTP update lifecycle and filesystem initialization policy. Buffer-bound and
filesystem tests also run with AddressSanitizer/UndefinedBehaviorSanitizer.
Tests reproduce the original buffer overflow, signature collision, command
readiness mutation, finder clock jumps and corrupt-image upload acceptance.

Uno: 12,588 bytes flash / 1,018 bytes static RAM. ESP: 411,360-byte image /
34,204 bytes static RAM. The validated image remains below the 448 KiB OTA limit.
The review does not install firmware or perform new loaded or timing measurements.
See [confirmed findings and regression mapping](CODE-REVIEW.md).

## Uniform function controls 0.6.4, 6 November 2026

All eight browser suites pass with a permanent single F0 toggle and the same
control for F1–F28, including power-off presets. Live ESP 0.6.4 checks toggled F0
and F28 both ways while A/B stayed disabled, restored the original function states,
and preserved loco/favorite metadata. No power-on request was sent; no JavaScript
or timing faults were observed. Uno 0.6.0 is unchanged. ESP image: 414,176 bytes.

## UI reliability review 0.6.2, 6 November 2026

Eight browser suites pass, including new regressions for timeouts, conflicting
requests, stale power snapshots, delayed save completion, clipped mobile buttons,
keyboard focus clearance and an editor whose connection/output/storage state
changes while open. These reproduced failures in the earlier page.

ESP image: 414,000 bytes. Live OTA verified 0.6.2; saved loco/favorite metadata
survived. A fresh live page preserved drafts/focus through polling, guarded the
open editor during a simulated lost connection and recovered without losing the
draft. Compact power stayed OFF; the live browser check sent no POSTs and found
no JavaScript/timing faults. The Uno firmware/DCC generator is unchanged. See the
[review findings and test mapping](UI-REVIEW.md). Mocked delay/failure tests do
not activate hardware.

## UI/project release 0.6.1, 6 November 2026

Both supported sketches compile from firmware/. All six browser suites pass.
ESP image: 413,280 bytes. Live OTA verified 0.6.1; the original loco metadata and
favorite survived. On the installed page, compact power remains visible through
scrolling/polling and hands back to the main panel at the top, with no JavaScript
or timing faults. Output was left OFF. Uno 0.6.0 did not need a reflash.

## Hardware observations, 6 November 2026

With motor VIN and track disconnected, installed Uno 0.6.0 passed all 29 function
bits by snapshot CRC, full eight-loco masks and 126 active update/KEEP pairs over
eight seconds with zero timing faults. The expiry test disabled A; a late KEEP
did not restart it. The complete HTTP → ESP → Uno path switched every bit ON and
OFF without changing the other loco, with zero timing faults. B stayed disabled.

A temporary named favorite survived actual ESP OTA; requested functions restarted
OFF and original names/addresses/types survived. The temporary favorite was
removed. The final output and all function bits were OFF.

Uno 0.6.0: 12,812 bytes flash and 1,018 bytes static RAM. ESP 0.6.0 image:
412,688 bytes. See build output for the current UI release's size.

Earlier no-load bridge tests measured +15 V and −15 V in the DC polarity tests.
DCC was observed on an oscilloscope around 30 V peak-to-peak with a 15 V supply.
Logic captures at 500 kS/s decoded zero-speed/direction/search packets with valid
XOR. That sample rate has 2 µs resolution; it does not establish sub-microsecond
pulse accuracy. New function-group physical pulse widths were not measured in
the 0.6.0 validation.

## Remaining limits

Actual decoder function mappings, current-sense calibration, loaded short-circuit
handling, thermal limits and long-running powered operation remain unverified.
The raw A0 cutoff is not proven protection. Do not describe successful software
checks as proof of electrical safety or loaded performance. Keep motor VIN and
track disconnected during uploads and firmware diagnostics.
