#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

"""Verify that Explorer's CocoaPods graph has one local Lynx owner."""

import argparse
import json
import os
import re
import subprocess
import sys
from pathlib import Path


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
SPARKLING_PODS = {
    "Sparkling",
    "SparklingMacro",
    "SparklingMethod",
    "Sparkling-Router",
}
FORBIDDEN_SPARKLING_PODS = {
    "Sparkling-DebugTool",
    "Sparkling-Media",
    "Sparkling-Storage",
}
FORBIDDEN_PATH_COMPONENTS = {"node_modules", "pods"}
COCOAPODS_PATH_ENV_VARS = ("CP_HOME_DIR", "CP_CACHE_DIR")


def _project_dir():
    return Path(__file__).resolve().parent.parent


def _resolve_project_path(path, project_dir):
    path = Path(path).expanduser()
    if not path.is_absolute():
        path = project_dir / path
    return path.resolve(strict=False)


def _cocoapods_roots(project_dir):
    home = Path.home()
    roots = {
        "CocoaPods home": (home / ".cocoapods").resolve(strict=False),
        "CocoaPods cache": (
            home / "Library" / "Caches" / "CocoaPods"
        ).resolve(strict=False),
    }
    for variable in COCOAPODS_PATH_ENV_VARS:
        value = os.environ.get(variable)
        if not value:
            continue
        path = Path(value).expanduser()
        candidates = (
            [path]
            if path.is_absolute()
            else [project_dir / path, Path.cwd() / path]
        )
        for index, candidate in enumerate(candidates):
            label = variable if index == 0 else f"{variable} (caller-relative)"
            roots[label] = candidate.resolve(strict=False)
    return roots


def _source_destination_error(source_root):
    project_dir = _project_dir().resolve(strict=False)
    try:
        source_root = _resolve_project_path(source_root, project_dir)
        forbidden_component = next(
            (
                part
                for part in source_root.parts
                if part.casefold() in FORBIDDEN_PATH_COMPONENTS
            ),
            None,
        )
        if forbidden_component:
            return (
                f"forbidden Sparkling source destination {source_root}: path component "
                f"'{forbidden_component}' is package-manager owned"
            )
        for label, root in _cocoapods_roots(project_dir).items():
            if _is_within(source_root, root):
                return (
                    f"forbidden Sparkling source destination {source_root}: "
                    f"inside {label} at {root}"
                )
    except (OSError, RuntimeError) as error:
        return f"cannot resolve Sparkling source destination {source_root}: {error}"
    return None


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


def _is_within(path, parent):
    path_parts = tuple(part.casefold() for part in path.parts)
    parent_parts = tuple(part.casefold() for part in parent.parts)
    return path_parts[: len(parent_parts)] == parent_parts


def _git(source_root, *args):
    return subprocess.run(
        ["git", "-C", str(source_root), *args],
        check=False,
        capture_output=True,
        text=True,
    )


def _validate_manifest(manifest_path, errors):
    error_count = len(errors)
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        errors.append(f"cannot read Sparkling source manifest {manifest_path}: {error}")
        return None

    if not isinstance(manifest, dict):
        errors.append("Sparkling source manifest must contain a JSON object")
        return None

    for field in ("repository", "removal_condition"):
        if not isinstance(manifest.get(field), str) or not manifest[field].strip():
            errors.append(f"Sparkling source manifest '{field}' must be a string")
    commit = manifest.get("commit")
    if not isinstance(commit, str) or not re.fullmatch(r"[0-9a-fA-F]{40}", commit):
        errors.append("Sparkling source manifest 'commit' must be a full 40-digit SHA")

    pods = manifest.get("pods")
    if not isinstance(pods, dict):
        errors.append("Sparkling source manifest 'pods' must be an object")
    elif set(pods) != SPARKLING_PODS:
        errors.append(
            "Sparkling source manifest must map exactly: "
            + ", ".join(sorted(SPARKLING_PODS))
        )
    else:
        for pod, relative_dir in pods.items():
            if not isinstance(relative_dir, str) or not relative_dir.strip():
                errors.append(
                    f"manifest path for {pod} must be a non-empty relative path"
                )
                continue
            path = Path(relative_dir)
            if path.is_absolute() or ".." in path.parts:
                errors.append(
                    f"manifest path for {pod} must be a non-empty relative path "
                    "inside the checkout"
                )
    if len(errors) != error_count:
        return None
    return manifest


