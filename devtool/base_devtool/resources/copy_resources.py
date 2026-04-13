#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
import shutil
import sys
import subprocess
args = sys.argv[1:]
enable_logbox = os.environ.get('ENABLE_DEVTOOL_LOGBOX', 'true').lower() == 'true'
if '--disable-logbox' in args:
    enable_logbox = False
elif '--enable-logbox' in args:
    enable_logbox = True

# Get the current directory where the script is located
current_dir = os.path.dirname(os.path.abspath(__file__))
# Calculate the root path
root_path = os.path.abspath(os.path.join(current_dir, '..', '..', '..'))
# Define the indicated output path
output = next((arg for arg in args if not arg.startswith('--')), None)
# Make sure the output directory exists
if output:
    print(f"Output directory: {output}")
    os.makedirs(output, exist_ok=True)

android_logbox_dir = os.path.join(
    root_path, 'devtool', 'base_devtool', 'android', 'base_devtool', 'src', 'main',
    'assets', 'logbox')

if enable_logbox:
    scripts = [
        os.path.join(root_path, 'devtool', 'base_devtool', 'js_libraries', 'logbox', 'build.py'),
        os.path.join(current_dir, 'images', 'copy_images.py')
    ]

    for script in scripts:
        if os.path.exists(script):
            command = ["python3", script, output] if output else ["python3", script]
            subprocess.run(command, check=True)
        else:
            print(f"Script {script} does not exist.")
else:
    if output and os.path.isdir(output):
        shutil.rmtree(output)
        os.makedirs(output, exist_ok=True)
    if os.path.isdir(android_logbox_dir):
        shutil.rmtree(android_logbox_dir)

# Create a zip archive of the output directory
if output and os.path.isdir(output):
    shutil.make_archive(output, 'zip', output)
