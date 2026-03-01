---
name: agent-device
description: Automates interactions for iOS simulators/devices and Android emulators/devices. Use when navigating apps, taking snapshots/screenshots, tapping, typing, scrolling, or extracting UI info on mobile targets.
---

# Mobile Automation with agent-device

For exploration, use snapshot refs. For deterministic replay, use selectors.
For structured exploratory QA bug hunts and reporting, use [../dogfood/SKILL.md](../dogfood/SKILL.md).

## Start Here (Read This First)

Use this skill as a router, not a full manual.

1. Pick one mode:
   - Normal interaction flow
   - Debug/crash flow
   - Replay maintenance flow
2. Run one canonical flow below.
3. Open references only if blocked.

## Decision Map

- No target context yet: `devices` -> pick target -> `open`.
- Normal UI task: `open` -> `snapshot -i` -> `press/fill` -> `diff snapshot -i` -> `close`
- Debug/crash: `open <app>` -> `logs clear --restart` -> reproduce -> `network dump` -> `logs path` -> targeted `grep`
- Replay drift: `replay -u <path>` -> verify updated selectors
- Remote multi-tenant run: allocate lease -> run commands with tenant isolation flags -> heartbeat/release lease
- Device-scope isolation run: set iOS simulator set / Android allowlist -> run selectors within scope only

## Canonical Flows

### 1) Normal Interaction Flow

```bash
agent-device open Settings --platform ios
agent-device snapshot -i
agent-device press @e3
agent-device diff snapshot -i
agent-device fill @e5 "test"
agent-device close
```

### 2) Debug/Crash Flow

```bash
agent-device open MyApp --platform ios
agent-device logs clear --restart
agent-device network dump 25
agent-device logs path
```

Logging is off by default. Enable only for debugging windows.
`logs clear --restart` requires an active app session (`open <app>` first).

### 3) Replay Maintenance Flow

```bash
agent-device replay -u ./session.ad
```

### 4) Remote Tenant Lease Flow (HTTP JSON-RPC)

```bash
# Allocate lease
curl -sS http://127.0.0.1:${AGENT_DEVICE_DAEMON_HTTP_PORT}/rpc \
  -H "content-type: application/json" \
  -H "Authorization: Bearer <token>" \
  -d '{"jsonrpc":"2.0","id":"alloc-1","method":"agent_device.lease.allocate","params":{"runId":"run-123","tenantId":"acme","ttlMs":60000}}'

# Use lease in tenant-isolated command execution
agent-device --daemon-transport http \
  --tenant acme \
  --session-isolation tenant \
  --run-id run-123 \
  --lease-id <lease-id> \
  session list --json

# Heartbeat and release
curl -sS http://127.0.0.1:${AGENT_DEVICE_DAEMON_HTTP_PORT}/rpc \
  -H "content-type: application/json" \
  -H "Authorization: Bearer <token>" \
  -d '{"jsonrpc":"2.0","id":"hb-1","method":"agent_device.lease.heartbeat","params":{"leaseId":"<lease-id>","ttlMs":60000}}'
curl -sS http://127.0.0.1:${AGENT_DEVICE_DAEMON_HTTP_PORT}/rpc \
  -H "content-type: application/json" \
  -H "Authorization: Bearer <token>" \
  -d '{"jsonrpc":"2.0","id":"rel-1","method":"agent_device.lease.release","params":{"leaseId":"<lease-id>"}}'
```

## Command Skeleton (Minimal)

### Session and navigation

```bash
agent-device devices
agent-device devices --platform ios --ios-simulator-device-set /tmp/tenant-a/simulators
agent-device devices --platform android --android-device-allowlist emulator-5554,device-1234
agent-device open [app|url] [url]
agent-device open [app] --relaunch
agent-device close [app]
agent-device session list
```

Use `boot` only as fallback when `open` cannot find/connect to a ready target.
For Android emulators by AVD name, use `boot --platform android --device <avd-name>`.
For Android emulators without GUI, add `--headless`.
Use `--target mobile|tv` with `--platform` (required) to pick phone/tablet vs TV targets (AndroidTV/tvOS).

Isolation scoping quick reference:
- `--ios-simulator-device-set <path>` scopes iOS simulator discovery + command execution to one simulator set.
- `--android-device-allowlist <serials>` scopes Android discovery/selection to comma/space separated serials.
- Scope is applied before selectors (`--device`, `--udid`, `--serial`); out-of-scope selectors fail with `DEVICE_NOT_FOUND`.
- With iOS simulator-set scope enabled, iOS physical devices are not enumerated.

TV quick reference:
- AndroidTV: `open`/`apps` use TV launcher discovery automatically.
- TV target selection works on emulators/simulators and connected physical devices (AndroidTV + AppleTV).
- tvOS: runner-driven interactions and snapshots are supported (`snapshot`, `wait`, `press`, `fill`, `get`, `scroll`, `back`, `home`, `app-switcher`, `record` and related selector flows).
- tvOS `back`/`home`/`app-switcher` map to Siri Remote actions (`menu`, `home`, double-home) in the runner.
- tvOS follows iOS simulator-only command semantics for helpers like `pinch`, `settings`, and `push`.

### Snapshot and targeting

