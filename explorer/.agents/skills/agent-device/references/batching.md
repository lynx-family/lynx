# Batching

## When to use batch

- The agent already knows a short sequence of commands.
- Steps belong to one logical screen flow.
- You want one result object with per-step timing and failure context.

## When not to use batch

- Flows are unrelated and should be retried independently.
- The workflow is highly dynamic and requires replanning after each step.
- You need human approvals between steps.

## CLI patterns

From file:

```bash
agent-device batch --session sim --platform ios --steps-file /tmp/batch-steps.json --json
```

Inline (small payloads only):

```bash
agent-device batch --steps '[{"command":"open","positionals":["settings"]}]'
```

## Step payload contract

```json
[
  { "command": "open", "positionals": ["settings"], "flags": {} },
  { "command": "wait", "positionals": ["label=\"Privacy & Security\"", "3000"], "flags": {} },
  { "command": "click", "positionals": ["label=\"Privacy & Security\""], "flags": {} },
  { "command": "get", "positionals": ["text", "label=\"Tracking\""], "flags": {} }
]
```

Rules:

- `positionals` optional, defaults to `[]`.
- `flags` optional, defaults to `{}`.
- nested `batch` and `replay` are rejected.
- stop-on-first-error is the supported mode (`--on-error stop`).

## Response handling

Success includes:

- `total`, `executed`, `totalDurationMs`
- `results[]` entries with `step`, `command`, `durationMs`, and optional `data`

Failure includes:

- `details.step`
- `details.command`
- `details.executed`
- `details.partialResults`

Use these fields to replan from the first failing step.

## Common error categories and agent actions

- `INVALID_ARGS`: payload/step shape issue; fix payload and retry.
- `SESSION_NOT_FOUND`: open or select the correct session, then retry.
- `UNSUPPORTED_OPERATION`: switch command/target to supported operation.
- `AMBIGUOUS_MATCH`: refine selector/locator, then retry failed step.
- `COMMAND_FAILED`: add sync guard (`wait`, `is exists`) and retry from failed step.

## Reliability guardrails

- Add sync guards after mutating steps.
- Assume snapshot/ref drift after navigation.
- Keep batch size moderate (about 5-20 steps).
- Split long workflows into phases:
  1. navigate
  2. verify/extract
  3. cleanup
