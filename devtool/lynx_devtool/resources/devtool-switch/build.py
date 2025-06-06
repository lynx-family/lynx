#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
import shutil
import sys

# Set the script exit mode, equivalent to set -e in bash
sys.tracebacklimit = 0

# Get the directory where the current script is located
current_dir = os.path.dirname(os.path.realpath(__file__))
# Get the root directory
root_dir = os.path.abspath(os.path.join(current_dir, '../../../../'))
sys.path.append(root_dir)
from tools.js_tools.pnpm_helper import run_pnpm_command

template_dir = "dist/devtoolSwitch.lynx.bundle"
android_target_dir = os.path.join(
    root_dir, "platform/android/lynx_devtool/src/main/assets")
# TODO(mitchilling): remove this deprecated duplication after applications adapt to the new path.
android_target_dir_deprecated = os.path.join(
    root_dir, "platform/android/lynx_devtool/src/main/assets/devtool_switch")
ios_target_dir = os.path.join(root_dir,
                              "platform/darwin/ios/lynx_devtool/assets")
switch_page_dir = "switchPage/"

# Get command-line arguments
output = sys.argv[1] if len(sys.argv) > 1 else None

print("========== build devtool switch page ==========")
# Change to the current directory
os.chdir(current_dir)
# Execute the pnpm build command
run_pnpm_command(["pnpm", "build"], current_dir)
bundle_path = os.path.join(current_dir, template_dir)

print("========== copy devtool switch resource ==========")
if output:
    # TODO(mitchilling): need validation for path trespass
    output_path = os.path.join(output, switch_page_dir)
    print(output_path)
    # Create the output directory
    os.makedirs(output_path, exist_ok=True)
    # Copy the file
    shutil.copy(bundle_path, output_path)

# Delete the old target directory
android_switch_page_path = os.path.join(android_target_dir, switch_page_dir)
android_switch_page_path_deprecated = os.path.join(android_target_dir_deprecated, switch_page_dir)
ios_switch_page_path = os.path.join(ios_target_dir, switch_page_dir)
if os.path.exists(android_switch_page_path):
    shutil.rmtree(android_switch_page_path)
if os.path.exists(android_switch_page_path_deprecated):
    shutil.rmtree(android_switch_page_path_deprecated)
if os.path.exists(ios_switch_page_path):
    shutil.rmtree(ios_switch_page_path)

# Create the new target directory
os.makedirs(android_switch_page_path, exist_ok=True)
os.makedirs(android_switch_page_path_deprecated, exist_ok=True)
os.makedirs(ios_switch_page_path, exist_ok=True)

# Copy the file to the target directory
shutil.copy(bundle_path, android_switch_page_path)
shutil.copy(bundle_path, android_switch_page_path_deprecated)
shutil.copy(bundle_path, ios_switch_page_path)
