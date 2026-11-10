# Measuring DCC on the Uno

Measure logic pins, **never A+/A−**, with a USB logic analyzer. Verify the analyzer
supports 5 V logic before connecting. Keep motor VIN and track disconnected.

| Lead | Uno pin | Purpose |
| --- | --- | --- |
| GND | GND | Common logic reference |
| CH0 | D10 | Timer1 hardware DCC reference |
| CH1 | D12 | Bridge-A direction signal, fixed-phase mirror |
| CH2 | D3 | Bridge-A enable |

D10 runs continuously when the generator is healthy. D12 carries the routed
waveform only during an explicit DCC bench/track test. D3 indicates output enable;
D11/bridge B stays LOW. No analyzer power connection is needed.

Capture at the fastest supported digital sample rate; a 500 kS/s capture provides
2 µs resolution, enough to distinguish nominal 58 µs and 100 µs half-bits but not
to establish fine edge accuracy. A binary one is 58 µs high + 58 µs low; zero is
100 µs + 100 µs. Decode complete DCC packets and verify XOR, not UART/SPI.

The six slots per loco cover F0–F4, speed/direction, F5–F8, F9–F12,
F13–F20 and F21–F28. Startup emits broadcast stops. Use the
[three-second DCC test](firmware/uno-stack-test/DCC-SCOPE.md) under Setup for a
bounded observation, or the explicit track session with its heartbeat. To check
local motion, keep motor VIN and track disconnected and follow the
[encoder cab guide](firmware/uno-stack-test/CAB.md).

Logic 2 worked with the particular generic analyzer used for development;
compatibility of other units varies. Record device, sample rate, channels and
whether motor power was connected. Rail measurements require a suitable
oscilloscope connection, not these logic inputs.
