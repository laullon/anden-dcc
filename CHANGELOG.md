# Changelog

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
- Port the guided Pico address finder with range, binary questions and confirmation.
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
- Pico firmware remains at 0.6.2.

## ESP13 / Uno bench experiment · 2026-11-02

- ESP13 v0.1.1 adds checked UART communication with Uno stack-test v0.1.0.
- Add web ping/L LED controls, enable-pin readback and uncalibrated ADC readings.
- Hold both L298P enables LOW; there are no bridge-enable or train commands.
- Validate a full original ESP backup, serial installation, Wi-Fi/mDNS, repeated
  OTA, and live web → ESP → Uno communication with the three boards stacked.
- Add shared framing/CRC corruption tests, Uno compilation and browser checks.
- Powered bridges, DCC output and current/overload calibration remain untested.

## 0.5.3

- Add a small train favicon, with SVG and 16/32px ICO formats.
- Serve the standard favicon path with binary-safe responses and versioned links.

## 0.5.2

- Remove decoder-programming UI, API, packet helpers and tests; use another method for addresses.
- Simplify hardware to one bridge output feeding both tracks, one AHCT level shifter and a current-sense divider.
- Retire the programming/latch/comparator proposal; add a single-sheet native KiCad schematic, PDF, net list and matching BOM.
- Native schematic ERC and key-net checks pass; track enable/current cutoff remain future firmware work.

## 0.5.1

- Programming preparation panel in English/Spanish; hardware-required state and disabled read/write controls.
- Forgotten-address link from the editor, with a return path that preserves drafts.
- Explicit unavailable capability/API: valid programming operations return 501 with no transmission or changes.
- Host-tested direct CV byte/bit packets, primary short/extended address decoding and known-short-address plans.
- Programming hardware/transport plan; no real decoder programming is enabled.

## 0.5.0

- Shared roster saved on the Pico: two default locomotives, up to eight.
- Responsive two-column locomotive cards, names, addresses and independent F0.
- Add/edit/remove dialog with duplicate prevention and English/Spanish labels.
- Names/addresses persist across normal OTA and reboot; F0 resets off at boot.
- Hardware DMA ring refreshes every roster address through the existing PIO.
- Idle packets for an empty roster; DMA and PIO stop before OTA.
- Host roster/multi-packet tests and expanded browser regression coverage.

## 0.4.0

- Responsive train-first UI, SVG app identity and GitHub banner.
- English default; optional Spanish remembered by the browser.
- Closed-by-default Setup: Wi-Fi and OTA with upload progress.
- English API errors, icon route and legacy update redirect.
- Editable web source, generated flash header and browser checks.
- Wiring proposal, BOM, net list, source references and build/OTA helpers.
- GitHub CI prepared. PIO waveform engine unchanged.

## 0.3.0

- PIO DCC on GP2; F0 and short addresses 1–127.
- Packet-boundary updates and autonomous repetition.
- Host timing check and real-capture CSV decoder.
- Physical F0 off/on verification through Logic 2 MCP.

## 0.2.0–0.2.7

- Persistent Wi-Fi, setup AP, reconnection and station-only mDNS.
- OTA verified without USB; connection and web-activity LED.
- Background status polls excluded from activity pulses.

## 0.1.0

- Initial setup AP, status page and updater.
