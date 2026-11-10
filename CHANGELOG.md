# Changelog

## Cab movement indicator · Uno 0.7.2

- Drive the optional D7 LED while a nonzero cab speed is commanded on powered track.
- Works with ESP13 0.7.1.

## Diorama selection and STOP · ESP13 / Uno 0.7.1

- Save **On diorama** per loco. The cab only selects marked addresses.
- Press the encoder button to cycle marked locos. Use the separate D2 button
  to stop all motion while preserving track power and selection.
- Update the ESP13 first to enable the new cab filter capability.

## Encoder cab · ESP13 / Uno 0.7.0

- Adjust local crawl speed by turning an encoder. Press to stop and hold to
  select the next address. Show the address and step on a 128×32 I²C OLED.
- Limit motion to one locomotive, three crawl steps and 800 ms per move.
  Prioritize stop packets and add an interrupt backup cutoff.
- Keep the web power session, direction, roster and function controls. Clear
  motion on schedule changes or loss of track power.

## Full review · ESP13 0.6.5 / Uno 0.6.1 · 2026-11-06

- Validate Uno roster bounds before signature buffers or address pointers are used.
- Apply legacy roster schedules even when a different mode has a colliding CRC16.
- Preserve control readiness when a malformed roster command is rejected.
- Use a monotonic finder settling clock, independent of system clock corrections.
- Validate image headers, app identity, embedded size and core CRC in the browser,
  helper and ESP endpoint; reject filesystem/multiple/incomplete upload requests.
- Commit staged firmware only after the complete HTTP request passes validation.
- Preserve nonblank/unreadable flash on a failed filesystem mount; only format
  verified erased storage on first use.
- Guard Wi-Fi/update changes during timed bench outputs as well as track operation.
- Verify asset markers and preserve literal SVG text during web embedding.
- Require the tested installed Arduino core versions before building.
- Add native parser, sanitizer, image, HTTP upload and generator regression checks.

## C++ module refactor · 2026-11-06

- Move nontrivial implementations into separately compiled `.cpp` modules.
- Give RosterControl explicit server/Uno dependencies and private model/session ownership.
- Keep Uno scheduler state and the Timer1 ISR private to UnoDcc.cpp.
- Generate the web payload once in SetupPage.cpp with declarations in its header.
- Link native tests to production objects; add standalone-header and multiple-TU checks.
- Preserve protocol, storage, UI, timeouts and release identifiers; no hardware reflash.

## Consistent function toggles 0.6.4 · 2026-11-06

- Use the same single-button On/Off control for every function and the explorer.
- Always show F0 on every loco, using its saved name or Headlights.
- Allow F0–F28 presets with track power off, without enabling output.
- Preserve request, connection, firmware-capability and finder guards.

## Human-readable controller status 0.6.3 · 2026-11-06

- Describe track power, DCC operation and the second output in plain language.
- Keep pin levels and uncalibrated sensor counts in collapsed Technical readings.
- Distinguish normal track operation from timed bench tests; remove the misleading
  automatic-stop countdown during normal operation.

## Web UI reliability review 0.6.2 · 2026-11-06

- Bound ordinary command requests and recover controls after a lost response.
- Serialize finder/roster/setup operations and invalidate stale polls/replies.
- Reflect checked power acknowledgments without flashing back to cached state.
- Preserve new favorite-name drafts while saves or status polls finish.
- Guard an open loco editor when connection, storage or output state changes.
- Wrap long F0 labels and keep keyboard-focused inputs above the floating bar.
- Add deterministic regressions that failed on the previous release.

## Uno / ESP13 project and scroll power control 0.6.1 · 2026-11-06

- Keep compact track power status and On/Off controls visible after scrolling.
- Await coalesced status refreshes consistently so overlapping requests cannot
  leave callers reading stale UI state.
- Promote both active sketches into firmware/ and make the stack the sole target.
- Replace obsolete firmware, wiring, build jobs, previews and docs with the Uno,
  L298P and ESP13 build. Preserve sketch names, UART protocol and ESP flash layout.

## ESP13 / Uno function favorites 0.6.0 · 2026-11-06

- Add independent F0–F28 for each locomotive, with continuous refresh of all five
  function groups and zero-speed direction packets.
- Add an expandable function explorer and eight named favorites per loco; quick
  buttons stay visible when collapsed. Preserve typing, selection and focus during
  polling, including a held mouse click.
- Save favorite metadata alongside the roster, migrate existing files on save,
  and start active functions OFF after ESP reboot.
- Add capability-gated CONTROLX snapshots with 29-bit function masks; older Uno
  firmware retains F0 controls and can save favorites before upgrading.
- Test function packet encoding, atomic publication, full-size transport, favorite
  validation, browser persistence, failed saves and narrow light/dark layouts.

## ESP13 mouse interaction and connection handling 0.5.4 · 2026-11-06

- Preserve unchanged loco cards through polling so hover, focus and held mouse
  targets survive; add clear hover/pressed/disabled feedback.
- Coalesce status requests and reduce active polling to once per second; release
  HTTP connections after replies and disable TCP coalescing for control latency.
- Remove visibility-triggered Stop, retry transient heartbeat failures and reject
  stale page cleanup without stopping a newer track session.
- Give browser reconnection six seconds while the ESP maintains the unchanged
  independent two-second Uno lease. Surface stop reasons in the power card.
- Test held clicks, page visibility, retries, session expiry, stale tokens and wrap.

## ESP13 editable finder range 0.5.3 · 2026-11-03

