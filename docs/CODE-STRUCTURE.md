# C++ module structure

The sketches compose the application. Headers declare module interfaces and value
types; implementation files own behavior and runtime state. Headers do not depend
on a particular include order or on globals declared by a sketch.

## ESP13

| Module | Responsibility and ownership |
| --- | --- |
| `esp13-ota.ino` | Wi-Fi/web runtime, server, Uno link and controller instances |
| `RosterControl.h/.cpp` | Roster, favorite metadata, finder, browser lease, storage and associated HTTP routes |
| `UnoLink.h/.cpp` | Checked UART transactions and cached Uno status |
| `CabLabel.h/.cpp` | Bounded display-name folding and hex transport codec |
| `CabControl.h/.cpp` | Local encoder/STOP state and cached eligible loco labels |
| `CabDisplay.h/.cpp` | OLED pixel rendering through bounded I²C transactions |
| `CabI2c.h/.cpp` | Polled AVR TWI transport that leaves Timer1 interrupts available |
| `HttpSupport.h/.cpp` | JSON serialization/replies; server and Uno dependencies are explicit parameters |
| `BrowserLease`, `StatusLed`, `FavoriteFunctions`, `WifiValidation` | Separately compiled policies and models |
| `RosterStorage.h/.cpp` | Mount existing storage; initialize only verified erased flash |
| `FirmwareImage.h/.cpp` | Portable streaming image profile/identity/size/CRC validator |
| `FirmwareUpdate.h/.cpp` | Firmware-only HTTP staging, multipart lifecycle and deferred commit |
| `SetupPage.h/.cpp` | Generated asset declaration and its single flash-resident definition |

`RosterControl` receives references to the server and Uno link. It owns its models
and session state privately; route callbacks capture that controller. It cannot
be copied because the Uno link retains a reference to its roster. The sketch
uses explicit methods for status, route registration, configuration and service.
The controller never relies on sketch globals or an implementation header.

## Uno

`UnoDcc.h` exposes the control/status interface. `UnoDcc.cpp` owns the scheduler,
output guard, flags, fault count and Timer1 ISR in one translation unit. Mutable
state is not exported. The sketch uses operations and read accessors, including
an atomic startup-stop operation, rather than reaching into the scheduler.
`BridgeBench.cpp` implements the output mode/lease policy.

## Shared library

`AndenStackLink.cpp` owns framing, CRC and parsing implementations. Address search,
roster operations, function packets and the packet scheduler each have their own
`.cpp`. Their headers contain types, declarations and constants. Value models
retain fields needed for serialization and packet preparation.

The remaining inline code is small: packet bit packing, scheduler flag operations,
and the interrupt guard's counter operations. Complete packet construction and
the half-bit scheduler are implemented in `.cpp`, not in headers.

## Generated assets, formatting and checks

Install the dependencies from [the main guide](../README.md#checks-and-limits).
Edit `firmware/esp13-ota/web/index.html` or `web/locos/`, then run:

```sh
python3 firmware/esp13-ota/embed.py
tools/build.sh
npm test --prefix tools/ui-tests
```

The embedder generates both `SetupPage.h` and `SetupPage.cpp` and checks that the
inflated payload matches the source. Do not hand-edit or reformat those generated
files. C++ source uses the repository's `.clang-format`.

Native checks compile and link the actual production `.cpp` files. The interface
check compiles each portable public header on its own, then links independent
consumers that include the same headers. The ISR simulator links `UnoDcc.cpp` and
observes output transitions through simulated registers; it does not include an
implementation file or modify private scheduler state.

## Refactor validation at `92667bd`

These measurements describe the 6 November 2026 refactor. See
[Validation](VALIDATION.md) for subsequent builds.

Both target builds and native checks pass. All eight existing browser suites pass;
the web page and compressed payload are byte-for-byte unchanged. UART commands,
packet formats, persisted file formats, timeouts and version identifiers are
unchanged. No hardware was reflashed for this source-only refactor.

With the tested AVR toolchain, the normalized Timer1 ISR has the same 112
instructions and the half-bit scheduler the same 478 instructions as the baseline;
register operands and relative branches match, with only linker addresses ignored.
This is a compiled-code comparison, not a new physical timing measurement.

Uno: 12,814 bytes flash / 1,018 bytes static RAM (baseline 12,812 / 1,018).
ESP: 416,160-byte image / 33,592 bytes static RAM (baseline 414,176 / 33,576).
