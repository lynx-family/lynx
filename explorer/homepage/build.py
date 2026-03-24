#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import os
import subprocess
import sys

# Get the directory where the current script is located
current_dir = os.path.dirname(os.path.realpath(__file__))
# Get the root directory
root_dir = os.path.abspath(os.path.join(current_dir, '../../'))
sys.path.append(root_dir)
from tools.js_tools.bun_helper import run_bun_command

# Install dependencies and build
run_bun_command(["bun", "install", "--frozen-lockfile"], os.getcwd())
run_bun_command(["bun", "run", "build"], os.getcwd())
