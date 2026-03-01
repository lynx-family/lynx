# Logs (Token-Efficient Debugging)

Logging is off by default in normal flows. Enable it on demand for debugging windows. App output is written to a session-scoped file so agents can grep it instead of loading full logs into context.
`network dump` parses recent HTTP(s) entries from this same session app log file.

## Data Handling

- Default app logs are stored under `~/.agent-device/sessions/<session>/app.log`.
- Replay scripts saved with `--save-script` are written to the explicit path you provide.
- Log files may contain sensitive runtime data; review before sharing and clean up when finished.
- Use `AGENT_DEVICE_APP_LOG_REDACT_PATTERNS` to redact sensitive patterns at write time when needed.

## Retention and Cleanup

- Keep logging scoped to active debug windows (`logs clear --restart` before repro, `logs stop` after repro).
- Prefer bounded inspection (`grep -n`, `tail -50`) instead of reading full logs into context.
- Clear session logs when finished:
  `agent-device logs clear`
- Close session to stop background logging state:
  `agent-device close`

## Quick Flow

```bash
agent-device open MyApp --platform ios      # or --platform android
agent-device logs clear --restart    # Preferred: stop stream, clear logs, and start streaming again
agent-device network dump 25         # Parse latest HTTP(s) requests (method/url/status) from app.log
agent-device logs path               # Print path, e.g. ~/.agent-device/sessions/default/app.log
agent-device logs doctor             # Check tool/runtime readiness for current session/device
agent-device logs mark "before tap"  # Insert a timeline marker into app.log
# ... run flows; on failure, grep the path (see below)
agent-device logs stop               # Stop streaming (optional; close also stops)
```

Precondition: `logs clear --restart` requires an active app session (`open <app>` first).

## Command Notes

- `logs path`: returns log file path and metadata (`active`, `state`, `backend`, size, timestamps).
- `logs start`: starts streaming; requires an active app session (`open` first). Supported on iOS simulator, iOS device, and Android.
- `logs stop`: stops streaming. Session `close` also stops logging.
- `logs clear`: truncates `app.log` and removes rotated `app.log.N` files. Requires logging to be stopped first.
- `logs clear --restart`: convenience reset for repro loops (stop stream, clear files, restart stream).
- `logs doctor`: reports backend/tool checks and readiness notes for troubleshooting.
- `logs mark`: writes a timestamped marker line to the session log.
- `network dump [limit] [summary|headers|body|all]`: parses recent HTTP(s) lines from the session app log and returns request summaries.
- `network log ...`: alias for `network dump`.

## Behavior and Limits

- `logs start` appends to `app.log` and rotates to `app.log.1` when `app.log` exceeds 5 MB.
- `network dump` scans the last 4000 app-log lines, returns up to 200 entries, and truncates payload/header fields at 2048 characters.
- Android log streaming automatically rebinds to the app PID after process restarts.
- iOS log capture relies on Unified Logging signals (for example `os_log`); plain stdout/stderr output may be limited depending on app/runtime.
- Retention knobs:
  - `AGENT_DEVICE_APP_LOG_MAX_BYTES`
  - `AGENT_DEVICE_APP_LOG_MAX_FILES`
- Optional write-time redaction patterns:
  - `AGENT_DEVICE_APP_LOG_REDACT_PATTERNS` (comma-separated regex)

## Grep Patterns

After getting the path from `logs path`, run `grep` (or `grep -E`) so only matching lines enter context.

```bash
# Get path first, then grep it; -n adds line numbers
grep -n "Error\|Exception\|Fatal" <path>
grep -n -E "Error|Exception|Fatal|crash" <path>

# Bounded context: last N lines only
tail -50 <path>
```

- Use `-n` for line numbers.
- Use `-E` for extended regex so `|` in the pattern does not need escaping.
- Prefer targeted patterns (e.g. `Error`, `Exception`, or app-specific tags) over reading the full file.

## Crash Triage Fast Path

Always start from the session app log, then branch by platform.

```bash
agent-device logs path
grep -n -E "SIGABRT|SIGSEGV|EXC_|fatal|exception|terminated|killed|jetsam|memorystatus|FATAL EXCEPTION|Abort message" <path>
nl -ba <path> | sed -n '<start>,<end>p'
```

### iOS

```bash
# If log shows ReportCrash / SIGABRT / EXC_*, inspect simulator DiagnosticReports:
ls -lt ~/Library/Logs/DiagnosticReports | grep -E "<AppName>|<BundleId>" | head
```

- `SIGABRT`: app/runtime abort; inspect `.ips` triggered thread and top frames.
- `SIGKILL` + jetsam/memorystatus markers: memory-pressure kill.
- `EXC_BAD_ACCESS`/`SIGSEGV`: native memory access issue.

### Android

```bash
# Capture fatal crash lines around app process death:
adb -s <serial> logcat -d | grep -n -E "FATAL EXCEPTION|Process: <package>|Abort message|signal [0-9]+ \\(SIG"
```

- `FATAL EXCEPTION` with Java stack: uncaught Java/Kotlin exception.
- `signal 6 (SIGABRT)` or `signal 11 (SIGSEGV)` with tombstone refs: native crash path (NDK/JNI/runtime).
- `Low memory killer` / `Killing <pid>` entries: OS memory-pressure/process reclaim.

## Stop Conditions

- If no crash signature appears in app log, switch to platform-native crash sources (`.ips` on iOS, logcat/tombstone flow on Android).
- If signatures are present and root cause class is identified (abort, native fault, memory pressure), stop collecting broad logs and focus on reproducing the specific path.