```bash
agent-device snapshot -i
agent-device diff snapshot -i
agent-device find "Sign In" click
agent-device press @e1
agent-device fill @e2 "text"
agent-device is visible 'id="anchor"'
```

`press` is canonical tap command; `click` is an alias.

### Utilities

```bash
agent-device appstate
agent-device clipboard read
agent-device clipboard write "token"
agent-device keyboard status
agent-device keyboard dismiss
agent-device perf --json
agent-device network dump [limit] [summary|headers|body|all]
agent-device push <bundle|package> <payload.json|inline-json>
agent-device trigger-app-event screenshot_taken '{"source":"qa"}'
agent-device get text @e1
agent-device screenshot out.png
agent-device settings permission grant notifications
agent-device settings permission reset camera
agent-device trace start
agent-device trace stop ./trace.log
```

### Batch (when sequence is already known)

```bash
agent-device batch --steps-file /tmp/batch-steps.json --json
```

### Performance Check

- Use `agent-device perf --json` (or `metrics --json`) after `open`.
- For detailed metric semantics, caveats, and interpretation guidance, see [references/perf-metrics.md](references/perf-metrics.md).

## Guardrails (High Value Only)

- Re-snapshot after UI mutations (navigation/modal/list changes).
- Prefer `snapshot -i`; scope/depth only when needed.
- Use refs for discovery, selectors for replay/assertions.
- Use `fill` for clear-then-type semantics; use `type` for focused append typing.
- iOS `appstate` is session-scoped; Android `appstate` is live foreground state.
- Clipboard helpers: `clipboard read` / `clipboard write <text>` are supported on Android and iOS simulators; iOS physical devices are not supported yet.
- Android keyboard helpers: `keyboard status|get|dismiss` report keyboard visibility/type and dismiss via keyevent when visible.
- `network dump` is best-effort and parses HTTP(s) entries from the session app log file.
- Biometric settings: iOS simulator supports `settings faceid|touchid <match|nonmatch|enroll|unenroll>`; Android supports `settings fingerprint <match|nonmatch>` where runtime tooling is available.
- For AndroidTV/tvOS selection, always pair `--target` with `--platform` (`ios`, `android`, or `apple` alias); target-only selection is invalid.
- `push` simulates notification delivery:
  - iOS simulator uses APNs-style payload JSON.
  - Android uses broadcast action + typed extras (string/boolean/number).
- `trigger-app-event` requires app-defined deep-link hooks and URL template configuration (`AGENT_DEVICE_APP_EVENT_URL_TEMPLATE` or platform-specific variants).
- `trigger-app-event` requires an active session or explicit selectors (`--platform`, `--device`, `--udid`, `--serial`); on iOS physical devices, custom-scheme triggers require active app context.
- Canonical trigger behavior and caveats are documented in [`website/docs/docs/commands.md`](../../website/docs/docs/commands.md) under **App event triggers**.
- Permission settings are app-scoped and require an active session app:
  `settings permission <grant|deny|reset> <camera|microphone|photos|contacts|notifications> [full|limited]`
- `full|limited` mode applies only to iOS `photos`; other targets reject mode.
- On Android, non-ASCII `fill/type` may require an ADB keyboard IME on some system images; only install IME APKs from trusted sources and verify checksum/signature.
- If using `--save-script`, prefer explicit path syntax (`--save-script=flow.ad` or `./flow.ad`).
- For tenant-isolated remote runs, always pass `--tenant`, `--session-isolation tenant`, `--run-id`, and `--lease-id` together.
- Use short lease TTLs and heartbeat only while work is active; release leases immediately after run completion/failure.
- Env equivalents for scoped runs: `AGENT_DEVICE_IOS_SIMULATOR_DEVICE_SET` (compat `IOS_SIMULATOR_DEVICE_SET`) and
  `AGENT_DEVICE_ANDROID_DEVICE_ALLOWLIST` (compat `ANDROID_DEVICE_ALLOWLIST`).

## Security and Trust Notes

- Prefer a preinstalled `agent-device` binary over on-demand package execution.
- If install is required, pin an exact version (for example: `npx --yes agent-device@<exact-version> --help`).
- Signing/provisioning environment variables are optional, sensitive, and only for iOS physical-device setup.
- Logs/artifacts are written under `~/.agent-device`; replay scripts write to explicit paths you provide.
- For remote daemon mode, prefer `AGENT_DEVICE_DAEMON_SERVER_MODE=http|dual` with `AGENT_DEVICE_HTTP_AUTH_HOOK` and tenant-scoped lease admission.
- Keep logging off unless debugging and use least-privilege/isolated environments for autonomous runs.

## Common Mistakes

- Mixing debug flow into normal runs (keep logs off unless debugging).
- Continuing to use stale refs after screen transitions.
- Using URL opens with Android `--activity` (unsupported combination).
- Treating `boot` as default first step instead of fallback.

## References

- [references/snapshot-refs.md](references/snapshot-refs.md)
- [references/logs-and-debug.md](references/logs-and-debug.md)
- [references/session-management.md](references/session-management.md)
- [references/permissions.md](references/permissions.md)
- [references/video-recording.md](references/video-recording.md)
- [references/coordinate-system.md](references/coordinate-system.md)
- [references/batching.md](references/batching.md)
- [references/perf-metrics.md](references/perf-metrics.md)
- [references/remote-tenancy.md](references/remote-tenancy.md)