- Allow First/Last address drafts while track power is off or the Uno is offline.
- Keep Start search guarded by the active page session and track power; explain
  the requirement beside the button and preserve range drafts across polling.

## ESP13 stable power transitions 0.5.2 · 2026-11-03

- Keep exactly one action visible while switching power on/off.
- Fix the action column width, reserve status-text space and remove redundant
  success messages so the power card and loco cards stay in place.
- Keep DCC diagnostics compact and stable across power changes.
- Verify delayed transitions at desktop, narrow-panel and phone widths.

## ESP13 power UI and directional lamps 0.5.1 · 2026-11-03

- Show explicit track ON/OFF, unknown and bench states with a matching action.
- Make the active power-off action red and retain off access while status is unknown.
- Swap white headlights and red tail lights with direction on all three illustrations.
- Darken the preview when F0 or track output is off; verify state and lamp rendering.
- ESP-only update; Uno 0.5.0 remains compatible.

## ESP13 / Uno stationary locomotive test 0.5.0 · 2026-11-03

- Add explicit track power, a browser-owned heartbeat and a two-second Uno lease.
- Keep speed fixed at zero, send startup broadcast stops, and add per-loco direction.
- Add the guided address finder with range, binary questions and confirmation.
- Generate search packets in the Uno without a large SRAM table; repeat zero-speed
  packets and use a ten-second settling interval for the full range.
- Keep current/timing cutoff, default-off behavior and bench tests; add packet,
  lease and browser checks. See the first-locomotive guide for verification limits.

## ESP13 / Uno timed DCC scope test 0.4.0 · 2026-11-03

- Route DCC to D12 through the normal shield header for a fixed three-second
  bridge-A scope burst, with packet-boundary start and no extra jumper.
- Keep D10 as the hardware reference; add a fixed-phase ISR mirror and late-edge
  fail-off, preserving the raw cutoff, Stop and interrupt backup timeout.
- Add mode/capability telemetry, a DCC test button, synchronized-roster guard
  and compatibility with older Uno replies/DC tests.
- Add actual-ISR register simulation and validate auto-off under Uno UART load.
- Confirm the no-load powered waveform visually on a scope; exact physical
  pulse timing and loaded operation remain unmeasured.

## ESP13 / Uno no-load bridge test 0.3.0 · 2026-11-03

- Add fixed three-second DC pulses in both bridge-A directions and a Stop command.
- Keep startup/idle disabled and B off; add raw A0 cutoff and a Timer1-interrupt
  backup cutoff. Repeated commands cannot extend or reverse an active pulse.
- Add Setup bench controls, active/fault diagnostics, roster locks and legacy
  Uno compatibility. DCC remains on D10, separate from the motor-shield test.
- Verify isolated 15 V supply and about +15 V / −15 V no-load bridge-A output,
  with both enables LOW after testing. Add native and browser checks.

## ESP13 theme buttons 0.2.3 · 2026-11-03

- Replace the theme dropdown with System / Light / Dark buttons and a highlighted
  active choice. Keep system following and saved browser preferences.
- Verify pressed states, keyboard activation, contrast and mobile touch targets.

## ESP13 themes 0.2.2 · 2026-11-03

- Add a visible System / Light / Dark theme selector to the ESP panel.
- Follow live system appearance changes by default; save explicit choices in
  browser storage and apply them before paint. Storage failures remain usable.
- Theme loco cards, forms, editor dialogs, status/error messages and native controls.
- Check text contrast, saved choices, system changes, drafts, offline operation
  and mobile layouts without sending controller commands. Uno remains at 0.2.0.

## ESP13 feedback 0.2.1 · 2026-11-03

- Use the DOIT shield blue LED on GPIO16 (active LOW) for Wi-Fi and UI feedback:
  500 ms blinking while disconnected, steady when connected, and a 120 ms
  off pulse on user requests. Automatic status polls remain quiet.
- Add LED pin/state/request diagnostics and host checks for pulse extension,
  background polling and millis rollover. Uno firmware remains at 0.2.0.

## ESP13 / Uno DCC experiment 0.2.0 · 2026-11-03

- Generate F0 DCC packets on Uno D10 with Timer1 hardware toggles, whole-packet
  roster updates, idle packets and timing-deadline fault telemetry.
- Add two loco cards (up to eight), independent F0, shared Electric/Diesel/Steam
  artwork, editable names/addresses and persistent ESP LittleFS roster storage.
- Keep F0 RAM-only, reset it OFF after ESP boot, and synchronize complete roster
  snapshots with checked Uno acknowledgements. Keep both bridge enables LOW.
- Gzip the self-contained ESP UI; preserve collapsed Wi-Fi/OTA Setup.
- Verify live F0 commands, saved roster through another OTA, F0 reset-off, Uno
  recovery, desktop/mobile UI, and native packet/boundary tests.
- Uno physical pulse widths and powered motor-shield operation remain pending.

## ESP13 / Uno bench experiment · 2026-11-02

- ESP13 v0.1.1 adds checked UART communication with Uno stack-test v0.1.0.
- Add web ping/L LED controls, enable-pin readback and uncalibrated ADC readings.
- Hold both L298P enables LOW; there are no bridge-enable or train commands.
- Validate a full original ESP backup, serial installation, Wi-Fi/mDNS, repeated
  OTA, and live web → ESP → Uno communication with the three boards stacked.
- Add shared framing/CRC corruption tests, Uno compilation and browser checks.
- Powered bridges, DCC output and current/overload calibration remain untested.
