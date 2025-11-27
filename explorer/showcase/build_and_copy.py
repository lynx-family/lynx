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
harmony_dir = os.path.join(explorer_dir, "harmony", "lynx_explorer", "src", "main", "resources", "rawfile")
# Define Windows resource directory
windows_resource_dir = os.path.join(explorer_dir, "windows", "lynx_explorer", "resources")
macos_resource_dir = os.path.join(explorer_dir, "darwin", "macos", "lynx_explorer", "Resource")

print(f"macOS resource directory: {macos_resource_dir}")
print(f"macOS resource directory exists: {os.path.exists(macos_resource_dir)}")

# Create Android and iOS asset directories if they don't exist
if not os.path.exists(android_assets_dir):
    os.makedirs(android_assets_dir)
if not os.path.exists(ios_resource_dir):
    os.makedirs(ios_resource_dir)
if not os.path.exists(harmony_dir):
    os.makedirs(harmony_dir)
# Create Windows/macos resource directory if it doesn't exist
if not os.path.exists(windows_resource_dir):   
    os.makedirs(windows_resource_dir)
if not os.path.exists(macos_resource_dir):
    os.makedirs(macos_resource_dir)

# Remove existing showcase directories and create new ones
showcase_android = os.path.join(android_assets_dir, "showcase")
showcase_ios = os.path.join(ios_resource_dir, "showcase")
showcase_harmony = os.path.join(harmony_dir, "showcase")
# Define Windows/macos showcase directory
showcase_windows = os.path.join(windows_resource_dir, "showcase")
showcase_macos = os.path.join(macos_resource_dir, "showcase")  

if os.path.exists(showcase_android):
    shutil.rmtree(showcase_android)
if os.path.exists(showcase_ios):
    shutil.rmtree(showcase_ios)
if os.path.exists(showcase_harmony):
    shutil.rmtree(showcase_harmony)
if os.path.exists(showcase_windows):
    shutil.rmtree(showcase_windows)
if os.path.exists(showcase_macos):
    shutil.rmtree(showcase_macos)   

os.makedirs(showcase_android)
os.makedirs(showcase_ios)
os.makedirs(showcase_harmony)
os.makedirs(showcase_windows)
os.makedirs(showcase_macos)

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
    path_harmony = os.path.join(showcase_harmony, path)
    path_windows = os.path.join(showcase_windows, path)
    path_macos = os.path.join(showcase_macos, path)
    os.makedirs(path_android)
    os.makedirs(path_ios)
    os.makedirs(path_harmony)
    os.makedirs(path_windows)
    os.makedirs(path_macos)

    dist_dir = os.path.join(node_modules_example_dir, path, "dist")
    for filename in os.listdir(dist_dir):
        if filename.endswith(".lynx.bundle"):
            shutil.copy(os.path.join(dist_dir, filename), path_android)
            shutil.copy(os.path.join(dist_dir, filename), path_ios)
            shutil.copy(os.path.join(dist_dir, filename), path_harmony)
            shutil.copy(os.path.join(dist_dir, filename), path_windows)
            shutil.copy(os.path.join(dist_dir, filename), path_macos)

# Copy menu resources
menu_dist_dir = os.path.join(showcase_root_dir, "menu", "dist")

menu_android = os.path.join(showcase_android, "menu")
menu_ios = os.path.join(showcase_ios, "menu")
menu_harmony = os.path.join(showcase_harmony, "menu")
menu_windows = os.path.join(showcase_windows, "menu")
menu_macos = os.path.join(showcase_macos, "menu")

print(f"Creating menu directories")
os.makedirs(menu_android)
os.makedirs(menu_ios)
os.makedirs(menu_harmony)
os.makedirs(menu_windows)
os.makedirs(menu_macos)
for filename in os.listdir(menu_dist_dir):
    if filename.endswith(".lynx.bundle"):
        shutil.copy(os.path.join(menu_dist_dir, filename), menu_android)
        shutil.copy(os.path.join(menu_dist_dir, filename), menu_ios)
        shutil.copy(os.path.join(menu_dist_dir, filename), menu_harmony)
        shutil.copy(os.path.join(menu_dist_dir, filename), menu_windows)
        shutil.copy(os.path.join(menu_dist_dir, filename), menu_macos)
