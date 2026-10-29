# Release 0.4.0 validation

29 October 2026. Tests were performed locally; the GitHub workflow is prepared
but has not run on a hosted repository.

| Check | Result |
| --- | --- |
| Arduino-Pico 6.3.1, Pico W, 1 MB application + 1 MB FS | Compiles: 452796 bytes program (43%), 73744 bytes globals (28%) |
| C++ encoder / assembled PIO model | 254 address/F0 combinations pass; complete-frame updates and exact nominal timing |
| Generated web header | Matches web source |
| Chrome mocked UI | English/Spanish, persisted language, closed Setup, F0 actions, address editing, rejected OTA recovery, offline state, 320/390px layouts pass |
| Live Chrome on Pico | English default, ES toggle, closed Setup, icon, F0 on/off and update redirect pass; no JS errors |
| OTA without USB | 0.4.0 accepted and confirmed after reboot, saved Wi-Fi retained |
| SVG and local document links | Parsed/checked; diagrams visually reviewed |
| Physical DCC capture for this release | Inconclusive: current input flat on all channels; old 0.3.0 baseline also flat; 0.4.0 restored |
| Existing physical reference | Earlier 0.3.0 off/on captures: 14 full packets each, correct bytes/XOR, 58.000–58.125 and 100.000–100.125 µs half-bits |
| Powered track assembly | Proposal only; not built, calibrated or measured |

The unchanged signal engine is documented separately from the proposed power
stage. The UI waveform is illustrative, not a hardware measurement. No track
power or motor movement was enabled. Network configuration was not altered by
UI tests. DCC test actions restored their initial state; firmware startup
retains the documented address-3/F0-off default.

The new helper scripts and CI workflow were syntax-checked or exercised locally
as appropriate. No remote repository was created and no code was pushed.
