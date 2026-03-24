#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
import platform
import subprocess
import sys

current_dir = os.path.dirname(os.path.realpath(__file__))
tools_dir = os.path.abspath(os.path.join(current_dir, '../'))
sys.path.append(tools_dir)
from buildtools_helper import get_buildtools_path

# Get development system
system = platform.system().lower()
is_win = system == "windows"
bun_path = os.path.join(
    get_buildtools_path(),
    "bun",
    "bun.exe" if is_win else "bun",
)


def get_bun_env():
    env = os.environ.copy()
    return env


def run_bun_command(command, cwd, env=None):
    if env is None:
        env = get_bun_env().copy()

    command[0] = bun_path
    subprocess.check_call(command, cwd=cwd, env=env)
