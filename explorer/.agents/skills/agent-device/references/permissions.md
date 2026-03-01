# Permissions and Setup

## iOS snapshots

iOS snapshots use XCTest and do not require macOS Accessibility permissions.

## iOS physical device runner

For iOS physical devices, XCTest runner setup requires valid signing/provisioning.
Use Automatic Signing in Xcode, or provide optional overrides:

- `AGENT_DEVICE_IOS_TEAM_ID`
- `AGENT_DEVICE_IOS_SIGNING_IDENTITY`
- `AGENT_DEVICE_IOS_PROVISIONING_PROFILE`
- `AGENT_DEVICE_IOS_BUNDLE_ID` (optional runner bundle-id base override)

Free Apple Developer (Personal Team) accounts may reject generic bundle IDs as unavailable.
Set `AGENT_DEVICE_IOS_BUNDLE_ID` to a unique reverse-DNS identifier when that happens.

Security guidance for these overrides:

- These variables are optional and only needed for physical-device XCTest setup.
- Treat values as sensitive host configuration; do not share in chat logs or commit to source control.
- Do not provide private keys or unrelated secrets; use the minimum values required for signing.
- Prefer Xcode Automatic Signing when possible to reduce manual secret/config handling.
- For autonomous/CI runs, keep these unset by default and require explicit opt-in for physical-device workflows.

If setup/build takes long, increase:

- `AGENT_DEVICE_DAEMON_TIMEOUT_MS` (default `90000`, for example `120000`)

If daemon startup fails with stale metadata hints, clean stale files and retry:

- `~/.agent-device/daemon.json`
- `~/.agent-device/daemon.lock`

## iOS: "Allow Paste" dialog

iOS 16+ shows an "Allow Paste" prompt when an app reads the system pasteboard. Under XCUITest (which `agent-device` uses), this prompt is suppressed by the testing runtime. Use `xcrun simctl pbcopy booted` to set clipboard content directly on the simulator instead.

## Simulator troubleshooting

- If snapshots return 0 nodes, restart Simulator and re-open the app.
