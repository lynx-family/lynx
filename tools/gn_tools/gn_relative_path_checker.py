#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import subprocess
import sys
import os

from gn_relative_path_converter import process_file

COLORED_YELLOW_MSG = '\033[33m'
COLORED_RED_MSG = '\033[31m'
COLORED_PRINT_END = '\033[0m'


def get_changed_files():
    git_diff_cmd = f'git diff --name-only HEAD~ --diff-filter=ACMRT'
    changed_files = subprocess.check_output(
        git_diff_cmd, shell=True).decode('utf-8').rstrip()
    changed_files = changed_files.split('\n')
    return changed_files

def get_lynx_path():
    lynx_path = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    return lynx_path


def process_gn_relative_path():
    """
    Process GN/GNI files in the changed_files list
    """
    gn_file_modified = False
    changed_files = get_changed_files()
    print(changed_files)
    for file in changed_files:
        if file.endswith('.gn') or file.endswith('.gni'):
            file_path = os.path.join(get_lynx_path(), file)
            is_need_process, _ = process_file(file_path, get_lynx_path())
            if is_need_process:
                print(f'{COLORED_RED_MSG}Found gn files with absolute path in {file}. {COLORED_PRINT_END}')
                gn_file_modified = True

    return gn_file_modified


def main():
    if process_gn_relative_path():
        print(' ')
        print(f'{COLORED_YELLOW_MSG}Please run "git status" or "git diff" to check local changes, and then run "git lynx format --changed" to format local changes.{COLORED_PRINT_END}')
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
