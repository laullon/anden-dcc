# anden-dcc · Pico 2 W project record

Last updated 12 November 2026, Europe/Madrid.

This is the central record for the custom cab PCB exploration. Keep accepted
requirements, engineering proposals, assumptions, open questions and validation
results here. Supporting drawings and tables are linked below.

## Current state

The project has a design brief, functional diagram, preliminary component list
and proposed GPIO allocation. Detailed electrical design, KiCad schematic,
verified footprints and PCB routing are pending. Component ratings and package
candidates in this record require circuit and layout review before an order.

Work is isolated on branch `hardware/pico2w-reva`, based on the tested Uno
baseline at `f3f9ec8`. The first hardware planning commit is `645c72a`.
The project name remains anden-dcc.

## Purpose and inherited requirements

Build a compact controller for an H0 railway diorama with two short,
two-rail tracks. The intended load is up to two locomotives and some rolling
stock. The decoders previously worked with a Roco Lokmaus 2.

The controller provides lights, sound, decoder functions and limited movement.
Local speed adjustment and stopping are priorities. The cab contains a rotary
encoder with a push button, a separate movement STOP button, an OLED and a
movement LED. Wi-Fi provides configuration, the web panel and firmware updates.

Eight saved locos, F0–F28, short addresses 1–127, named favorites and saved
presence selection are features of the existing software to carry into the
new design. Final Pico implementation and storage allocation are still to be
specified.

Decoder programming was explicitly deferred. The existing address finder uses
a user-supplied address range, F0 on one half/off on the other and repeated
user answers to narrow the range. The eventual firmware port should preserve
this workflow.

## Confirmed choices

These entries record user choices. The electrical proposals in the following
sections have a separate status.

| ID | Choice | Reason or context |
| --- | --- | --- |
| D01 | Explore a custom PCB integrating the controller, power stage and cab | More integration and headroom than the Uno/shield arrangement |
| D02 | Use a Pico 2 W as the controller assembly | The processor, flash and radio are provided by the Pico board |
| D03 | Use individual components for PD, regulation, driver and support circuits | Prebuilt power and driver modules were rejected for this PCB |
| D04 | Make assembly practical both by hand and at a factory | The user enjoys soldering and wants an accessible design |
| D05 | Use through-hole parts wherever practical and accept SMD mainly in the power section | A fully through-hole build was relaxed because the selected power ICs use SMD packages |
| D06 | Connect the Pico through through-hole sockets | Agreed mounting approach for the retained controller module |
| D07 | Keep the encoder for speed and use its button to change loco | This replaced the earlier short-press stop/long-press select arrangement |
| D08 | Use a separate button to stop all movement | Normal movement stop preserves rail power for lights and sound |
| D09 | Call presence selection On track in the UI | This replaced the earlier On diorama wording |
| D10 | Put the presence toggle directly on each loco card | Selection should save without opening Edit |
| D11 | Disable direction and function controls on unmarked cards | On track and Edit remain accessible when the normal edit guards permit |
| D12 | Show name, speed and an improved gauge on the OLED | Address, power and STEP/STOP/MOVE labels were removed from the current display |
| D13 | Keep this exploration separate from the established project | The new hardware work uses its own Git branch and worktree |
| D14 | Keep the existing project open and permissively licensed | The repository uses MIT and retains its magic-smoke notice |
| D15 | Keep setup and control free of application passwords | This is the earlier user preference for the local project |

Future changes should retain these IDs. Mark a changed decision as superseded
and identify its replacement, so the reason for an old design remains visible.

## Current engineering direction

| ID | Direction | Status |
| --- | --- | --- |
| P01 | PIO generates DCC edges while software supplies complete buffered packets | Proposed implementation direction enabled by the accepted Pico platform |
| P02 | Wi-Fi, web, cab logic and firmware updates run on the Pico 2 W | Intended architecture, firmware port pending |
| P03 | One DCC bridge feeds both tracks in parallel | Carried-forward baseline and current design assumption |
| P04 | USB-C PD requests 15 V and a buck regulator produces the logic supply | Current power plan |
| P05 | The 5 V regulator and programming USB feed Pico VSYS through source isolation | Circuit selection and reverse-current verification pending |
| P06 | Bridge startup defaults off with pull-downs and guarded enable | Detailed reset, wake and packet-boundary timing pending |
| P07 | An external watchdog gates bridge enable using application and DCC health | Timeout, output polarity, latch and re-arm behavior pending |
| P08 | Use current and rail-voltage sensing with protected ADC inputs | Reference values, gain, filtering and clamps pending |
| P09 | Prefer 3.3 V peripheral interfaces and driver inputs | Existing OLED pull-up and supply voltages still need checking |
| P10 | Retain the existing SSD1315 OLED assembly through a connector | Mechanical and electrical compatibility assumption |
| P11 | Use top-side SMD population followed by through-hole assembly | Current assembly plan |
| P12 | Use a two-layer, 1.6 mm FR-4 board as the starting target | Copper weight, dimensions and thermal routing pending |

