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
from tools.js_tools.pnpm_helper import get_pnpm_env, run_pnpm_command

# Define the distribution path
dist_path = os.path.join(current_dir, 'dist')

# Define the indicated output path
output = sys.argv[1] if len(sys.argv) > 1 else None

# Define target paths
target_paths = [
    os.path.join(root_path, 'devtool', 'base_devtool', 'android', 'base_devtool', 'src', 'main', 'assets', 'logbox'),
    os.path.join(root_path, 'devtool', 'base_devtool', 'darwin', 'ios', 'assets', 'logbox'),
    os.path.join(root_path, 'platform', 'harmony', 'lynx_devtool', 'src', 'main', 'resources', 'rawfile', 'logbox'),
    os.path.join(root_path, 'platform', 'darwin', 'macos', 'lynx_devtool', 'assets', 'logbox'),
]

def git_root_dir():
    command = ['git', 'rev-parse', '--show-toplevel']
    p = subprocess.Popen(' '.join(command),
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE,
                         shell=True)
    result, error = p.communicate()
    return result.decode('utf-8').strip()

def build():
    env = get_pnpm_env()
    env['COREPACK_HOME'] = os.path.join(git_root_dir(), 'buildtools', 'corepack')
    env['COREPACK_ENABLE_NETWORK'] = '0'
    # Change to the root directory
    os.chdir(root_path)

    # Create the distribution directory if it doesn't exist
    os.makedirs(dist_path, exist_ok=True)

    # Run the pnpm build command
    run_pnpm_command(['pnpm', '--filter', '@lynx-dev/logbox', 'build'],
                     root_path, env)

    if output and os.path.exists(output):
        target_paths.append(output)

    # Remove existing target directories and create new ones
    for target_path in target_paths:
        if os.path.exists(target_path):
            shutil.rmtree(target_path)
        os.makedirs(target_path, exist_ok=True)

    # Copy the contents of the distribution directory to all target directories
    for item in os.listdir(dist_path):
        s = os.path.join(dist_path, item)
        for target_path in target_paths:
            d = os.path.join(target_path, item)
            if os.path.isdir(s):
                shutil.copytree(s, d)
            else:
                shutil.copy2(s, d)

if __name__ == "__main__":
    build()
