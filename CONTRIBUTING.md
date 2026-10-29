# Contributing

Keep the firmware stage explicit: GP2 is a logic output, not powered track.
Power-stage changes need default-off behavior, startup stop packets, trip
handling and OTA disarm before a hardware release. Mark proposed versus
measured wiring accurately.

Edit `web/`, run `tools/embed_web.py`, then the host and relevant browser checks.
Packet/PIO changes also need external timing/byte captures. Use the core and
flash layout documented in README.md.

Keep credentials, keys, build files and local captures out of commits. Use a
synthetic SSID in previews. Label measurements with instrument, sample rate
and whether physical or simulated.

Project contributions are provided under the [MIT license](LICENSE).
Third-party tools and referenced code retain their own licenses.