P01 includes defining PIO FIFO underrun behavior, DMA/buffering, resource
allocation alongside the wireless driver and a fault path that removes track
power. More CPU or RAM alone does not establish correct DCC timing.

D08 is a motion command handled locally. P06/P07 cover electrical fault and
controller-failure shutdown. Their behavior must be explicit in the circuit
and firmware.

## Design assumptions and limits

| Item | Current assumption | What must be established |
| --- | --- | --- |
| Input source | Charger offering 15 V / 3 A | Negotiation, fallback, inrush and startup behavior |
| Track current | 2 A combined across both connectors | Actual loads, current limit, thermal performance and connector ratings |
| Track topology | Shared DCC district | Confirm whether independent track power is needed |
| Logic rail | Regulated 5 V feeding VSYS through isolation | Tolerance, USB coexistence and transient behavior |
| GPIO logic | 3.3 V | Check every external pull-up and analog input path |
| OLED | Existing 0.96-inch, 128×64, yellow/blue SSD1315, presently addressed at 0x3C | Supply tolerance, pin order, mounting and I²C pull-up voltage |
| Display labels | First 16 characters folded to uppercase ASCII | Preserve full saved UTF-8 names when porting storage |
| Mechanical parts | Existing encoder, button and OLED | Obtain dimensions or exact part drawings before footprint placement |

The 2 A target was introduced as an engineering assumption. It is separate
from the driver's advertised peak current. The final board current rating
must follow measurements and thermal calculations.

The current firmware caps movement at three crawl steps and 800 ms per move.
The user asked why the speed was capped. Configurable per-loco speed limits
were mentioned as a possible change, but that feature was not requested for
implementation. Treat those values as the existing baseline, with any future
change recorded explicitly.

Standalone local operation remains a design requirement to resolve fully.
In particular, decide how local track power is armed and how the existing
browser lease policy translates to a controller with its own cab.

## Component shortlist

All power components below are candidates for circuit capture. Exact parts,
values, packages and availability need a final check before the manufacturing
BOM is released.

| Function | Current candidate | Package / important detail |
| --- | --- | --- |
| Controller | Pico 2 W | Two 1×20, 2.54 mm sockets, accessible USB/BOOTSEL and SWD |
| PD controller | CH224K | ESSOP-10 with underside ground pad, hardware 15 V request configuration |
| 5 V regulator | LMR51420XDDCR | SOT-23-THIN-6, regulator support network required |
| Track bridge | DRV8874PWPR | HTSSOP-16 with underside thermal pad, 3.3 V control and current sense |
| Watchdog | TPS3435, leaded variant | SOT-23-THIN-8, full order code and timeout pending |
| Enable gate | SN74LVC1G08DBVR | SOT-23-5, 3.3 V logic |
| USB-C inlet | GCT USB4750-GF-C | Six-contact through-hole, vertical-entry candidate |
| Conditional OLED translator | PCA9306 leaded variant | Populate only if the actual OLED needs a 5 V I²C bus |

The initial suggestion used a TPS3431 watchdog. The planning shortlist moved
to a leaded TPS3435 variant to support hand assembly. This is an engineering
candidate change, with the actual order code still open.

The USB-C connector is a candidate rather than a locked mechanical decision.
A side-entry enclosure would require a different verified part and footprint.

The [BOM](bom.csv) includes protection, regulator/driver support parts,
source isolation, ADC protection, cab wiring and test/debug connections.
Grouped entries and TBD values make it a planning list rather than a
factory purchasing BOM.

