#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import argparse
import os
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(
        description="Run a Cargo command for the Starlight Rust workspace."
    )
    parser.add_argument("--workspace-dir", required=True)
    parser.add_argument("--target-dir", required=True)
    parser.add_argument("--stamp", required=True)
    parser.add_argument("cargo_args", nargs=argparse.REMAINDER)
    args = parser.parse_args()

    if not args.cargo_args or args.cargo_args[0] != "--":
        parser.error("expected '--' followed by cargo subcommand arguments")

    cargo_args = args.cargo_args[1:]
    if not cargo_args:
        parser.error("missing cargo subcommand")

    env = os.environ.copy()
    env["CARGO_TARGET_DIR"] = os.path.abspath(args.target_dir)
    absolutize_env_path(env, "STARLIGHT_RUST_FFI_LIBRARY")
    os.makedirs(env["CARGO_TARGET_DIR"], exist_ok=True)

    result = subprocess.run(
        ["cargo", *cargo_args],
        cwd=args.workspace_dir,
        env=env,
        check=False,
    )
    if result.returncode != 0:
        return result.returncode

    os.makedirs(os.path.dirname(args.stamp), exist_ok=True)
    with open(args.stamp, "w", encoding="utf-8") as stamp:
        stamp.write("ok\n")
    return 0


def absolutize_env_path(env, name):
    value = env.get(name)
    if value and not os.path.isabs(value):
        env[name] = os.path.abspath(value)


if __name__ == "__main__":
    sys.exit(main())
