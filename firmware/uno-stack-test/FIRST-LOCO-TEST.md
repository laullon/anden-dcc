# First locomotive test: lights, direction and address finder

Uno **0.6.0** and ESP **0.6.4** provide stationary track operation to the Uno/ESP13/L298P stack.
Speed is fixed at **zero**; the Uno rejects every nonzero speed request. No throttle,
CV programming or automatic power restoration are provided. F0–F28 and named
favorite functions are available in each locomotive card.

## Before the first run

Use the [verified 15 V supply and cut VIN CONNECT link](POWER-TEST.md). Logic
remains USB-powered. Track A connects across A+ and A−; neither rail is GND.
Bridge B stays disabled. Turn motor power off before changing wiring.

Install both firmware images with motor VIN and trains disconnected. For the Uno
upload, both ESP13 switches must be OFF; turn them ON afterward. ESP updates use
its usual OTA endpoint. An ESP update alone cannot update the Uno. Old Uno firmware
keeps the new track/direction controls disabled.

1. With power off, connect a short test track and **one locomotive** to bridge A.
2. Reconnect the isolated 15 V motor supply and open the panel.
3. Set the locomotive's short address (1–127) in its card, if known.
4. Press **Power on**. Startup sends 32 broadcast emergency-stop packets before
   repeating each roster entry's function groups and 128-step zero-speed/direction packets.
5. Switch F0 on and use **← Rev / Fwd →** to test directional headlights. These
   are requested directions; actual end/lamp behavior depends on decoder mapping
   and its normal-direction configuration. The decoder must support 128-step
   speed/direction packets. The direction choice is RAM-only and defaults forward.
6. Press **Power off** before changing the roster or moving wires.

## Find an unknown address

Keep one locomotive on the test track. Power on, then choose **Find address**.
Enter a possible range, for example **1–100**; address 0 is broadcast, not a loco
candidate. The search first verifies all-on/all-off, halves the candidate range
based on your answers, then confirms a single address with on/off again.

Every candidate receives repeated F0 and zero-speed packets. Each step waits
**10 seconds**: the Uno must service up to 127 addresses with both packet types.
Repeat a question if
unsure. Do not answer before the locomotive's lamps have settled.

Normal loco controls, roster editing, Wi-Fi changes and OTA are locked during a
search. Cancel, success or a three-minute inactivity timeout clears search lights
before restoring the roster. Power off remains available immediately. Losing track
power cancels the search. After confirmation, turn power off and use **Use this
address** to open the existing card or a new card with that address filled in.

This is a human-observed F0 search, not a CV read. Long addresses, consists and
nonstandard headlight mapping can prevent a match. Failed confirmation does not
write decoder configuration.

## Local cutoff and connection loss

- Output starts off and requires an explicit Power on after restart, timeout or trip.
- Only the page that requested power renews its browser session, every 500 ms.
  Ordinary status polling does not renew it. Changing tabs is allowed; closing
  or reloading the owning page requests off. Stale page cleanup cannot stop a
  newer session.
- The ESP tolerates brief browser/network interruptions for **6 seconds**,
  maintaining checked Uno KEEP messages every 500 ms while that session is alive.
  After browser expiry it requests Stop. If the ESP fails, the Uno independently
  expires after **2 seconds** without KEEP. Neither path restarts output.
- An interrupt guard still disables A after 60,000 half-bits (3.48–6 seconds) if
  main-loop servicing stalls while interrupts continue. Timing faults disable A.
- The existing raw A0 cutoff **100** remains unchanged. It is deliberately
  conservative and **uncalibrated**, not a proven short-circuit protection system.
  Decoder startup current may trip it. Stop and inspect rather than increasing
  the limit blindly. Loaded fault behavior and current calibration remain unverified.

## Protocol and implementation

The STATE reply has an eighteenth capability field: `1` for stationary control,
`2` for stationary control plus F0–F28 (Uno 0.6.0).
Bench mode 3 is stationary track operation; state 7 is lease expiry. Earlier
8/14/16/17-field responses remain readable.

- `CONTROL,count,f0Mask,directionMask,address,0,...`: atomic stationary roster.
- `CONTROLX,count,directionMask,address,functionMask,...`: 0.6.0 atomic roster,
  with one decimal 29-bit mask per address (bit 0 = F0, bit 28 = F28). Maximum
  eight short addresses; the checked body limit is 144 bytes. All speed packets
  remain zero. The ESP uses this only after seeing capability `2`.
- `TRACKON`: explicit start, requiring a prepared control/search schedule.
- `KEEP`: renew only an already-running stationary track lease.
- `BENCHSTOP`: immediate output disable.
- `FIND,first,last,onLow,onHigh`: generate a range in the ISR without a large
  packet table. `0,0` in the last pair means all-off.

