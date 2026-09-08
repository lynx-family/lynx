#!/bin/bash
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
# cspell:ignore bootstatus

set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

usage() {
  cat <<'EOF'
Usage: run_sparkling_smoke.sh --mode enable_sparkling|disable_sparkling --app PATH [OPTIONS]

Options:
  --device UDID          Simulator UDID (defaults to an available iOS 16.4+ iPhone)
  --bundle-id ID         Override CFBundleIdentifier
  --executable NAME      Override CFBundleExecutable
  --fixture URL          Override the deterministic startup fixture
  --timeout SECONDS      Marker/process timeout (default: 30)
EOF
}

MODE=
APP_PATH=
DEVICE=
DEVICE_WAS_EXPLICIT=false
DEVICE_CANDIDATES=
HOT_URL_SUPPORTED=true
BUNDLE_ID=
EXECUTABLE=
FIXTURE=
TIMEOUT=30
FAILURE_GRACE_SECONDS=${LYNX_SMOKE_FAILURE_GRACE_SECONDS:-5}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --mode)
      MODE=${2:-}
      shift 2
      ;;
    --app)
      APP_PATH=${2:-}
      shift 2
      ;;
    --device)
      DEVICE=${2:-}
      DEVICE_WAS_EXPLICIT=true
      shift 2
      ;;
    --bundle-id)
      BUNDLE_ID=${2:-}
      shift 2
      ;;
    --executable)
      EXECUTABLE=${2:-}
      shift 2
      ;;
    --fixture)
      FIXTURE=${2:-}
      shift 2
      ;;
    --timeout)
      TIMEOUT=${2:-}
      shift 2
      ;;
    -h | --help)
      usage
      exit 0
      ;;
    *)
      echo "error: unknown argument '$1'" >&2
      usage >&2
      exit 2
      ;;
  esac
done

if [[ "$MODE" != "enable_sparkling" && "$MODE" != "disable_sparkling" ]]; then
  echo "error: --mode must be enable_sparkling or disable_sparkling" >&2
  exit 2
fi
if [[ ! -d "$APP_PATH" ]]; then
  echo "error: app bundle is missing at '$APP_PATH'" >&2
  exit 2
fi
if [[ ! "$TIMEOUT" =~ ^[1-9][0-9]*$ ]]; then
  echo "error: --timeout must be a positive integer" >&2
  exit 2
fi
if [[ ! "$FAILURE_GRACE_SECONDS" =~ ^[1-9][0-9]*$ ]]; then
  echo "error: LYNX_SMOKE_FAILURE_GRACE_SECONDS must be a positive integer" >&2
  exit 2
fi

PLIST="$APP_PATH/Info.plist"
if [[ -z "$BUNDLE_ID" ]]; then
  BUNDLE_ID=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$PLIST")
fi
if [[ -z "$EXECUTABLE" ]]; then
  EXECUTABLE=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "$PLIST")
