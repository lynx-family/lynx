#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

"""Materialize and validate Explorer's pinned Sparkling source checkout."""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


EXPECTED_PODS = {
    "Sparkling",
    "SparklingMacro",
    "SparklingMethod",
    "Sparkling-Router",
}
FORBIDDEN_PATH_COMPONENTS = {"node_modules", "pods"}
COCOAPODS_PATH_ENV_VARS = ("CP_HOME_DIR", "CP_CACHE_DIR")


class SourceError(RuntimeError):
    pass


def _resolve_project_path(path, project_dir):
    path = Path(path).expanduser()
    if not path.is_absolute():
        path = project_dir / path
    try:
        return path.resolve(strict=False)
    except (OSError, RuntimeError) as error:
        raise SourceError(f"cannot resolve Sparkling source path {path}: {error}")


def _is_within(path, parent):
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
    for variable in COCOAPODS_PATH_ENV_VARS:
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


def validate_source_destination(source_root, project_dir=None, environ=None):
    """Reject mutable package-manager locations before any checkout writes."""
    project_dir = (project_dir or _project_dir()).resolve(strict=False)
    environ = os.environ if environ is None else environ
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
        raise SourceError(
            f"forbidden Sparkling source destination {source_root}: "
            f"path component '{forbidden_component}' is package-manager owned"
        )

    for label, root in _cocoapods_roots(project_dir, environ).items():
        if _is_within(source_root, root):
            raise SourceError(
                f"forbidden Sparkling source destination {source_root}: "
                f"inside {label} at {root}"
            )
    return source_root


def load_manifest(manifest_path):
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise SourceError(f"cannot read Sparkling source manifest {manifest_path}: {error}")

    if not isinstance(manifest, dict):
        raise SourceError("Sparkling source manifest must contain a JSON object")

    for field in ("repository", "commit", "removal_condition"):
        if not isinstance(manifest.get(field), str) or not manifest[field].strip():
            raise SourceError(f"Sparkling source manifest '{field}' must be a string")
    if not re.fullmatch(r"[0-9a-fA-F]{40}", manifest["commit"]):
        raise SourceError("Sparkling source manifest 'commit' must be a full 40-digit SHA")

    pods = manifest.get("pods")
    if not isinstance(pods, dict) or set(pods) != EXPECTED_PODS:
        raise SourceError(
            "Sparkling source manifest must map exactly: "
            + ", ".join(sorted(EXPECTED_PODS))
        )
    for pod, relative_dir in pods.items():
        if not isinstance(relative_dir, str) or not relative_dir.strip():
            raise SourceError(f"Sparkling source manifest path for {pod} must be a string")
        path = Path(relative_dir)
        if path.is_absolute() or ".." in path.parts:
            raise SourceError(
                f"Sparkling source manifest path for {pod} must stay inside the checkout"
            )
    return manifest


def _git(source_root, *args, check=True):
    result = subprocess.run(
        ["git", "-C", str(source_root), *args],
        check=False,
        capture_output=True,
        text=True,
    )
    if check and result.returncode != 0:
        detail = result.stderr.strip() or result.stdout.strip()
        raise SourceError(f"git {' '.join(args)} failed in {source_root}: {detail}")
    return result


def validate_checkout(source_root, manifest):
    if not source_root.is_dir():
        raise SourceError(f"Sparkling source checkout is missing at {source_root}")
    if not (source_root / ".git").exists():
        raise SourceError(f"Sparkling source checkout is not a Git repository: {source_root}")

    origin = _git(source_root, "remote", "get-url", "origin").stdout.strip()
    if origin != manifest["repository"]:
        raise SourceError(
            f"Sparkling source origin mismatch: expected {manifest['repository']}, "
            f"found {origin}"
        )

    head = _git(source_root, "rev-parse", "HEAD").stdout.strip()
    if head.lower() != manifest["commit"].lower():
        raise SourceError(
            f"Sparkling source HEAD mismatch: expected {manifest['commit']}, found {head}"
        )

    branch = _git(source_root, "symbolic-ref", "--quiet", "HEAD", check=False)
    if branch.returncode == 0:
        raise SourceError(
            "Sparkling source checkout must use a detached HEAD, found "
            + branch.stdout.strip()
        )

    status = _git(
        source_root, "status", "--porcelain", "--untracked-files=all"
    ).stdout.strip()
    if status:
        raise SourceError(
            "Sparkling source checkout is dirty; local changes were not overwritten:\n"
            + status
        )

    resolved_root = source_root.resolve()
    for pod, relative_dir in manifest["pods"].items():
        pod_dir = (source_root / relative_dir).resolve()
        if not _is_within(pod_dir, resolved_root):
            raise SourceError(f"Sparkling source path for {pod} escapes {resolved_root}")
        podspec = pod_dir / f"{pod}.podspec"
        if not podspec.is_file():
            raise SourceError(f"missing {pod} podspec at {podspec}")
        try:
            content = podspec.read_text(encoding="utf-8")
        except OSError as error:
            raise SourceError(f"cannot read {podspec}: {error}")
        name_match = re.search(
            r"\b(?:\w+\.)?name\s*=\s*['\"]([^'\"]+)['\"]", content
        )
        if not name_match or name_match.group(1) != pod:
            found = name_match.group(1) if name_match else "no pod name"
            raise SourceError(
                f"podspec name mismatch at {podspec}: expected {pod}, found {found}"
            )


def _materialize_checkout(source_root, manifest):
    source_root.parent.mkdir(parents=True, exist_ok=True)
    temporary_root = Path(
        tempfile.mkdtemp(
            prefix=f".{source_root.name}.tmp-", dir=str(source_root.parent)
        )
    )
    try:
        _git(temporary_root, "init", "--quiet")
        _git(temporary_root, "remote", "add", "origin", manifest["repository"])
        _git(
            temporary_root,
            "fetch",
            "--depth",
            "1",
            "origin",
            manifest["commit"],
        )
        _git(temporary_root, "checkout", "--quiet", "--detach", "FETCH_HEAD")
        validate_checkout(temporary_root, manifest)
        if os.path.lexists(source_root):
            raise SourceError(
                f"refusing to replace existing Sparkling source path {source_root}"
            )
        os.replace(temporary_root, source_root)
    finally:
        if temporary_root.exists():
            shutil.rmtree(temporary_root)


def sync_source(source_root, manifest, check_only=False):
    source_root = validate_source_destination(source_root)
    if os.path.lexists(source_root):
        validate_checkout(source_root, manifest)
        return "matches pinned source" if check_only else "already matches pinned source"
    if check_only:
        raise SourceError(f"Sparkling source checkout is missing at {source_root}")
    _materialize_checkout(source_root, manifest)
    return "materialized pinned source"


def _project_dir():
    return Path(__file__).resolve().parent.parent


def parse_args(argv=None):
    project_dir = _project_dir()
    default_source_root = (
        os.environ.get("SPARKLING_SOURCE_ROOT") or "generated/sparkling-source"
    )
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--manifest", type=Path, default=project_dir / "sparkling-source.json"
    )
    parser.add_argument("--source-root", type=Path, default=Path(default_source_root))
    parser.add_argument(
        "--check", action="store_true", help="validate without creating a checkout"
    )
    return parser.parse_args(argv)


def main(argv=None):
    args = parse_args(argv)
    try:
        source_root = validate_source_destination(args.source_root)
        manifest = load_manifest(args.manifest)
        result = sync_source(source_root, manifest, args.check)
    except SourceError as error:
        print(f"Sparkling source sync failed: {error}", file=sys.stderr)
        return 1
    print(f"Sparkling source checkout {result}: {source_root}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
