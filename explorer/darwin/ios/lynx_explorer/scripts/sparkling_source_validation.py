#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

"""Shared validation for Explorer's pinned Sparkling source checkout."""

import json
import os
import re
import subprocess
from pathlib import Path


SPARKLING_SOURCE_PODS = frozenset(
    {
        "Sparkling",
        "SparklingMacro",
        "SparklingMethod",
        "Sparkling-Router",
    }
)
_FORBIDDEN_PATH_COMPONENTS = {"node_modules", "pods"}
_COCOAPODS_PATH_ENV_VARS = ("CP_HOME_DIR", "CP_CACHE_DIR")


def resolve_project_path(path, project_dir):
    path = Path(path).expanduser()
    if not path.is_absolute():
        path = project_dir / path
    return path.resolve(strict=False)


def is_within(path, parent):
    path_parts = tuple(part.casefold() for part in path.parts)
    parent_parts = tuple(part.casefold() for part in parent.parts)
    return path_parts[: len(parent_parts)] == parent_parts


def _cocoapods_roots(project_dir, environ):
    home = Path(environ.get("HOME") or Path.home()).expanduser()
    roots = {
        "CocoaPods home": (home / ".cocoapods").resolve(strict=False),
        "CocoaPods cache": (
            home / "Library" / "Caches" / "CocoaPods"
        ).resolve(strict=False),
    }
    for variable in _COCOAPODS_PATH_ENV_VARS:
        value = environ.get(variable)
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


def validate_source_destination(source_root, project_dir, environ=None):
    """Return the resolved destination and every package-manager ownership error."""
    project_dir = project_dir.resolve(strict=False)
    environ = os.environ if environ is None else environ
    try:
        source_root = resolve_project_path(source_root, project_dir)
    except (OSError, RuntimeError) as error:
        return Path(source_root), [
            f"cannot resolve Sparkling source destination {source_root}: {error}"
        ]

    errors = []
    forbidden_component = next(
        (
            part
            for part in source_root.parts
            if part.casefold() in _FORBIDDEN_PATH_COMPONENTS
        ),
        None,
    )
    if forbidden_component:
        errors.append(
            f"forbidden Sparkling source destination {source_root}: "
            f"path component '{forbidden_component}' is package-manager owned"
        )

    for label, root in _cocoapods_roots(project_dir, environ).items():
        if is_within(source_root, root):
            errors.append(
                f"forbidden Sparkling source destination {source_root}: "
                f"inside {label} at {root}"
            )
    return source_root, errors


def load_manifest(manifest_path):
    """Return a validated manifest and all schema errors."""
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        return None, [
            f"cannot read Sparkling source manifest {manifest_path}: {error}"
        ]

    if not isinstance(manifest, dict):
        return None, ["Sparkling source manifest must contain a JSON object"]

    errors = []
    for field in ("repository", "commit", "removal_condition"):
        if not isinstance(manifest.get(field), str) or not manifest[field].strip():
            errors.append(f"Sparkling source manifest '{field}' must be a string")

    commit = manifest.get("commit")
    if not isinstance(commit, str) or not re.fullmatch(r"[0-9a-fA-F]{40}", commit):
        errors.append("Sparkling source manifest 'commit' must be a full 40-digit SHA")

    pods = manifest.get("pods")
    if not isinstance(pods, dict):
        errors.append("Sparkling source manifest 'pods' must be an object")
    elif set(pods) != SPARKLING_SOURCE_PODS:
        errors.append(
            "Sparkling source manifest must map exactly: "
            + ", ".join(sorted(SPARKLING_SOURCE_PODS))
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
    return (None if errors else manifest), errors


def _git(source_root, *args):
    return subprocess.run(
        ["git", "-C", str(source_root), *args],
        check=False,
        capture_output=True,
        text=True,
    )


def validate_checkout(source_root, manifest):
    """Return all provenance and podspec errors for a materialized checkout."""
    if not source_root.is_dir():
        return [f"Sparkling source checkout is missing at {source_root}"]
    if not (source_root / ".git").exists():
        return [
            f"Sparkling source checkout is not a Git repository: {source_root}"
        ]

    errors = []
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
        errors.append(
            f"cannot inspect Sparkling source checkout cleanliness at {source_root}"
        )
    elif status.stdout.strip():
        errors.append(
            "Sparkling source checkout is dirty; use a clean checkout at "
            f"{source_root}: {status.stdout.strip()}"
        )

    resolved_root = source_root.resolve()
    for pod, relative_dir in manifest["pods"].items():
        pod_dir = (source_root / relative_dir).resolve()
        if not is_within(pod_dir, resolved_root):
            errors.append(f"manifest path for {pod} escapes {resolved_root}")
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
            errors.append(
                f"podspec name mismatch at {podspec}: expected {pod}, found {found}"
            )
    return errors
