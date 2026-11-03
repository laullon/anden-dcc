# Uno / ESP13 / L298P: first powered bench test

This guide starts with a **no-load DC test**. The separate
[timed DCC scope test](DCC-SCOPE.md) is available in firmware 0.4.0. Leave trains, rails and loads
disconnected from both bridges. The firmware can pulse **bridge A for 3 seconds**
in either direction; B stays disabled. There is no continuous track mode.

## Power wiring

The Deek-Robot shield normally joins its motor VIN terminal to the Arduino VIN
header. Isolate that connection **before using the planned 15 V motor supply**.
Our stack keeps the Uno/ESP logic powered from Uno USB.

1. With the PD output disconnected from every board, select 15 V using the PD
   module's own table. Verify its output voltage and polarity with a meter.
2. Unplug all supplies and remove the motor shield. Identify its VIN CONNECT link.
   Open only that link. Confirm motor VIN terminal → Arduino VIN header is open,
   while GND terminal → GND header still has continuity. Check the actual clone;
   do not cut an unidentified trace.
3. Restack the boards. With the PD charger unplugged, wire:

   | Source | Shield terminal |
   | --- | --- |
   | PD positive | VIN screw terminal |
   | PD negative | GND screw terminal |
   | Meter red / black | A+ / A−, for the output test only |

4. Power Uno USB first. With both ESP switches ON, wait for Uno connected and
   **D3 LOW / D11 LOW** in the panel. Then connect the PD charger and verify the
   voltage at the shield's VIN/GND terminals. The unloaded L298P should stay cool;
   disconnect the motor supply if it heats rapidly.

Never put 15 V on the 5 V header. Keep the VIN CONNECT link open for this setup.
Disconnect motor VIN before Uno uploads, resets, or changing the physical wiring.
Do not connect D10 to D12: D12 is an output in this firmware.

Sources: [DCC-EX isolation and power wiring](https://dcc-ex.com/ex-commandstation/diy/assembly.html#disable-power-sharing-arduino-to-motor-driver),
[Arduino shield schematic](https://www.arduino.cc/en/uploads/Main/arduino_MotorShield_Rev3-schematic.pdf),
[ST L298 datasheet](https://www.st.com/resource/en/datasheet/l298.pdf).

## Meter test

Install **Uno 0.4.0** and **ESP 0.4.0** using the stack README. Keep motor VIN
unplugged for the Uno upload; both ESP switches OFF. Afterward turn the switches
ON, confirm the link, and reconnect the isolated motor supply.

Open **Setup → Motor shield bench test**. Use the meter in DC-volts mode across
**A+ and A−** with red on A+, black on A−. No other output connection is needed.

- **A + · 3 s** drives D12 HIGH and enables A temporarily.
- **A − · 3 s** drives D12 LOW and enables A temporarily.
- **Stop test** disables A immediately when the Uno receives it.

Read the voltage during each pulse. Expect opposite polarities and a magnitude
below the supply voltage because of bridge losses. Do not use this test with a
locomotive: it applies steady DC, while the DCC logic stays on the unused D10 pin.
After a pulse, enable A returns LOW. The output is then high impedance, so a meter
reading on the unloaded terminals need not settle to exactly zero immediately.

## Limits and stop behavior

- Each pulse has a fixed 3,000 ms deadline owned by the Uno. Another pulse request
  while active is refused and cannot reverse or extend it.
- The main loop targets a raw A0 sample every millisecond while active; UART
  responses can extend that interval. A value
  of **100 ADC counts or higher** stops/refuses a pulse. This is a conservative,
  **uncalibrated raw limit**, not a validated current rating or decoder protector.
- Timer1 also counts 60,000 DCC half-bit interrupts. If the main loop stalls but
  interrupts continue, this backup drops D3 after a nominal 3.48–6 seconds. A Timer1 timing
  fault drops D3 immediately. The guard is not independent hardware protection
  against MCU failure, blocked interrupts or a damaged enable circuit.
- B is held LOW. Startup is disabled; no pulse is restored after reboot, Wi-Fi
  reconnection or ESP OTA. The Uno stops an active pulse without needing Wi-Fi.
- The panel blocks LED and roster controls while an output is enabled. Background
  status polls can observe the pulse but never start or renew one.

The assembly still needs calibrated overload behavior, physical DCC waveform
checks under load, startup stop packets and validated continuous gating before using
trains. Do not connect a logic analyzer directly to powered bridge outputs.

## Observations, 2026-11-03

- The user measured about +15 V at the PD output and at the shield VIN/GND.
- VIN initially had continuity to the Arduino header. The VIN CONNECT link was
  opened; the user confirmed VIN isolation and retained ground continuity.
- With 15 V present and both outputs disabled, the ESP/Uno link remained online,
  A0/A1 read zero in the captured status, and no DCC deadline/CRC faults appeared.
- Uno 0.3.0 compiled to 7,788 bytes flash / 576 bytes static RAM; ESP 0.3.0 to
  393,920 bytes / 30,940 bytes static RAM.
- With motor VIN disconnected, USB telemetry observed a forward pulse stop after
  3.025 seconds. A duplicate pulse did not extend/reverse it. Reverse direction
  and manual Stop passed; B stayed LOW and the DCC packet counter advanced.
- The user also observed the three-second DC pulse on a battery-powered scope.
- Native tests cover deadline expiry, no extension, stop, raw cutoff, signal and
  guard faults, millis rollover and the interrupt guard countdown. Browser checks
  cover capability detection, both pulse requests, Stop, completion, fault display,
  roster lock and mobile/light/dark layouts.

- With isolated 15 V VIN reconnected and only the meter across A+/A−, the user
  measured about **+15 V during A +** and **−15 V during A −**. This confirms
  bridge-A output polarity at no load. Afterward both enables read LOW, with no
  reported deadline or CRC faults.

Loaded sensing/calibration, fault response under real current, thermal limits,
powered DCC waveform quality and locomotive operation remain unvalidated.
