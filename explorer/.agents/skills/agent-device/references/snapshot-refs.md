# Snapshot Refs and Selectors (Mobile)

## Purpose

Refs are useful for discovery/debugging. For deterministic scripts, use selectors.
For tap interactions, `press` is canonical; `click` is an equivalent alias.

## Snapshot

```bash
agent-device snapshot -i
```

Output:

```
Page: com.apple.Preferences
App: com.apple.Preferences

@e1 [ioscontentgroup]
  @e2 [button] "Camera"
  @e3 [button] "Privacy & Security"
```

## Using refs (discovery/debug)

```bash
agent-device press @e2
agent-device fill @e5 "test"
```

## Using selectors (deterministic)

```bash
agent-device press 'id="camera_row" || label="Camera" role=button'
agent-device fill 'id="search_input" editable=true' "test"
agent-device is visible 'id="camera_settings_anchor"'
```

## Ref lifecycle

Refs can become invalid when UI changes (navigation, modal, dynamic list updates).
Re-snapshot after transitions if you keep using refs.

## Scope snapshots

Use `-s` to scope to labels/identifiers. This reduces size and speeds up results:

```bash
agent-device snapshot -i -s "Camera"
agent-device snapshot -i -s @e3
```

## Diff snapshots (structural)

Use `diff snapshot` when you need compact state-change visibility between nearby UI states:

```bash
agent-device snapshot -i           # First snapshot initializes baseline
agent-device press @e5
agent-device diff snapshot -i           # Shows +/− structural lines vs prior snapshot
```

Efficient pattern:
- Initialize once at a stable point.
- Mutate UI (`press`, `fill`, `swipe`).
- Run `diff snapshot` after interactions to confirm expected change shape with bounded output.
- Re-run full/scoped `snapshot` only when you need fresh refs for next step selection.

## Troubleshooting

- Ref not found: re-snapshot.
- If XCTest returns 0 nodes, foreground app state may have changed. Re-open the app or retry after state is stable.

## Stop Conditions

- If refs are unstable after UI transitions, switch to selector-based targeting and stop investing in ref-only flows.

## Replay note

- Prefer selector-based actions in recorded `.ad` replays.
- Use `agent-device replay -u <path>` to update selector drift and rewrite replay scripts in place.
