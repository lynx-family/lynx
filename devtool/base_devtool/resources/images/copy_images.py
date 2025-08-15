#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
import shutil

# Get the current directory
current_dir = os.path.dirname(os.path.abspath(__file__))

# Calculate the root path
root_path = os.path.abspath(os.path.join(current_dir, '..', '..', '..', '..'))

# Define the source path
src_path = os.path.join(current_dir, 'notification_cancel.png')

# Define the Android target path
android_target_path = os.path.join(root_path, 'devtool', 'base_devtool', 'android', 'base_devtool', 'src', 'main', 'res', 'drawable')

# Define the iOS target path
ios_target_path = os.path.join(root_path, 'devtool', 'base_devtool', 'darwin', 'ios', 'assets')

# Define the macOS target path
macos_target_path = os.path.join(root_path, 'platform', 'darwin', 'macos', 'lynx_devtool', 'assets')

# Define the harmony target path
harmony_target_path = os.path.join(root_path, 'platform', 'harmony', 'lynx_devtool', 'src', 'main', 'resources', 'base', 'media')

def copy_images():
    # Create the target directories if they don't exist
    os.makedirs(android_target_path, exist_ok=True)
    os.makedirs(ios_target_path, exist_ok=True)
    os.makedirs(harmony_target_path, exist_ok=True)
    os.makedirs(macos_target_path, exist_ok=True)

    # Copy the image to the Android target path
    shutil.copy2(src_path, android_target_path)

    # Copy the image to the iOS target path
    shutil.copy2(src_path, ios_target_path)
    
    # Copy the image to the iOS target path
    shutil.copy2(src_path, harmony_target_path)

    # Copy the image to the macOS target path
    shutil.copy2(src_path, macos_target_path)


if __name__ == "__main__":
    copy_images()