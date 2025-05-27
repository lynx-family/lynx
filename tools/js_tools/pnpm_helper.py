#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
import platform
import subprocess

# Get the directory where the current script is located
current_dir = os.path.dirname(os.path.realpath(__file__))
# Get the root directory
lynx_dir = os.path.abspath(os.path.join(current_dir, '../../'))
root_dir = os.path.abspath(os.path.join(lynx_dir, '../'))
# Get development system
system = platform.system().lower()
is_win = system == "windows"
node_bin_path = os.path.join(
    lynx_dir, "buildtools/node" if is_win else "buildtools/node/bin")
if not os.path.exists(node_bin_path):
    node_bin_path = os.path.join(
        root_dir, "buildtools/node" if is_win else "buildtools/node/bin")


def get_pnpm_env():
    env = os.environ.copy()
    env["PATH"] = rf"{node_bin_path}{os.pathsep}{os.environ['PATH']}"
    return env


def run_pnpm_command(command, cwd, env=get_pnpm_env()):
    command[0] = os.path.join(node_bin_path, "pnpm.CMD" if is_win else "pnpm")
    subprocess.check_call(
        command,
        cwd=cwd,
        env=env,
    )
