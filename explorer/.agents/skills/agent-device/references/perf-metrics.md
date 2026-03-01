# Performance Metrics (`perf` / `metrics`)

Use this reference when you need to measure launch performance in agent workflows.

## Quick flow

```bash
agent-device open Settings --platform ios
agent-device perf --json
```

Alias:

```bash
agent-device metrics --json
```

## What is measured today

- Session-scoped `startup` timing only.
- Sampling method: `open-command-roundtrip`.
- Unit: milliseconds (`ms`).
- Source: elapsed wall-clock time around each session `open` command dispatch for the active app target.

## Output fields to use

- `metrics.startup.lastDurationMs`: most recent startup sample.
- `metrics.startup.lastMeasuredAt`: ISO timestamp of most recent sample.
- `metrics.startup.sampleCount`: number of retained samples.
- `metrics.startup.samples[]`: recent startup history for the current session.
- `sampling.startup.method`: current sampling method identifier.

## Platform support (current)

- iOS simulator: supported for startup sampling.
- iOS physical device: supported for startup sampling.
- Android emulator/device: supported for startup sampling.
- `fps`, `memory`, and `cpu`: currently placeholders (`available: false`).

## Interpretation guidance

- Treat startup values as command round-trip timing, not true app first-frame or first-interactive telemetry.
- Compare like-for-like runs:
  - same device target
  - same app build
  - same workflow/session steps
- Use multiple runs and compare trend/median, not one-off samples.

## Common pitfalls

- Running `perf` before any `open` in the session yields no startup sample yet.
- Comparing values across different devices/runtimes introduces large noise.
- Interpreting current `startup` as CPU/FPS/memory would be incorrect.
