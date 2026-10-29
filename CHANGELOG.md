# Changelog

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