def _validate_source_checkout(source_root, manifest, errors):
    if not source_root.is_dir():
        errors.append(
            f"Sparkling source checkout is missing at {source_root}; "
            "run scripts/sync_sparkling_source.py"
        )
        return

    origin = _git(source_root, "remote", "get-url", "origin")
    if origin.returncode != 0:
        errors.append(f"Sparkling source checkout at {source_root} has no origin remote")
    elif origin.stdout.strip() != manifest["repository"]:
        errors.append(
            "Sparkling source checkout origin mismatch: expected "
            f"{manifest['repository']}, found {origin.stdout.strip()}"
        )

    head = _git(source_root, "rev-parse", "HEAD")
    if head.returncode != 0:
        errors.append(f"Sparkling source checkout at {source_root} has no valid HEAD")
    elif head.stdout.strip().lower() != manifest["commit"].lower():
        errors.append(
            "Sparkling source checkout HEAD mismatch: expected "
            f"{manifest['commit']}, found {head.stdout.strip()}"
        )

    branch = _git(source_root, "symbolic-ref", "--quiet", "HEAD")
    if branch.returncode == 0:
        errors.append(
            "Sparkling source checkout must use a detached HEAD, found "
            f"{branch.stdout.strip()}"
        )

    status = _git(source_root, "status", "--porcelain", "--untracked-files=all")
    if status.returncode != 0:
        errors.append(f"cannot inspect Sparkling source checkout cleanliness at {source_root}")
    elif status.stdout.strip():
        errors.append(
            "Sparkling source checkout is dirty; preserve local changes and use a clean "
            f"checkout at {source_root}: {status.stdout.strip()}"
        )

    resolved_root = source_root.resolve()
    for pod, relative_dir in manifest["pods"].items():
        pod_dir = (source_root / relative_dir).resolve()
        if not _is_within(pod_dir, resolved_root):
            errors.append(f"manifest path for {pod} escapes the Sparkling source checkout")
            continue
        podspec = pod_dir / f"{pod}.podspec"
        if not podspec.is_file():
            errors.append(f"missing {pod} podspec at {podspec}")
            continue
        try:
            content = podspec.read_text(encoding="utf-8")
        except OSError as error:
            errors.append(f"cannot read {podspec}: {error}")
            continue
        name_match = re.search(
            r"\b(?:\w+\.)?name\s*=\s*['\"]([^'\"]+)['\"]", content
        )
        if not name_match or name_match.group(1) != pod:
            found = name_match.group(1) if name_match else "no pod name"
            errors.append(f"podspec name mismatch at {podspec}: expected {pod}, found {found}")


def verify_ownership(mode, lockfile, manifest_path, source_root, lynx_root):
    errors = []
    if mode not in {"enable_sparkling", "disable_sparkling"}:
        return ["mode must be 'enable_sparkling' or 'disable_sparkling'"]

    try:
        content = lockfile.read_text(encoding="utf-8")
    except OSError as error:
        return [f"cannot read CocoaPods lockfile {lockfile}: {error}"]
    pods, external_sources = parse_lockfile(content)

    destination_error = _source_destination_error(source_root)
    if destination_error:
        errors.append(destination_error)
    else:
        source_root = _resolve_project_path(source_root, _project_dir())

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

    manifest = _validate_manifest(manifest_path, errors)
    if manifest is None:
        return errors
    if destination_error:
        return errors
    _validate_source_checkout(source_root, manifest, errors)

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
        if not _is_within(actual_path, resolved_source_root):
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
        os.environ.get("SPARKLING_SOURCE_ROOT") or "generated/sparkling-source"
    )
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mode", choices=("enable_sparkling", "disable_sparkling"), required=True)
    parser.add_argument("--lockfile", type=Path, default=project_dir / "Podfile.lock")
    parser.add_argument(
        "--manifest", type=Path, default=project_dir / "sparkling-source.json"
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
