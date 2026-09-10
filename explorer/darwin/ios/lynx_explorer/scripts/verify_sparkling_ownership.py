#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

"""Verify that Explorer's CocoaPods graph has one local Lynx owner."""

import argparse
import os
import re
import sys
from pathlib import Path

EXPLORER_ROOT = Path(__file__).resolve().parents[4]
sys.path.insert(0, str(EXPLORER_ROOT / "scripts"))

from sparkling_source_validation import (
    SPARKLING_SOURCE_PODS,
    is_within,
    load_manifest,
    validate_checkout,
    validate_source_destination,
)


SOURCE_OWNED_PODS = {
    "Lynx",
    "LynxBase",
    "LynxServiceAPI",
    "LynxService",
    "LynxDevtool",
    "LynxLibraryRegistry",
    "BaseDevtool",
    "XElement",
}
GENERATED_SOURCE_OWNED_PODS = {
    "LynxLibraryRegistry": Path("generated/lynx-library"),
}
SPARKLING_PODS = set(SPARKLING_SOURCE_PODS)
FORBIDDEN_SPARKLING_PODS = {
    "Sparkling-DebugTool",
    "Sparkling-Media",
    "Sparkling-Storage",
}


def _project_dir():
    return Path(__file__).resolve().parent.parent


def _strip_lock_value(value):
    value = value.strip()
    if len(value) >= 2 and value[0] == value[-1] and value[0] in "'\"`":
        return value[1:-1]
    return value


def parse_lockfile(content):
    """Return root pod names and their EXTERNAL SOURCES attributes."""
    pods = set()
    external_sources = {}
    section = None
    current_source = None

    for line in content.splitlines():
        if line and not line[0].isspace() and line.endswith(":"):
            section = line[:-1]
            current_source = None
            continue

        if section == "PODS":
            match = re.match(r"^  - (.+?)(?: \([^)]*\))?:?$", line)
            if match:
                pods.add(match.group(1).split("/", 1)[0])
        elif section == "EXTERNAL SOURCES":
            source_match = re.match(r"^  ([^:]+):$", line)
            if source_match:
                current_source = source_match.group(1)
                external_sources[current_source] = {}
                continue
            attribute_match = re.match(r"^    :([^:]+):\s*(.*)$", line)
            if current_source and attribute_match:
                external_sources[current_source][attribute_match.group(1)] = (
                    _strip_lock_value(attribute_match.group(2))
                )

    return pods, external_sources


def _resolve_lock_path(lockfile, value):
    path = Path(value).expanduser()
    if not path.is_absolute():
        path = lockfile.parent / path
    return path.resolve()


def verify_ownership(mode, lockfile, manifest_path, source_root, lynx_root):
    errors = []
    if mode not in {"enable_sparkling", "disable_sparkling"}:
        return ["mode must be 'enable_sparkling' or 'disable_sparkling'"]

    try:
        content = lockfile.read_text(encoding="utf-8")
    except OSError as error:
        return [f"cannot read CocoaPods lockfile {lockfile}: {error}"]
    pods, external_sources = parse_lockfile(content)

    source_root, destination_errors = validate_source_destination(
        source_root,
        _project_dir(),
        os.environ,
    )
    errors.extend(destination_errors)

    expected_lynx_root = lynx_root.resolve()
    for pod in sorted(SOURCE_OWNED_PODS):
        if pod not in pods:
            errors.append(f"source-owned pod {pod} is missing from {lockfile}")
        source = external_sources.get(pod)
        if source is None:
            errors.append(
                f"{pod} must come from the current checkout via EXTERNAL SOURCES :path"
            )
            continue
        if set(source) != {"path"}:
            errors.append(
                f"{pod} must come from the current checkout via :path, found "
                + ", ".join(f":{key}" for key in sorted(source))
            )
            continue
        actual_path = _resolve_lock_path(lockfile, source["path"])
        expected_path = (
            (lockfile.parent / GENERATED_SOURCE_OWNED_PODS[pod]).resolve()
            if pod in GENERATED_SOURCE_OWNED_PODS
            else expected_lynx_root
        )
        if actual_path != expected_path:
            errors.append(
                f"{pod} must come from the current checkout at {expected_path}, "
                f"found {actual_path}"
            )

    installed_sparkling = {pod for pod in pods if pod.startswith("Sparkling")}
    present_sparkling = set(installed_sparkling)
    present_sparkling.update(
        pod for pod in external_sources if pod.startswith("Sparkling")
    )
    forbidden = present_sparkling & FORBIDDEN_SPARKLING_PODS
    for pod in sorted(forbidden):
        errors.append(f"forbidden Sparkling pod is present: {pod}")

    if mode == "disable_sparkling":
        for pod in sorted(present_sparkling):
            errors.append(f"Sparkling pod {pod} must be absent when mode is disable_sparkling")
        return errors

    missing_sparkling = SPARKLING_PODS - installed_sparkling
    extra_sparkling = present_sparkling - SPARKLING_PODS
    for pod in sorted(missing_sparkling):
        errors.append(f"required Sparkling source pod is missing: {pod}")
    for pod in sorted(extra_sparkling - forbidden):
        errors.append(f"unexpected Sparkling pod is present: {pod}")

    manifest, manifest_errors = load_manifest(manifest_path)
    errors.extend(manifest_errors)
    if manifest is None:
        return errors
    if destination_errors:
        return errors
    errors.extend(validate_checkout(source_root, manifest))

    resolved_source_root = source_root.resolve()
    for pod in sorted(SPARKLING_PODS):
        source = external_sources.get(pod)
        if source is None:
            errors.append(f"{pod} must use an EXTERNAL SOURCES :path entry")
            continue
        if set(source) != {"path"}:
            errors.append(
                f"{pod} must use a local :path source, found "
                + ", ".join(f":{key}" for key in sorted(source))
            )
            continue
        actual_path = _resolve_lock_path(lockfile, source["path"])
        expected_path = (source_root / manifest["pods"][pod]).resolve()
        if not is_within(actual_path, resolved_source_root):
            errors.append(
                f"{pod} source path must stay inside {resolved_source_root}, "
                f"found {actual_path}"
            )
        if actual_path != expected_path:
            errors.append(
                f"{pod} source path must match manifest path {expected_path}, "
                f"found {actual_path}"
            )
    return errors


def parse_args(argv=None):
    project_dir = _project_dir()
    default_source_root = (
        os.environ.get("SPARKLING_SOURCE_ROOT")
        or EXPLORER_ROOT / "generated/sparkling-source"
    )
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", choices=("enable_sparkling", "disable_sparkling"), required=True)
    parser.add_argument("--lockfile", type=Path, default=project_dir / "Podfile.lock")
    parser.add_argument(
        "--manifest", type=Path, default=EXPLORER_ROOT / "sparkling-source.json"
    )
    parser.add_argument(
        "--source-root",
        type=Path,
        default=Path(default_source_root),
    )
    parser.add_argument(
        "--lynx-root", type=Path, default=project_dir.parents[3]
    )
    return parser.parse_args(argv)


def main(argv=None):
    args = parse_args(argv)
    errors = verify_ownership(
        args.mode,
        args.lockfile,
        args.manifest,
        args.source_root,
        args.lynx_root,
    )
    if errors:
        print("Sparkling ownership verification failed:", file=sys.stderr)
        for error in errors:
            print(f"- {error}", file=sys.stderr)
        return 1
    print(f"Sparkling ownership verification passed ({args.mode} mode)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
