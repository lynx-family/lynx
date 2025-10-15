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

# Add lynx/tools to sys.path to import buildtools_helper
sys.path.append(os.path.join(root_dir, 'lynx', 'tools'))
from buildtools_helper import get_buildtools_path

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
harmony_dir = os.path.join(explorer_dir, "harmony", "lynx_explorer", "src", "main", "resources", "rawfile")

# Create Android and iOS asset directories if they don't exist
if not os.path.exists(android_assets_dir):
    os.makedirs(android_assets_dir)
if not os.path.exists(ios_resource_dir):
    os.makedirs(ios_resource_dir)
if not os.path.exists(harmony_dir):
    os.makedirs(harmony_dir)

# Remove existing showcase directories and create new ones
showcase_android = os.path.join(android_assets_dir, "showcase")
showcase_ios = os.path.join(ios_resource_dir, "showcase")
showcase_harmony = os.path.join(harmony_dir, "showcase")
if os.path.exists(showcase_android):
    shutil.rmtree(showcase_android)
if os.path.exists(showcase_ios):
    shutil.rmtree(showcase_ios)
if os.path.exists(showcase_harmony):
    shutil.rmtree(showcase_harmony)
os.makedirs(showcase_android)
os.makedirs(showcase_ios)
os.makedirs(showcase_harmony)

print("========== build showcase page ==========")
os.chdir(showcase_root_dir)
# Get buildtools path using buildtools_helper
buildtools_path = get_buildtools_path()
if not buildtools_path:
    print("Error: Could not find buildtools directory", file=sys.stderr)
    sys.exit(1)
pnpm_script = os.path.join(buildtools_path, "pnpm", "pnpm")
subprocess.check_call([pnpm_script, "install", "--frozen-lockfile"], cwd=os.getcwd())
subprocess.check_call([pnpm_script, "run", "build"], cwd=os.getcwd())

print("========== copy showcase resource ==========")
# Copy resources from node_modules
node_modules_example_dir = os.path.join(showcase_root_dir, "node_modules",
                                        LYNX_EXAMPLE_DIR_NAME)
for path in os.listdir(node_modules_example_dir):
    path_android = os.path.join(showcase_android, path)
    path_ios = os.path.join(showcase_ios, path)
    path_harmony = os.path.join(showcase_harmony, path)
    os.makedirs(path_android)
    os.makedirs(path_ios)
    os.makedirs(path_harmony)
    dist_dir = os.path.join(node_modules_example_dir, path, "dist")
    for filename in os.listdir(dist_dir):
        if filename.endswith(".lynx.bundle"):
            shutil.copy(os.path.join(dist_dir, filename), path_android)
            shutil.copy(os.path.join(dist_dir, filename), path_ios)
            shutil.copy(os.path.join(dist_dir, filename), path_harmony)

# Copy menu resources
menu_dist_dir = os.path.join(showcase_root_dir, "menu", "dist")
menu_android = os.path.join(showcase_android, "menu")
menu_ios = os.path.join(showcase_ios, "menu")
menu_harmony = os.path.join(showcase_harmony, "menu")
os.makedirs(menu_android)
os.makedirs(menu_ios)
os.makedirs(menu_harmony)
for filename in os.listdir(menu_dist_dir):
    if filename.endswith(".lynx.bundle"):
        shutil.copy(os.path.join(menu_dist_dir, filename), menu_android)
        shutil.copy(os.path.join(menu_dist_dir, filename), menu_ios)
        shutil.copy(os.path.join(menu_dist_dir, filename), menu_harmony)
