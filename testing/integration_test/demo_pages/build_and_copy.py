#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
import shutil
import subprocess
import sys

# Get the directory where the current script is located
current_dir = os.path.dirname(os.path.realpath(__file__))
# Get the root directory
root_dir = os.path.abspath(os.path.join(current_dir, '../../../'))
sys.path.append(root_dir)
from tools.js_tools.pnpm_helper import run_pnpm_command

# Get the directory of the current script
script_dir = os.path.dirname(os.path.abspath(__file__))

# Calculate the explorer directory
explorer_dir = os.path.abspath(os.path.join(script_dir, '..', '..', '..'))
print(f"explorer_dir: {explorer_dir}")

# Define the Android assets directory
android_assets_dir = os.path.join(explorer_dir, 'explorer', 'android',
                                  'lynx_explorer', 'src', 'main', 'assets')
if not os.path.exists(android_assets_dir):
    os.makedirs(android_assets_dir)

# Define the iOS resource directory
ios_resource_dir = os.path.join(explorer_dir, 'explorer', 'darwin', 'ios',
                                'lynx_explorer', 'LynxExplorer', 'Resource')
if not os.path.exists(ios_resource_dir):
    os.makedirs(ios_resource_dir)

# Clean up and create the automation directories
automation_android = os.path.join(android_assets_dir, 'automation')
if os.path.exists(automation_android):
    shutil.rmtree(automation_android)
os.makedirs(automation_android)

automation_ios = os.path.join(ios_resource_dir, 'automation')
if os.path.exists(automation_ios):
    shutil.rmtree(automation_ios)
os.makedirs(automation_ios)

print("========== build integration test demo pages ==========")
# Install dependencies and build
run_pnpm_command(["pnpm", "install", "--frozen-lockfile"], os.getcwd())
run_pnpm_command(["pnpm", "run", "build"], os.getcwd())

print("========== copy integration test demo pages resource==========")
# Iterate through directories in the current script directory
for item in os.listdir(script_dir):
    item_path = os.path.join(script_dir, item)
    if os.path.isdir(item_path) and item != 'node_modules':
        android_dest = os.path.join(automation_android, item)
        ios_dest = os.path.join(automation_ios, item)
        os.makedirs(android_dest, exist_ok=True)
        os.makedirs(ios_dest, exist_ok=True)
        dist_dir = os.path.join(item_path, 'dist')
        if os.path.exists(dist_dir):
            for filename in os.listdir(dist_dir):
                if filename.endswith('.lynx.bundle'):
                    src_file = os.path.join(dist_dir, filename)
                    shutil.copy(src_file, android_dest)
                    shutil.copy(src_file, ios_dest)
