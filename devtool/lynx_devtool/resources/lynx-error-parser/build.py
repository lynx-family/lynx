#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
import shutil
import subprocess
import sys
import platform

# Get the current directory of the script
current_dir = os.path.dirname(os.path.realpath(__file__))

# Calculate the root path
root_path = os.path.abspath(os.path.join(current_dir, '..', '..', '..', '..'))

# Add lynx/tools to sys.path to import buildtools_helper
sys.path.append(os.path.join(root_path, 'tools'))
from buildtools_helper import get_buildtools_path

# Define the distribution path
dist_path = os.path.join(root_path, 'devtool', 'lynx_devtool', 'resources',
                         'lynx-error-parser', 'dist', 'static', 'js')

# Define the indicated output path
output = sys.argv[1] if len(sys.argv) > 1 else None

# Define target paths
target_paths = [
    os.path.join(root_path, 'platform', 'android', 'lynx_devtool', 'src', 'main', 'assets', 'logbox'),
    os.path.join(root_path, 'platform', 'darwin', 'ios', 'lynx_devtool', 'assets', 'logbox'),
    os.path.join(root_path, 'platform', 'harmony', 'lynx_devtool', 'src', 'main', 'resources', 'rawfile', 'logbox'),
]

def build():
    # Change to the root directory
    os.chdir(root_path)

    # Create the distribution directory if it doesn't exist
    os.makedirs(dist_path, exist_ok=True)

    # Run the pnpm build command via wrapper
    pnpm_wrapper = os.path.join(root_path, 'tools', 'js_tools', 'pnpm_wrapper.py')
    subprocess.check_call([sys.executable, pnpm_wrapper, '--filter', '@lynx-dev/lynx-error-parser', 'build'],
                         cwd=root_path)

    # Create target directories and copy file
    source_file = os.path.join(dist_path, "lynx-error-parser.js")
    for target_path in target_paths:
        os.makedirs(target_path, exist_ok=True)
        shutil.copy(source_file, target_path)
    if output and os.path.isdir(output):
        shutil.copy(source_file, output)

if __name__ == "__main__":
    build()
