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

# Add lynx/tools to sys.path to import buildtools_helper
sys.path.append(os.path.join(root_dir, 'tools'))
from buildtools_helper import get_buildtools_path

# Use pnpm wrapper script
pnpm_wrapper = os.path.join(root_dir, 'tools', 'js_tools', 'pnpm_wrapper.py')
subprocess.check_call([sys.executable, pnpm_wrapper, 'install', '--frozen-lockfile'], cwd=os.getcwd())
subprocess.check_call([sys.executable, pnpm_wrapper, 'build'], cwd=os.getcwd())
