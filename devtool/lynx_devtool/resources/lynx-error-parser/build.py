#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
import shutil
import subprocess
import sys

# Get the current directory of the script
current_dir = os.path.dirname(os.path.realpath(__file__))

# Calculate the root path
root_path = os.path.abspath(os.path.join(current_dir, '..', '..', '..', '..'))

sys.path.append(root_path)
from tools.js_tools.pnpm_helper import run_pnpm_command

# Define the distribution path
dist_path = os.path.join(root_path, 'devtool', 'lynx_devtool', 'resources',
                         'lynx-error-parser', 'dist', 'static', 'js')

# Define the Android target path
android_target_path = os.path.join(root_path, 'platform', 'android',
                                   'lynx_devtool', 'src', 'main', 'assets',
                                   'logbox')

# Define the iOS target path
ios_target_path = os.path.join(root_path, 'platform', 'darwin', 'ios',
                               'lynx_devtool', 'assets', 'logbox')

# Define the harmony target path
harmony_target_path = os.path.join(root_path, 'platform', 'harmony', 'lynx_devtool', 'src', 'main', 'resources', 'rawfile', 'logbox')

# Define the macOS target path
macos_target_path = os.path.join(root_path, 'platform', 'darwin', 'macos', 'lynx_devtool', 'assets', 'logbox')

def build():
    # Change to the root directory
    os.chdir(root_path)

    # Create the distribution directory if it doesn't exist
    os.makedirs(dist_path, exist_ok=True)

    # Run the pnpm build command
    run_pnpm_command(['pnpm', '--filter', '@lynx-dev/lynx-error-parser', 'build'],
                     root_path)

    # Create the Android and iOS target directories
    os.makedirs(android_target_path, exist_ok=True)
    os.makedirs(ios_target_path, exist_ok=True)
    os.makedirs(harmony_target_path, exist_ok=True)
    os.makedirs(macos_target_path, exist_ok=True)

    # Copy the file to the target directory
    shutil.copy(os.path.join(dist_path, "lynx-error-parser.js"), android_target_path)
    shutil.copy(os.path.join(dist_path, "lynx-error-parser.js"), ios_target_path)
    shutil.copy(os.path.join(dist_path, "lynx-error-parser.js"), harmony_target_path)
    shutil.copy(os.path.join(dist_path, "lynx-error-parser.js"), macos_target_path)

if __name__ == "__main__":
    build()
