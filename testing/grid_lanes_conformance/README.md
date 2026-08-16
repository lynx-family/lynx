# Grid Lanes Differential Conformance

This Ring 1 harness renders hand-mirrored fixtures in native Lynx, Chromium,
and WebKit, then compares their box-model geometry numerically.

## Run

```sh
testing/grid_lanes_conformance/run.sh
```

This command builds the native subject, runs Ring 0, installs the browser
oracles, builds all fixtures, gates on calibration, and scores the seed corpus.
It writes `out/grid-lanes-conformance/results.json`, prints a human
scoreboard, and appends the same table to `$GITHUB_STEP_SUMMARY` when set.
Calibration cases are a hard gate and must pass both browser oracles.
Grid-lanes cases are expected to remain red until the implementation
milestones land, but each case must render successfully in every runtime.

## Fixture format

Each directory under `fixtures/` contains:

- `fixture.json`: suite, viewport, expected status, and any oracle
  disagreement annotation.
- `subjectOmissions`: a temporary, explicit pre-M1 omission when the current
  Lynx encoder rejects a new property before a bundle can run. The canonical
  property remains in `oracle.html`.
- `index.tsx` and `index.css`: the ReactLynx subject, compiled to
  `dist/main.lynx.bundle`.
- `oracle.html`: a hand-mirrored plain HTML/CSS oracle.

Every compared element has a stable `data-test-tag` in HTML and
`lynx-test-tag` in Lynx. Fixtures explicitly normalize the viewport, body
margin, `box-sizing`, flex direction, and intrinsic minimum sizes to avoid
known Lynx/web default-layout differences.

## Result policy

Geometry is rounded to two decimal places with negative zero normalized to
zero, matching `LayoutTreeTestBench::RoundToLayoutAccuracy`. Quads are compared
with a `0.01` epsilon. Chromium and WebKit are cross-checked before native Lynx
is scored; a fixture must include `oracleDisagreement` metadata if the two
browser results differ.

Use `--suite calibration` or `--suite grid-lanes` for a focused run. The
grid-lanes seed corpus adapts the core WPT masonry scenarios to the current
CSS Grid Level 3 names: fixed lanes, auto-fill, spans, flow tolerance, gaps,
alignment, and RTL.
