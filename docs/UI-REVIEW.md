# Web UI reliability review · 6 November 2026

Reviewed the ESP13 web panel at `cc1aa87` (0.6.1), including its request handling,
status polling, card rendering, modal editor and floating power controls. The
fixes shipped in ESP 0.6.2. This report records that release. See the
[full review](CODE-REVIEW.md) for later finder and OTA fixes.

## Findings and fixes

| Priority | Reproduction / observed defect | Correction |
| --- | --- | --- |
| P1 | Hold a function POST response open: its busy flag never clears, leaving controls locked until reload. Other mutation paths have the same unbounded fetch. | Five-second deadlines for ordinary mutations, error feedback, cleanup and a fresh status read; no automatic command retry. |
| P2 | Hold address-search start, then send a loco/diagnostic command: both proceed and can replace the shared state out of order. Polls also run through power transitions. | One shared busy guard, immediate control locks, poll cancellation/revision checks, and stale reply rejection. Power off can still preempt other work. |
| P2 | Delay the status read after Power on/off: the panel briefly renders the old power state and can offer a duplicate start. | Reflect only the server's checked power acknowledgment, then reconcile with fresh status. No optimistic change on click. |
| P2 | Save a favorite, then type a new name while its follow-up poll is held: late save completion replaces the new draft. | Finish save callbacks at acknowledgment, separate background polling, and preserve newer edits with a local draft revision. Selection/name editing remains available during a function save. |
| P2 | Use a valid 24-character unbroken F0 label at 320 px: the paired Off/On buttons extended beyond the card and are clipped. Native Tab can also leave a focused input behind the fixed power bar. | Wrap the label, prevent button-pair shrink, and reserve scroll clearance for the bar. |
| P2 | Open the loco editor, then disconnect or enable track output from another page: Save remains enabled, either doing nothing or receiving a server rejection. | Guard Save/Remove using current connection/output/search/storage state, explain the reason and preserve the draft/Cancel action. |

Source: [web panel](../firmware/esp13-ota/web/index.html).

## Verification

The new mocked-browser regressions were demonstrated to fail on the old behavior:

- [Request/state regressions](../tools/ui-tests/check-ui-reliability.cjs): stalled
  requests, finder exclusion, power polling/confirmation, and edits during both
  a favorite POST and its follow-up status poll.
- [Layout/editor regressions](../tools/ui-tests/check-ui-layout.cjs): clipped
  light controls, native keyboard focus above the power bar in light/dark at
  320 px, editor connection/power/search/storage changes and draft preservation.

The existing suites additionally cover held mouse targets, heartbeat retries,
session expiry, stale Stop tokens, explicit power, OTA, finder flow, themes and
function favorites. Mock tests exercise delayed/lost responses without powering
hardware; they do not prove every network/browser behavior.

An aborted/timed-out POST can already have taken effect on the ESP. The UI reports
uncertainty and refreshes status instead of blindly retrying a command. Its busy
state is released, while firmware output leases remain independent. Physical
function activation is not part of this web review.

An already-open tab must be reloaded after an OTA update to use the new page
code. The firmware-version badge alone does not replace the JavaScript loaded
in that tab. Live validation opens a fresh page.