The ESP exposes `/api/track/on`, `/api/track/keep` (session lease),
`/api/track/off`, `/dcc/direction`, `/finder/start`, `/finder/answer` and
`/finder/cancel`. Search replies include a step token; stale answers are rejected.
The search state machine and Uno packet generator are checked separately. Search and roster banks change only at packet boundaries.

## Verification

Uno build: 10,752 bytes flash / 794 bytes static RAM. ESP image: 404,384 bytes.
The ESP build and browser suites also pass.

Native tests decode stop/direction/F0 packets and XOR, every publication phase,
every search range/target and all 127 generated splits. They cover lease expiry,
rollover, no automatic restart, current trips and the interrupt cutoff model.
Browser tests cover explicit power, heartbeat/refusal, Power off, direction/F0,
search to address 37, settling, editor handoff, locks, legacy firmware and mobile.

With motor VIN disconnected, the installed Uno passed checked USB control,
nonzero-speed rejection, live direction/F0 updates, full-range search commands,
restore and manual off. Missing KEEP disabled output after 2.055 seconds; a late
KEEP did not restart it. The timing-fault counter stayed zero.

A 500 kS/s logic capture decoded 1,261 packets with valid XOR, including both
requested directions and search packets; every decoded speed command was zero.
The brief search captures included 92 distinct addresses; exhaustive range
coverage is a host-test result. D10/D12 showed nominal 58/100 µs halves with the
capture's 2 µs resolution. This is logic-pin verification, not a loaded rail test.

The installed ESP/Uno pair also completed a 1–100 search with simulated answers
for address 37, including settling, stale-step rejection, cleanup and restored
roster. In version 0.5.0, observed web lost-heartbeat cutoff was 2.179 seconds including
polling latency; ESP 0.5.4 now uses the separate six-second browser grace above. The actual UI sustained power, changed direction/F0 and switched off on
reload without restarting; no browser errors or timing faults were reported.

On 2026-11-06, the installed ESP 0.5.4 / Uno 0.5.0 pair stayed enabled through a
three-second browser-heartbeat gap. It disabled output after 6.129 seconds when
heartbeats stopped (including polling latency). Late KEEP did not restart it;
stale KEEP and page-cleanup requests did not stop a newer session. Manual off
passed and the timing-fault counter remained zero. A locomotive was connected,
but motor VIN and load current were not independently measured during this check.

The installed UI also preserved a held mouse target through four real status
polls, acknowledged the click, restored the original F0 request and kept output
off. Browser simulations cover tab visibility, one failed heartbeat response and
recovery, focus/target stability, legacy controls and search flows.

Actual locomotive behavior, loaded timing and current protection remain unverified.

Packet reference: [NMRA S-9.2.1, broadcast, 128-step control and function groups](https://www.nmra.org/sites/default/files/standards/sandrp/DCC/S/s-9.2.1_dcc_extended_packet_formats.pdf).


## F0–F28 refresh (0.6.0)

Each locomotive gets six packet slots: F0–F4, zero-speed/direction, F5–F8,
F9–F12, F13–F20 and F21–F28. The entire schedule repeats while the generator is
running; enabling the bridge still requires explicit track power. Instructions
are `80`/F0–F4, `B0`/F5–F8, `A0`/F9–F12, `DE data`/F13–F20 and
`DF data`/F21–F28, with XOR checksums. See
[NMRA S-9.2.1](https://www.nmra.org/sites/default/files/standards/sandrp/DCC/S/s-9.2.1_dcc_extended_packet_formats.pdf).

The Uno prepares two banks of compact packet bytes in the main loop and selects
the next prebuilt packet at a packet boundary, avoiding a full 48-packet bitmap
per bank and keeping function encoding out of the timing ISR. Publication remains
atomic. Its signature covers every address, direction and function bit. Metadata
names/pins stay on the ESP. Function refresh does not add speed or CV programming.

Native tests decode every function bit, both directions, all publication phases,
eight-loco refresh, startup stops and maximum UART frames. Browser tests cover
pinning, drafts, independent cards and failed saves. These tests do not measure
AVR execution time: validate the new ISR schedule on D10/D12 with motor VIN and
track disconnected before using it with a locomotive.


Installed 0.6.0 validation with motor VIN and track disconnected: Uno **12,812
bytes flash / 1,018 bytes static RAM**, ESP image **412,688 bytes**. All 29 bits
were acknowledged by snapshot CRC. Eight-loco full-mask traffic and 126 active
updates/KEEP pairs over eight seconds produced zero timing faults; expiry shut
A off and a late KEEP did not restart it. The complete HTTP/ESP/Uno path then
switched every bit ON/OFF independently with zero timing faults. A temporary
favorite survived actual OTA; requested functions restarted OFF, existing
addresses/names/types survived, and the temporary pin was removed. Final output
and all requested function bits were OFF. Physical pulse widths and locomotive
function mapping were not measured in this validation.
