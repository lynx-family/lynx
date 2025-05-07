#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
import subprocess

# Get the current directory where the script is located
current_dir = os.path.dirname(os.path.abspath(__file__))

# Run the build.sh scripts
scripts = [
    os.path.join(current_dir, 'devtool-switch', 'build.py'),
    os.path.join(current_dir, 'lynx-logbox', 'build.py'),
    os.path.join(current_dir, 'lynx-error-parser', 'build.py'),
    os.path.join(current_dir, 'images', 'copy_images.py')
]

for script in scripts:
    if os.path.exists(script):
        command = ["python3", script]
        subprocess.run(command, check=True)
    else:
        print(f"Script {script} does not exist.")
    