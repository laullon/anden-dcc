# Project and experiment boundaries

## Supported baseline

The supported anden-dcc hardware is Arduino Uno + Deek-Robot L298P motor
shield + DOIT ESP13. Its build scripts, firmware, wiring and validation are
documented in the root README and docs/VALIDATION.md.

## Pico 2 W experiment

The work under hardware/pico2w-reva is EXPERIMENTAL and DESIGN ONLY.
Its working branch is hardware/pico2w-reva. The central record is
hardware/pico2w-reva/PROJECT.md.

The experiment currently contains a design brief, preliminary BOM, architecture
diagram and proposed GPIO allocation. Electrical schematic, verified footprints,
PCB routing, Pico firmware and prototype validation remain pending.

Choose the platform from the user's task and read the relevant project record
before editing. Keep Pico experimental artifacts and findings clearly labelled
and isolated from supported-baseline files. Existing Uno/ESP13 build and upload
artifacts belong to that baseline.

User-authorized experimental development can proceed with the experimental
status retained. Promotion to supported hardware requires an explicit user
decision and validation evidence for the new hardware itself. Baseline Uno
measurements remain historical evidence for the Uno system.

Update the experiment's project record when decisions, assumptions, artifact
status or measurements change. Preserve the distinction between confirmed
requirements, engineering proposals and validated results.
