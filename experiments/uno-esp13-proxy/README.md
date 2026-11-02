# Stacked Uno / ESP13 software relay experiment

This exploratory diagnostic tested whether a single-UART Uno could relay ESP ROM
programming traffic while leaving the ESP13 shield stacked. **It did not establish
a usable programming connection. Do not use it to flash an ESP.**

The Uno buffers ESP traffic instead of echoing it continuously. Host requests and
reports use ASCII hex with a sequence number and CRC32, so reports do not contain
ESP SLIP delimiters. The host echo operation does not send anything to the ESP;
the supplied probe subsequently sends only a read-only ROM SYNC request.

## Physical results, 2026-11-02

- The Uno sketch compiled with AVR core 1.8.7: 3,890 bytes flash, 871 bytes RAM.
- Both switches OFF: checked Mac → Uno → Mac transfers passed at 32, 64, 128,
  256, 512 and 640 bytes.
- Both switches ON, after the user entered ESP KEY/RST bootloader mode: the Uno
  still sent its startup report, but no checked host echo response was received.
- No ESP erase or flash-write command was sent. The ESP's original firmware is
  unchanged. The Uno now runs this diagnostic instead of the earlier ROM probe.

The result isolates a limitation on host input when the ESP TX connection joins
the shared Uno RX / USB TX line. It does not contradict the earlier successful
Uno-generated ESP ROM SYNC test; that test did not require host packets to arrive
on the shared input at the same time.

The probe explicitly toggles DTR/RTS and allows the Uno to settle before testing;
opening the CH340 port alone was inconsistent during initial tests.

## Reproduce

Keep motor/track power disconnected. The sketch holds standard L298P enable pins
D3 and D11 LOW while running, but cannot drive them during reset or upload.
Turn both shield switches OFF for an Uno upload:

```sh
arduino-cli compile --fqbn arduino:avr:uno \
  --build-path /tmp/anden-uno-proxy-build experiments/uno-esp13-proxy
arduino-cli upload --fqbn arduino:avr:uno --port /dev/cu.usbserial-110 \
  --input-dir /tmp/anden-uno-proxy-build
```

The Python probe needs `pyserial`. Change `--port` to the confirmed Uno port. With
the switches OFF it can test only the host echo; with both ON it also attempts the
ESP SYNC if host echo passes. Enter the ESP bootloader with KEY held while pressing
and releasing its own RST, then release KEY after one second.

```sh
python3 experiments/uno-esp13-proxy/probe.py --port /dev/cu.usbserial-110
```

Press ESP13 RST with KEY released to leave the bootloader afterward. The direct
[Uno-only serial crossover](../esp13-ota/README.md#using-only-the-uno-as-the-serial-adapter)
is an alternative first-installation arrangement; it still requires temporary
physical wiring and has not been tested on this setup.
