#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import argparse
import os
import shutil
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(
        description="Build a Cargo staticlib artifact for the Starlight Rust workspace."
    )
    parser.add_argument("--workspace-dir", required=True)
    parser.add_argument("--target-dir", required=True)
    parser.add_argument("--package", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    target_dir = os.path.abspath(args.target_dir)
    env = os.environ.copy()
    env["CARGO_TARGET_DIR"] = target_dir
    os.makedirs(target_dir, exist_ok=True)

    result = subprocess.run(
        ["cargo", "build", "-p", args.package, "--release"],
        cwd=args.workspace_dir,
        env=env,
        check=False,
    )
    if result.returncode != 0:
        return result.returncode

    try:
        artifact = find_staticlib(target_dir, args.package)
    except FileNotFoundError as error:
        print(error, file=sys.stderr)
        return 1

    os.makedirs(os.path.dirname(args.output), exist_ok=True)
    shutil.copyfile(artifact, args.output)
    return 0


def find_staticlib(target_dir, package):
    release_dir = os.path.join(target_dir, "release")
    candidates = [
        os.path.join(release_dir, f"lib{package}.a"),
        os.path.join(release_dir, f"{package}.lib"),
    ]
    for candidate in candidates:
        if os.path.exists(candidate):
            return candidate
    raise FileNotFoundError(
        f"cargo build succeeded, but no staticlib for {package} was found in {release_dir}"
    )


if __name__ == "__main__":
    sys.exit(main())
