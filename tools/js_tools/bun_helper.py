#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import os
import platform
import subprocess
import sys

current_dir = os.path.dirname(os.path.realpath(__file__))
tools_dir = os.path.abspath(os.path.join(current_dir, "../"))
sys.path.append(tools_dir)
from buildtools_helper import get_buildtools_path


def _resolve_bun_executable() -> str:
    system = platform.system().lower()
    is_win = system == "windows"
    bun_dir = os.path.join(get_buildtools_path(), "bun")
    candidates = [
        os.path.join(bun_dir, "bun.exe" if is_win else "bun"),
    ]
    for entry in os.listdir(bun_dir) if os.path.isdir(bun_dir) else []:
        entry_path = os.path.join(bun_dir, entry)
        if not os.path.isdir(entry_path):
            continue
        candidates.append(os.path.join(entry_path, "bun.exe" if is_win else "bun"))

    for path in candidates:
        if os.path.exists(path):
            return path

    raise FileNotFoundError(f"bun executable not found in: {bun_dir}")


def get_bun_env():
    env = os.environ.copy()
    bun_exe = _resolve_bun_executable()
    env["PATH"] = rf"{os.path.dirname(bun_exe)}{os.pathsep}{os.environ.get('PATH','')}"
    return env


def run_bun_command(command, cwd, env=None):
    if env is None:
        env = get_bun_env().copy()

    bun_command = list(command)
    bun_command[0] = _resolve_bun_executable()
    subprocess.check_call(bun_command, cwd=cwd, env=env)