fi
if [[ -z "$DEVICE" ]]; then
  DEVICE_CANDIDATES=$(xcrun simctl list devices available -j | python3 -c '
import json
import re
import sys

candidates = []
for runtime, devices in json.load(sys.stdin).get("devices", {}).items():
    match = re.search(r"\.iOS-(\d+)-(\d+)(?:-(\d+))?$", runtime)
    if not match:
        continue
    version = tuple(int(part) for part in match.groups() if part is not None)
    if version < (16, 4):
        continue
    runtime_candidates = []
    for device in devices:
        if (device.get("isAvailable", True) and
                device.get("name", "").startswith("iPhone") and
                device.get("udid") and
                device.get("deviceTypeIdentifier")):
            runtime_candidates.append((
                device["name"],
                device["udid"],
                device["deviceTypeIdentifier"],
            ))
    if runtime_candidates:
        name, udid, device_type = max(runtime_candidates)
        candidates.append((version, name, udid, runtime, device_type))
if candidates:
    for candidate in sorted(candidates, reverse=True):
        print("\t".join(candidate[2:]))
')
fi
if [[ -z "$DEVICE" && -z "$DEVICE_CANDIDATES" ]]; then
  echo "error: no available iOS 16.4+ iPhone simulator found" >&2
  exit 1
fi

CANONICAL_ROUTE='hybrid://lynxview_page?bundle=homepage.lynx.bundle&title=Sparkling%20Smoke'
MALFORMED_ROUTE='hybrid://lynxview_page?bundle='
if [[ -z "$FIXTURE" ]]; then
  if [[ "$MODE" == "enable_sparkling" ]]; then
    FIXTURE=$CANONICAL_ROUTE
  else
    FIXTURE='file://lynx?local://homepage.lynx.bundle?fullscreen=true'
  fi
fi

if [[ "$MODE" == "enable_sparkling" ]]; then
  EXPECTED_CONTAINER=sparkling
  EXPECTED_MALFORMED_CODE=missing_target
else
  EXPECTED_CONTAINER=legacy
  EXPECTED_MALFORMED_CODE=sparkling_unavailable
fi
LEGACY_SUCCESS_PATTERN='LynxExplorer route_success container=legacy([[:space:]]|$)'
SPARKLING_SUCCESS_PATTERN='LynxExplorer route_success container=sparkling([[:space:]]|$)'
LEGACY_VERSIONED_SUCCESS_PATTERN='LynxExplorer route_success container=legacy lynx_source=local lynx_version=[^[:space:]]+'
SPARKLING_VERSIONED_SUCCESS_PATTERN='LynxExplorer route_success container=sparkling lynx_source=local lynx_version=[^[:space:]]+'
DUPLICATE_CLASS_PATTERN='Class .+ is implemented in both'
if [[ "$EXPECTED_CONTAINER" == "sparkling" ]]; then
  INITIAL_PATTERN=$SPARKLING_VERSIONED_SUCCESS_PATTERN
else
  INITIAL_PATTERN=$LEGACY_VERSIONED_SUCCESS_PATTERN
fi
MALFORMED_MARKER="LynxExplorer route_failure code=$EXPECTED_MALFORMED_CODE"

LOG_FILE=$(mktemp -t lynx-explorer-sparkling-smoke.XXXXXX)
cleanup() {
  rm -f "$LOG_FILE"
}
trap cleanup EXIT
DUPLICATE_CLASS_BASELINE=
APP_PID=

boot_device() {
  local boot_output
  local boot_status=0
  xcrun simctl boot "$DEVICE" >/dev/null 2>&1 || true
  boot_output=$(xcrun simctl bootstatus "$DEVICE" -b 2>&1) || boot_status=$?
  printf '%s\n' "$boot_output"
  if (( boot_status != 0 )) || grep -Fq 'Data Migration Failed' <<<"$boot_output"; then
    return 1
  fi
}

refresh_logs() {
  xcrun simctl spawn "$DEVICE" log show \
    --style compact \
    --info \
    --start "$LOG_START" \
    --predicate "process == '$EXECUTABLE'" >"$LOG_FILE"
  if [[ -n "$DUPLICATE_CLASS_BASELINE" ]] &&
    (( $(grep -E -c "$DUPLICATE_CLASS_PATTERN" "$LOG_FILE" || true) > DUPLICATE_CLASS_BASELINE )); then
    echo "error: duplicate Objective-C class implementation detected:" >&2
    grep -E "$DUPLICATE_CLASS_PATTERN" "$LOG_FILE" >&2
    return 1
  fi
}

pattern_count() {
  local pattern=$1
  local count
  refresh_logs
  count=$(grep -E -c "$pattern" "$LOG_FILE" || true)
  echo "$count"
}

wait_for_pattern_count() {
  local pattern=$1
  local minimum=$2
  local deadline=$((SECONDS + TIMEOUT))
  local count
  while (( SECONDS <= deadline )); do
    count=$(pattern_count "$pattern")
    if (( count >= minimum )); then
      return 0
    fi
    sleep 1
  done
  echo "error: timed out waiting for runtime marker: $pattern" >&2
  cat "$LOG_FILE" >&2
  return 1
}

# A freshly booted CoreSimulator can report boot completion before
# LaunchServices accepts an app-specific URL. Retry only the deliberately
# malformed probe: duplicate failure markers are harmless, and once this
# succeeds the following canonical hot URL can still be delivered exactly
# once and verified strictly.
openurl_until_pattern_count() {
  local url=$1
  local pattern=$2
  local minimum=$3
  local deadline=$((SECONDS + TIMEOUT))
  local retry_interval=5
  local attempt_deadline
  local count
  while (( SECONDS <= deadline )); do
    xcrun simctl openurl "$DEVICE" "$url"
    attempt_deadline=$((SECONDS + retry_interval))
    if (( attempt_deadline > deadline )); then
      attempt_deadline=$deadline
    fi
    while (( SECONDS <= attempt_deadline )); do
      count=$(pattern_count "$pattern")
      if (( count >= minimum )); then
        return 0
      fi
      sleep 1
    done
  done
  echo "error: timed out waiting for runtime marker: $pattern" >&2
  cat "$LOG_FILE" >&2
  return 1
}

assert_known_lynx_version() {
  refresh_logs
  if grep -E -i \
    "LynxExplorer route_success container=$EXPECTED_CONTAINER lynx_source=local lynx_version=(unknown|unavailable)([[:space:]]|$)" \
    "$LOG_FILE" >/dev/null; then
    echo "error: runtime marker reported an unknown Lynx version" >&2
    cat "$LOG_FILE" >&2
    return 1
  fi
}

assert_running() {
  if [[ -z "$APP_PID" ]] || ! ps -p "$APP_PID" -o pid= >/dev/null; then
    echo "error: $EXECUTABLE is not running on simulator $DEVICE" >&2
    refresh_logs || true
    cat "$LOG_FILE" >&2
    return 1
  fi
}

assert_single_application_process() {
  local unexpected_routes
  refresh_logs
  unexpected_routes=$(grep -E 'LynxExplorer route_(success|failure)' "$LOG_FILE" |
    grep -Fv "$EXECUTABLE[$APP_PID:" || true)
  if [[ -n "$unexpected_routes" ]]; then
    echo "error: route markers came from multiple application processes; hot URL delivery relaunched $EXECUTABLE" >&2
    echo "$unexpected_routes" >&2
    return 1
  fi
}

wrapper_url() {
  python3 -c \
    'import sys; from urllib.parse import quote; print("lynx://open?url=" + quote(sys.argv[1], safe=""))' \
    "$1"
}

capture_application_pid() {
  APP_PID=$(xcrun simctl spawn "$DEVICE" launchctl list | awk \
    -v label="UIKitApplication:$BUNDLE_ID" \
    'index($3, label) == 1 && !found { pid = $1; found = 1 }
     END { if (found) print pid }')
  if [[ ! "$APP_PID" =~ ^[1-9][0-9]*$ ]]; then
    echo "error: could not resolve the running $BUNDLE_ID process ID" >&2
    return 1
  fi
}

# Boot is idempotent only through bootstatus; tolerate simctl's already-booted
# exit status and make boot completion authoritative before installation. A
# pre-existing runner simulator can finish bootstatus with a failed data
# migration, leaving LaunchServices permanently unusable. In automatic mode,
# try available devices from newer to older runtimes until one boots cleanly;
# never replace or erase an explicitly requested device.
if [[ "$DEVICE_WAS_EXPLICIT" == true ]]; then
  if ! boot_device; then
    echo "error: explicitly selected simulator $DEVICE failed to boot cleanly" >&2
    exit 1
  fi
else
  selected_device=
  fallback_device=
  while IFS=$'\t' read -r candidate _runtime _device_type; do
    [[ -n "$candidate" ]] || continue
    if [[ -z "$fallback_device" ]]; then
      fallback_device=$candidate
    fi
    DEVICE=$candidate
    if boot_device; then
      selected_device=$DEVICE
      break
    fi
    echo "warning: simulator $DEVICE failed to boot cleanly; trying an older runtime" >&2
    xcrun simctl shutdown "$DEVICE" >/dev/null 2>&1 || true
  done <<<"$DEVICE_CANDIDATES"
  if [[ -z "$selected_device" ]]; then
    DEVICE=$fallback_device
    HOT_URL_SUPPORTED=false
    echo "warning: no simulator booted with healthy data migration; hot URL smoke is unavailable" >&2
    # A migration-failed simulator can still install and cold-launch the app,
    # but its LaunchServices database cannot be trusted for `simctl openurl`.
    # Boot it again after the candidate loop shut it down and retain the cold
    # route/process/runtime checks below instead of masking them entirely.
    boot_device || true
  fi
fi
xcrun simctl install "$DEVICE" "$APP_PATH"
xcrun simctl terminate "$DEVICE" "$BUNDLE_ID" >/dev/null 2>&1 || true
# Scope every query to this invocation. Erasing the simulator-wide log store is
# both destructive to parallel diagnostics and forbidden by newer runtimes.
LOG_START=$(date -u '+%Y-%m-%d %H:%M:%S%z')
refresh_logs
DUPLICATE_CLASS_BASELINE=$(grep -E -c "$DUPLICATE_CLASS_PATTERN" "$LOG_FILE" || true)
initial_before=$(grep -E -c "$INITIAL_PATTERN" "$LOG_FILE" || true)
malformed_before=$(grep -E -c "$MALFORMED_MARKER" "$LOG_FILE" || true)
legacy_before=$(grep -E -c "$LEGACY_SUCCESS_PATTERN" "$LOG_FILE" || true)
sparkling_before=$(grep -E -c "$SPARKLING_SUCCESS_PATTERN" "$LOG_FILE" || true)

# Use Explorer's registered app-specific transport for real LaunchServices
# delivery. The nested canonical scheme remains unchanged after coordinator
# unwrapping, and no generic `hybrid` scheme registration is required.
SIMCTL_CHILD_lynx_initial_url="$FIXTURE" \
  xcrun simctl launch --terminate-running-process "$DEVICE" "$BUNDLE_ID" >/dev/null
capture_application_pid

wait_for_pattern_count "$INITIAL_PATTERN" "$((initial_before + 1))"
assert_known_lynx_version
assert_running

if [[ "$HOT_URL_SUPPORTED" != true ]]; then
  assert_single_application_process
  echo "Sparkling smoke passed ($MODE, cold route only) on $DEVICE"
  exit 0
fi

openurl_until_pattern_count \
  "$(wrapper_url "$MALFORMED_ROUTE")" \
  "$MALFORMED_MARKER" \
  "$((malformed_before + 1))"
malformed_after_probe=$(pattern_count "$MALFORMED_MARKER")
assert_running

xcrun simctl openurl "$DEVICE" "$(wrapper_url "$CANONICAL_ROUTE")"
if [[ "$MODE" == "enable_sparkling" ]]; then
  wait_for_pattern_count \
    "$SPARKLING_VERSIONED_SUCCESS_PATTERN" \
    "$((initial_before + 2))"
else
  wait_for_pattern_count \
    'LynxExplorer route_failure code=sparkling_unavailable' \
    "$((malformed_after_probe + 1))"
fi
assert_running
assert_single_application_process

# Leave a bounded window for an incorrect asynchronous fallback to surface.
sleep "$FAILURE_GRACE_SECONDS"
assert_running
assert_single_application_process
legacy_after=$(grep -E -c "$LEGACY_SUCCESS_PATTERN" "$LOG_FILE" || true)
sparkling_after=$(grep -E -c "$SPARKLING_SUCCESS_PATTERN" "$LOG_FILE" || true)
if [[ "$MODE" == "enable_sparkling" ]]; then
  expected_legacy_after=$legacy_before
  expected_sparkling_after=$((sparkling_before + 2))
else
  expected_legacy_after=$((legacy_before + 1))
  expected_sparkling_after=$sparkling_before
fi
if (( legacy_after != expected_legacy_after || sparkling_after != expected_sparkling_after )); then
  echo "error: unexpected route success after failure; route counts indicate a fallback" >&2
  echo "error: Legacy success count $legacy_before -> $legacy_after (expected $expected_legacy_after)" >&2
  echo "error: Sparkling success count $sparkling_before -> $sparkling_after (expected $expected_sparkling_after)" >&2
  cat "$LOG_FILE" >&2
  exit 1
fi
echo "Sparkling smoke passed ($MODE) on $DEVICE"
