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
root_dir = os.path.abspath(os.path.join(current_dir, '../../../'))

# Add lynx/tools to sys.path to import buildtools_helper
sys.path.append(os.path.join(root_dir, 'lynx', 'tools'))
from buildtools_helper import get_buildtools_path

# Get buildtools path using buildtools_helper
buildtools_path = get_buildtools_path()
if not buildtools_path:
    print("Error: Could not find buildtools directory", file=sys.stderr)
    sys.exit(1)
pnpm_script = os.path.join(buildtools_path, "pnpm", "pnpm")
subprocess.check_call([pnpm_script, 'install', '--frozen-lockfile'], cwd=os.getcwd())
subprocess.check_call([pnpm_script, 'build'], cwd=os.getcwd())
