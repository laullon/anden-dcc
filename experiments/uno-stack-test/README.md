# Uno + ESP13 + L298P: USB-only bench test

This checks the three-board stack without generating DCC or powering a track.
The ESP hosts the web controls; the Uno replies over its hardware UART and holds
both motor-shield enable pins LOW. The only action command blinks the Uno L LED.

## Setup

Keep **motor VIN and tracks disconnected** for this test. Power the stack from
the Uno USB. Remove the separate USB-UART adapter's signal wires; the ESP uses
its normal stacked UART connection, including the shield's level conversion.

1. Build both sketches with the repository's local serial library:

   ```sh
   tools/build-uno-stack.sh
   tools/build-esp13.sh
   ```

2. Turn both ESP13 switches **OFF** for an Uno USB upload:

   ```sh
   arduino-cli upload --fqbn arduino:avr:uno --port UNO_PORT \
     --input-dir build/uno-stack-test
   ```

3. Update the already-installed ESP firmware over Wi-Fi:

   ```sh
   python3 experiments/esp13-ota/ota.py --host ESP_IP \
     --file build/esp13-ota/esp13-ota.ino.bin --expect-version 0.1.1
   ```

4. Turn both switches **ON** to connect Uno and ESP. No reset is required.
5. Open the ESP page. The **Uno + motor shield** card shows the link status,
   last round-trip time, enable-pin readback and raw ADC counts. Try **Ping Uno**
   and **Blink Uno L LED**.

The build scripts supply `--libraries libraries`. To use Arduino IDE directly,
install/copy `libraries/AndenStackLink` into its sketchbook libraries directory.
The Uno sketch uses AVR core 1.8.7; the ESP uses ESP8266 core 3.1.2.

## Pin states

| Shield function | Uno pin | Test behavior |
| --- | --- | --- |
| Enable A | D3 | Held LOW |
| Enable B | D11 | Held LOW |
| Brake A / B | D9 / D8 | LOW |
| Direction A | D12 | LOW |
| Direction B / Uno L LED | D13 | Eight short flashes on request, then LOW |
| Current-sense A / B | A0 / A1 | Raw ADC readings; not converted to amperes |
| Uno RX / TX | D0 / D1 | Checked UART messages to/from ESP |

D13 is also the bridge B direction input. The web LED endpoint first requests
fresh pin states and rejects the action if either enable is HIGH. The Uno keeps
both enables LOW in its loop; there are no commands to enable a bridge. During
reset/upload the MCU cannot hold outputs, which is why motor VIN stays disconnected.
Power LEDs stay on; this test does not assign an ESP user-LED pin or test powered
motor-output indicators.

## Wire protocol and API

The shared `AndenStackLink` library frames bounded ASCII messages as
`@BODY*CRC16\n`, with a 96-character body limit, CRC16-CCITT (initial 0xFFFF), and
resynchronization at the next `@`. It rejects corrupt, overlong, empty-field and
malformed messages. ROM startup noise is ignored. Responses must carry the
current nonzero request ID; this is not a continuous echo bridge.

Requests: `AD1,id,PING`, `AD1,id,STATUS`, or `AD1,id,LED`.
Responses: `AD1,id,STATE,version,adcA,adcB,flags,ledEdges`.
Flag bits 0..5 report enable A, enable B, brake A, brake B, direction A, direction B.
The ESP requests status every two seconds. Timeout counts include periods when
the switches were intentionally OFF. Raw ADC values can vary with USB/RF noise;
they are not calibrated current measurements or a short-circuit cutoff.

- `GET /api/status` includes `uno` status and explicitly reports DCC unimplemented.
- `POST /api/uno/ping` returns a fresh checked Uno response, or HTTP 503.
- `POST /api/uno/led` requires a fresh reply with both enables LOW; HTTP 409 blocks
  an unsafe enable state. A successful reply starts the short L LED pattern.
- ESP OTA still uses `/update`; it updates only the ESP, not the Uno.

The Uno UART is also connected to its USB converter. Switches OFF isolate the ESP
for Uno uploads/direct USB commands; switches ON connect the stack. With ON, use
the ESP API to send test commands rather than writing from the Mac into that
shared serial input.

## Hardware observations, 2026-11-02

- Uno test v0.1.0 compiled to 4,996 bytes, with 380 bytes of static RAM.
- ESP web test v0.1.1 compiled to 361,232 bytes, within the 448 KiB OTA limit.
- With switches OFF, a checked USB ping returned Uno v0.1.0, both enable pins LOW,
  and initial A0/A1 readings of zero.
- With switches ON, the complete web → ESP → Uno → ESP path returned in 4–5 ms,
  with no CRC errors in the captured exchanges.
- The LED command changed D13 HIGH/LOW and finished with it LOW. Both enable
  readbacks stayed LOW in every captured reply. The user also saw the L LED blink.
- A0 was zero in captured samples; A1 occasionally returned small nonzero raw
  values (up to 28 in those samples). No current calibration is claimed.
- An additional ESP v0.1.1 OTA update passed with the UART connected and all
  three boards stacked. After reboot the Uno link recovered with zero timeout
  and bad-frame counts; both enable pins read LOW.
- Powered bridge operation, track waveform, real current sensing under load,
  overload protection and locomotive control remain untested by this build.
