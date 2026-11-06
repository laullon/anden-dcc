# Contributing

The supported hardware is an Arduino Uno R3, a Deek-Robot L298P motor shield and
a DOIT ESP13 shield. Keep default-off output behavior, startup zero-speed packets,
current/timing trips and independent control timeouts intact.

Edit `firmware/esp13-ota/web/index.html`, run
`python3 firmware/esp13-ota/embed.py`, then the relevant native and browser checks.
`tools/build.sh` builds both boards and checks the generated page. Use the core
versions and flash layout in README.md. DCC changes also need live UART stress
and external timing/byte captures; distinguish modeled from measured results.

Keep credentials, build files and private captures out of commits. Use synthetic
Wi-Fi details in previews. Label measurements with instrument, sample rate and
whether the motor supply was connected. Disconnect motor VIN and tracks for
uploads and diagnostics.

Contributions use the [MIT license](LICENSE). Third-party tools retain their own licenses.

## C++ boundaries

Keep module interfaces in headers and implementations in `.cpp` files. Put
mutable runtime state under one owner, pass dependencies explicitly, and avoid
implementation includes or sketch-global dependencies. Small inline helpers
are appropriate where their cost/role warrants it. See
[code structure and validation](docs/CODE-STRUCTURE.md).

The native checks must link production translation units. Keep public headers
self-contained; run `python3 tests/check_cpp_interfaces.py` after changing them.
Use `.clang-format` for C++ sources, excluding generated `SetupPage.h/.cpp`.
