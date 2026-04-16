#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import os
import subprocess
import sys
from pathlib import Path


PNPM_WRAPPER = Path(__file__).resolve().with_name("pnpm_wrapper.py")


def get_pnpm_env():
    return os.environ.copy()


def run_pnpm_command(command, cwd, env=None):
    if env is None:
        env = get_pnpm_env().copy()

    args = list(command)
    if args and Path(args[0]).name.startswith("pnpm"):
        args = args[1:]

    subprocess.check_call(
        [sys.executable, str(PNPM_WRAPPER), *args],
        cwd=cwd,
        env=env,
    )
