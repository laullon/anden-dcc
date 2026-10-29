# Release 0.5.0 validation

29 October 2026. Local tests and real Pico OTA/HTTP/browser checks.

| Check | Result |
| --- | --- |
| Arduino-Pico 6.3.1, Pico W, unchanged 1 MB application / 1 MB FS layout | Compiles: 469196 bytes program (44%), 74192 bytes globals (28%) |
| Production C++ roster | Independent F0, rename preservation, address-change reset, duplicate/invalid/full rejection, removal and ID reuse pass |
| Assembled PIO model | All 254 address/F0 cases and 2/3/8-loco schedules repeat complete frames at exact nominal 58/100 µs half-bit timing |
| Empty roster | Host test emits valid `FF 00 FF` idle frames; UI empty/add flow passes |
| Mocked browser | Two desktop cards / stacked mobile, independent F0, add/edit/remove up to eight, duplicate prevention, failed saves, draft preservation and stale-poll protection pass |
| UI regression | EN/ES persistence, collapsed Setup, failed OTA recovery, offline disabled controls, dismissible offline editor, 320/390px layouts and safe text rendering pass; no JS errors |
| Live Pico API and two browser contexts | Naming, independent F0, add/remove, input rejection, cross-browser state, Spanish and mobile pass |
| Actual OTA restart | Shared roster IDs/names/addresses and Wi-Fi survive; all F0 reset off; original test state restored |
| Final firmware | 0.5.0 deployed over Wi-Fi; GP2 logic only; track power remains false |
| Physical multi-address waveform | Pending: Logic 2 MCP reports no connected devices; earlier 0.3.0 captures validate the original single-address pulses, not the new DMA scheduler |
| Powered track | Still a proposed assembly; no bridge, track power or movement enabled |

The PIO program/encoder are unchanged. Two claimed DMA channels now feed an
eight-word aligned read ring and reload it without CPU callbacks. Host schedule
checks exercise packet selection and timing; they do not prove the physical DMA
transfer loop or voltage. Capture both addresses on GP2 at the bench next.

Screenshots in `assets/` use synthetic names and network details. No router
configuration or credentials were changed. Live tests restored the initial two
slots (addresses 3 and 4, empty names, F0 off).

## Historical release 0.4.0 validation

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
