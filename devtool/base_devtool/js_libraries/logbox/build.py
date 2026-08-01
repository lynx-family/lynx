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
# The resource builders are cross-platform, so an Android build may run both
# this script and devtool/lynx_devtool/resources/lynx-error-parser/build.py.
# Their Android and iOS outputs use separate BaseDevtool and LynxDevtool
# directories, but their Harmony outputs share the LogBox directory above:
# this script owns the LogBox bundle, while the other script independently
# owns lynx-error-parser.js. A module dependency does not necessarily order
# these subprocess-backed resource tasks, and other build entry points have
# their own dependency graphs. Deleting the shared directory can therefore
# race with the parser writer or remove its completed output. Keep cleanup
# ownership-based; if this resource pipeline is redesigned, prefer separate
# staging directories followed by an explicit assembly step.
preserved_target_items = {
    target_paths[2]: {"lynx-error-parser.js"},
}


def clear_target_path(target_path):
    os.makedirs(target_path, exist_ok=True)
    for item in os.listdir(target_path):
        if item in preserved_target_items.get(target_path, set()):
            continue
        item_path = os.path.join(target_path, item)
        if os.path.isdir(item_path):
            shutil.rmtree(item_path)
        else:
            os.remove(item_path)

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
    # Change to the root directory
    os.chdir(root_path)

    # Create the distribution directory if it doesn't exist
    os.makedirs(dist_path, exist_ok=True)

    # Run the pnpm build command
    run_pnpm_command(['pnpm', '--filter', '@lynx-dev/logbox', 'build'],
                     root_path, env)

    if output and os.path.exists(output):
        target_paths.append(output)

    for target_path in target_paths:
        clear_target_path(target_path)

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