Manufacturer references are collected in the
[design brief](README.md#package-choices) and the source column of the BOM.

## Hand and factory assembly

Keep one SMD population side and use standard manufacturer land patterns.
Use accessible lead toes and rework spacing where the electrical layout
permits. Keep switching loops and local bypass capacitors close to their ICs.

Prefer through-hole controls, connectors, sockets, general resistors and bulk
capacitors. Use 1206 passives where practical and 0805 for local decoupling
where necessary. The SMD power section also includes its inductor and support
parts. Some watchdog, gate or translator parts may be SMD.

The CH224K and DRV8874 have underside pads. Their assembly requires solder
paste and reflow heat from a suitable hot plate/hot-air process or a factory.
Visible leads can be inspected and reworked with an iron.

Populate and inspect the SMD section first, then through-hole parts, then fit
the Pico and OLED. Provide clear orientation, polarity and connector markings.
Protect the Pico antenna keep-out and retain access to programming and debug
interfaces.

Factory release needs verified footprints, a flat BOM with exact MPNs and
reference designators, centroid data, fabrication/stencil files, assembly
drawings, a programming procedure and acceptance tests. Quote SMD and
through-hole operations separately.

## Prototype history and evidence

The last verified software baseline is Uno 0.7.6 with ESP13 0.7.4. The platform
is an Uno R3 at 16 MHz, a Deek-Robot L298P Rev3-style shield and a DOIT ESP13.
The motor shield VIN CONNECT link was opened and its isolation checked.
USB-C PD at 15 V and an MP1584 were explored for the existing cab power plan.

The prototype exposed a DCC timing problem while using interrupt-driven OLED
I²C. Uno 0.7.5 changed the display transport to polling and preserved the
strict DCC deadline checks. A 120-second USB-only run with 132 label redraws
and 21 output-enable cycles recorded zero timing faults. Six further web
power/heartbeat cycles also passed. The minimal display layout in Uno 0.7.6
was uploaded and visually confirmed afterward.

Uno 0.7.6 uses 17,296 bytes of flash and 1,283 bytes of static RAM, leaving
765 bytes for stack/local variables. That remainder is not a measured
worst-case runtime stack margin. CPU utilization was not measured.

The 12 November session also sampled roughly 36 KiB of ESP free heap,
414,352 bytes of application image against a 458,752-byte OTA capacity and
about 133 DCC packets/second with two locos. These are historical observations,
not live readings or measurements of Pico firmware.

The Uno prototype remains evidence for cab behavior and DCC packet logic.
Its tests do not qualify the new power circuit or prove Pico firmware timing.
Detailed records are in [validation](../../docs/VALIDATION.md), with the current
cab behavior documented in [CAB.md](../../firmware/uno-stack-test/CAB.md).

## Alternatives considered

| Alternative | Outcome |
| --- | --- |
| Continue the Uno + motor shield + ESP13 stack as the only hardware | Retained as the tested baseline while exploring the custom PCB |
| Pico W | Considered before choosing Pico 2 W for the new board |
| Additional ESP13 or ESP32 for networking | Networking is assigned to the selected Pico 2 W architecture |
| Bare RP2350 and a custom radio section | The user chose the Pico 2 W module |
| Prebuilt PD, buck and H-bridge modules | Explicitly rejected for the new power circuitry |
| Entirely through-hole construction | Relaxed to a mixed design after package constraints were discussed |
| Factory assembly of every component | Both hand and factory assembly remain supported goals |
| Decoder programming circuit | Deferred in favor of the visual address-finding workflow |

## Open work

| ID | Open item | Completion evidence |
| --- | --- | --- |
| O01 | Power and current budget | Measured locomotive loads and calculated total rail budget |
| O02 | Driver current limit and thermal design | Calculations, PCB copper design and load/temperature tests |
| O03 | PD configuration, protection and inrush | Verified schematic plus attach/fallback and transient tests |
| O04 | Buck values and source isolation | Regulation/load tests and measured reverse-current behavior |
| O05 | Watchdog order code and recovery | Defined timeout and startup/failure/re-arm tests |
| O06 | OLED electrical compatibility | Supply and pull-up measurements with documented connector pin order |
| O07 | Cab mechanics and enclosure | Exact encoder/button/OLED dimensions and connector orientation |
| O08 | Pico firmware architecture | PIO/DMA allocation, underrun policy, local stop and update recovery design |
| O09 | Standalone local power control | Defined arming and network-loss behavior |
| O10 | GPIO allocation | Review of [pinmap.csv](pinmap.csv) against the completed circuit |
| O11 | Electrical schematic | Captured circuit with ERC and reviewed symbol pin assignments |
| O12 | PCB layout and assembly | Verified footprints, DRC, thermal/antenna review and assembly outputs |
| O13 | Prototype bring-up | Logic, PD, unloaded DCC, faults, loaded heating and stopping-distance tests |

These are engineering tasks, not settled requirements. Record new findings and
their sources here before promoting a candidate into the released BOM.

## Working files and maintenance

- [Project record](PROJECT.md) holds decisions, assumptions, history and open work.
- [Design brief](README.md) holds the current technical and assembly direction.
- [Functional architecture](architecture.svg) illustrates the blocks and assembly sequence.
- [Preliminary BOM](bom.csv) holds component candidates and unresolved values.
- [Proposed GPIO allocation](pinmap.csv) holds signal names and physical pin choices.
- [Repository license](../../LICENSE) is MIT.
- [Project README](../../README.md) contains the existing magic-smoke notice.

After each meaningful decision or hardware result, update this record and any
affected supporting file in the same change. Include the date, decision ID,
status and reason. Keep measured results separate from calculated targets.
Preserve superseded choices in the history and avoid silently changing a
previously confirmed requirement.

### Record history

| Date | Update |
| --- | --- |
| 12 November 2026 | Created the central record from this exploration and the existing validated prototype. Classified confirmed decisions, engineering proposals and unresolved assumptions. |
