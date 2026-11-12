# Experimental Pico 2 W design

Status is EXPERIMENTAL / DESIGN ONLY.

Read PROJECT.md before continuing work here. README.md, bom.csv, pinmap.csv
and architecture.svg are supporting planning artifacts.

Keep experiment labels visible on new documents, diagrams, CAD and firmware
artifacts. Record provisional values and validation status explicitly.
Use results measured on this hardware when making claims about its operation.

Follow the supported-baseline and promotion boundaries in the repository's
root AGENTS.md. Continue user-requested development while retaining the
experimental designation until the user explicitly changes that status.

Schematic/PCB workflow (D18): the assistant captures schematic sections for
user review. The user owns PCB placement and routing; the assistant reviews
the resulting layout. Preserve user PCB edits and record review status.
Current schematic status is documented in kicad/README.md and kicad/review/.

Drawing convention: wire internal connections directly. Reserve named signal
labels for cross-section interfaces and add a nearby explanation of destination,
voltage or active level as appropriate. Standard common-ground symbols are fine.
Do not commit or push until the user explicitly requests it.

Datasheet convention: keep locally accessible English-language PDFs and link
to those directly. Do not substitute a Chinese-only document for review.

Hide the Value text on ground symbol instances; the ground symbol alone is
sufficient. Preserve the GND net name and connectivity.
