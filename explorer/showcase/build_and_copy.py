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
root_dir = os.path.abspath(os.path.join(current_dir, '../../'))
sys.path.append(root_dir)
from tools.js_tools.pnpm_helper import run_pnpm_command

# Define the Lynx example directory name
LYNX_EXAMPLE_DIR_NAME = "@lynx-example"

# Get the showcase root directory
showcase_root_dir = os.path.dirname(os.path.abspath(__file__))
explorer_dir = os.path.dirname(showcase_root_dir)

# Define Android and iOS asset directories
android_assets_dir = os.path.join(explorer_dir, "android", "lynx_explorer",
                                  "src", "main", "assets")
ios_resource_dir = os.path.join(explorer_dir, "darwin", "ios", "lynx_explorer",
                                "LynxExplorer", "Resource")

# Create Android and iOS asset directories if they don't exist
if not os.path.exists(android_assets_dir):
    os.makedirs(android_assets_dir)
if not os.path.exists(ios_resource_dir):
    os.makedirs(ios_resource_dir)

# Remove existing showcase directories and create new ones
showcase_android = os.path.join(android_assets_dir, "showcase")
showcase_ios = os.path.join(ios_resource_dir, "showcase")
if os.path.exists(showcase_android):
    shutil.rmtree(showcase_android)
if os.path.exists(showcase_ios):
    shutil.rmtree(showcase_ios)
os.makedirs(showcase_android)
os.makedirs(showcase_ios)

print("========== build showcase page ==========")
os.chdir(showcase_root_dir)
# Install dependencies and build
run_pnpm_command(["pnpm", "install", "--frozen-lockfile"], os.getcwd())
run_pnpm_command(["pnpm", "run", "build"], os.getcwd())

print("========== copy showcase resource ==========")
# Copy resources from node_modules
node_modules_example_dir = os.path.join(showcase_root_dir, "node_modules",
                                        LYNX_EXAMPLE_DIR_NAME)
for path in os.listdir(node_modules_example_dir):
    path_android = os.path.join(showcase_android, path)
    path_ios = os.path.join(showcase_ios, path)
    os.makedirs(path_android)
    os.makedirs(path_ios)
    dist_dir = os.path.join(node_modules_example_dir, path, "dist")
    for filename in os.listdir(dist_dir):
        if filename.endswith(".lynx.bundle"):
            shutil.copy(os.path.join(dist_dir, filename), path_android)
            shutil.copy(os.path.join(dist_dir, filename), path_ios)

# Copy menu resources
menu_dist_dir = os.path.join(showcase_root_dir, "menu", "dist")
menu_android = os.path.join(showcase_android, "menu")
menu_ios = os.path.join(showcase_ios, "menu")
os.makedirs(menu_android)
os.makedirs(menu_ios)
for filename in os.listdir(menu_dist_dir):
    if filename.endswith(".lynx.bundle"):
        shutil.copy(os.path.join(menu_dist_dir, filename), menu_android)
        shutil.copy(os.path.join(menu_dist_dir, filename), menu_ios)
