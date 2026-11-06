# Full code review · 6 November 2026

Reviewed the Uno sketch, Timer1 ISR and scheduler, shared framing/packet/model
library, ESP control/session/storage code, web request and interaction handling,
OTA and generated-asset tooling, build scripts and CI. Baseline: `5fd79e2`.
The resulting firmware versions are ESP13 **0.6.5** and Uno **0.6.1**.

## Confirmed issues and fixes

| Finding and trigger | Fix | Regression coverage |
| --- | --- | --- |
| `UnoDcc::set()` calculated a signature before validating count; count 9 overflowed an eight-address stack buffer | Reject oversized counts/null address inputs before preparation | `check_uno_bridge_route.py`, actual module with ASan/UBSan |
| A CRC16 collision made a legacy `ROSTER` falsely match an extended-function schedule | Remove the signature-only early return; always prepare the requested schedule | Actual ISR packets for address 3, flags 122242 versus F0-off legacy roster: both signatures 52175 |
| Malformed `ROSTER` cleared stationary-control readiness despite rejecting the command | Change readiness only after validation and publication succeed | `check_uno_commands.py`, actual sketch parser with simulated hardware |
| Finder settling used wall time, so clock corrections unlocked answers early or produced enormous countdowns | Use `performance.now()` for elapsed time | Browser regression moves wall time forwards and backwards |
| Browser/helper OTA checks accepted truncated or corrupted images containing a valid marker | Validate both headers, exact embedded length and ESP8266 core CRC | Independent fixture CRC, damaged headers/metadata/content, truncation/trailing data, actual compiled image |
| The stock `/update` endpoint allowed filesystem writes and bypassed client image checks | Accept one firmware file only; stream validation; defer commit until the complete request is accepted | `check_firmware_image.py` and `check_firmware_update.py` link production modules |
| A failed filesystem mount formatted existing storage unconditionally | Verify the entire region is erased before first-use formatting; preserve nonblank/unreadable storage | `check_roster_storage.py`, actual policy with fake flash and ASan/UBSan |
| Setup lock covered normal track operation but omitted timed bench outputs | Lock Wi-Fi/update changes during any active bench/track output | Target build; existing bench-state coverage |
| Missing/duplicated art markers silently generated incomplete assets; regex replacement interpreted SVG backslashes | Require exactly one ordered marker pair and use literal replacement | `check_embed.py`, isolated generator with malformed markers and literal backslashes |
| Build manifests claimed a tested core version even if another was installed | Require exact installed ESP8266 3.1.2 / AVR 1.8.7 cores | Positive/negative local CLI checks; CI installs these exact versions |

## Update transaction

`FirmwareImage` validates incrementally without retaining the image in RAM.
`FirmwareUpdate` stages to the updater area using `U_FLASH`, never `U_FS`.
One byte remains withheld until the HTTP completion handler. This ensures that
discarding an invalid upload with `Update.end(false)` cannot activate it, even
when the upload fills the advertised capacity. A valid file's end callback does
not commit it: a later multipart file still invalidates the request.

Native tests cover missing/wrong/multiple files, corrupt/truncated images,
interruption, abandoned requests, staging/finalization failures, capacity-sized
failures, retries and exactly one successful commit/reboot. The lifecycle was
also checked against the installed ESP8266 core 3.1.2 parser/updater sources.

## Verification and limits

Both firmware builds and all eight browser suites pass. Native regressions link
production translation units, with simulated registers, serial input, flash or
HTTP callbacks where physical hardware would otherwise be required. Portable
interfaces and new native code were checked with GCC as well as Clang.

The normalized Timer1 ISR and half-bit scheduler retain the same instruction
streams as the pre-refactor baseline: 112 and 478 instructions respectively,
ignoring linker placement. This comparison and the register models do not
replace physical timing or loaded electrical measurements.

No device was reflashed or powered by this review. No credentials, authentication,
protocol formats, filesystem layout, throttle or programming support were added.
