#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

"""Materialize and validate Explorer's pinned Sparkling source checkout."""

import argparse
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

from sparkling_source_validation import (
    load_manifest as load_manifest_errors,
    validate_checkout as validate_checkout_errors,
    validate_source_destination as validate_source_destination_errors,
)


class SourceError(RuntimeError):
    pass


def validate_source_destination(source_root, project_dir=None, environ=None):
    """Reject mutable package-manager locations before any checkout writes."""
    project_dir = (project_dir or _project_dir()).resolve(strict=False)
    source_root, errors = validate_source_destination_errors(
        source_root,
        project_dir,
        environ,
    )
    if errors:
        raise SourceError(errors[0])
    return source_root


def load_manifest(manifest_path):
    manifest, errors = load_manifest_errors(manifest_path)
    if errors:
        raise SourceError(errors[0])
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
    errors = validate_checkout_errors(source_root, manifest)
    if errors:
        raise SourceError(errors[0])


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
