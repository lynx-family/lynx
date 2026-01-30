#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
import sys
import shutil

# Get the current directory
current_dir = os.path.dirname(os.path.abspath(__file__))

# Calculate the root path
root_path = os.path.abspath(os.path.join(current_dir, '..', '..', '..', '..'))

# Define the source path
src_path = os.path.join(current_dir, 'notification_cancel.png')

# Define the indicated output path
output = sys.argv[1] if len(sys.argv) > 1 else None

# Windows does not need this image because it draws the button by code.
target_paths = [
    os.path.join(root_path, 'devtool', 'base_devtool', 'android', 'base_devtool', 'src', 'main', 'res', 'drawable'),
    os.path.join(root_path, 'devtool', 'base_devtool', 'darwin', 'ios', 'assets'),
    os.path.join(root_path, 'platform', 'darwin', 'macos', 'lynx_devtool', 'assets'),
    os.path.join(root_path, 'platform', 'harmony', 'lynx_devtool', 'src', 'main', 'resources', 'base', 'media')
]

def copy_images():
    for (target_path) in target_paths:
        os.makedirs(target_path, exist_ok=True)
        shutil.copy2(src_path, target_path)
    if output and os.path.isdir(output):
        shutil.copy2(src_path, output)

if __name__ == "__main__":
    copy_images()