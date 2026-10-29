# Release 0.5.2 · simple controller validation

29 October 2026. Decoder programming has been removed. The current hardware
proposal uses one bridge output for both tracks and one AHCT buffer IC.

| Check | Result |
| --- | --- |
| Pico W / unchanged 1 MB sketch + 1 MB filesystem | Compiles: 469196 bytes program (44%), 74192 bytes globals (28%) |
| Existing encoder / PIO / roster checks | Pass |
| Mocked UI | Two-loco controls and all previous non-programming regressions pass; programming controls absent |
| OTA / real browser | 0.5.2 deployed; both loco cards and saved settings retained; no JS errors |
| Removed API | No programming status object; all former programming endpoints return 404 |
| Native KiCad ERC | Zero violations on the single-sheet Rev C schematic |
| Exported electrical connectivity | 21 circuit components; both tracks share A, B disabled, buffer input/output enables and pull-downs checked, ADC divider and VSYS diode polarity checked |
| Documentation | BOM and pin list updated to Rev C; single-sheet SVG/PDF and editable native project provided |
| Physical power / current cutoff | Not built or validated; enable/current cutoff firmware still required before track power |
| Analyzer | User deferred troubleshooting; no new physical signal pass claimed |

KiCad rules/netlist checks establish drawing consistency, not electrical or
thermal performance. Firmware still drives only GP2 logic and reports
`trackPower=false`. The GPIO waveform/roster engine was not changed by this
release. Pre-OTA F0 states were restored after deployment.

## Historical release 0.5.1 · programming preparation validation

29 October 2026. This release prepares the UI and packet/address helpers;
**physical decoder reading/writing is not implemented or tested**.

| Check | Result |
| --- | --- |
| Pico W / Arduino-Pico 6.3.1 / unchanged flash layout | Compiles: 480164 bytes program (45%), 74192 bytes globals (28%) |
| Existing encoder, PIO model and multi-loco roster checks | Pass unchanged |
| Service helpers | All CV1–1024 byte values / byte and bit operations, packet XOR, known vectors, short-address plans, short/extended address interpretation, explicit missing/ambiguous ACK outcomes pass on host |
| Mocked browser | Programming collapsed initially, hardware-needed badge, disabled read/write, forgotten-address navigation with editor draft preserved, EN/ES and mobile layouts pass; existing F0/roster/OTA/offline regressions pass |
| OTA / real browser | 0.5.1 deployed and verified; both locos retained; Programming panel and 320px layout pass; no JavaScript errors |
| Real programming API refusal | 6 valid requests return 501 and 8 invalid requests return 400; DCC running state and full roster unchanged |
| Saved settings | Wi-Fi and roster retained; pre-OTA F0 states restored after verification |
| Actual CV I/O | Not attempted: no programming transport, powered isolated output or calibrated ACK acquisition exists |
| Analyzer | Deferred at the user's request; no new waveform pass claimed |

The ordinary GP2 packet generator, DMA roster schedule and power-pin behavior
are unchanged. Programming POST handlers only validate input and report
unavailability. The service helpers cannot be passed into the current fixed
three-byte transmitter. See the historical programming preparation (removed in 0.5.2).

## Historical release 0.5.0 validation

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
