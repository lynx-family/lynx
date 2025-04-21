#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import os
import platform
import subprocess
import sys
import shutil

current_dir = os.path.dirname(os.path.abspath(__file__))
lynx_dir = os.path.dirname(os.path.dirname(current_dir))
root_dir = os.path.dirname(lynx_dir)

VENV_PATH = os.path.join(root_dir, ".venv")
requirements_path = os.path.join(current_dir, "requirements.txt")

system = platform.system()

def create_venv():
  if os.path.exists(VENV_PATH):
    return True
  try:
    cmd_prefix = ''
    # avoid conflict with other venv like mechanism.
    # TODO(yongjie): delete it later.
    if system == "Windows":
      site_packages_path = os.path.join(VENV_PATH, "Lib", "site-packages")
      cmd_prefix = f'set PYTHONPATH= & '
    else:
      cmd_prefix = 'unset PYTHONPATH && '
    cmd = f'{cmd_prefix}"{sys.executable}" -m venv {VENV_PATH}'
    subprocess.run(cmd, check=True, shell=True, stderr=subprocess.PIPE)
    return True
  except subprocess.CalledProcessError as e:
    print(f"Failed to create virtual environment of python. Error: {e.stderr.decode('utf-8')}")
    return False

def command_separator():
  if system == "Windows":
    return "&"
  return "&&"

def copy_python3_exe():
  python_exe = os.path.join(VENV_PATH, "Scripts", "python.exe")
  python3_exe_dir = os.path.join(VENV_PATH, "bin")
  python3_exe_path = os.path.join(python3_exe_dir, "python3.exe")
  if not os.path.exists(python3_exe_dir):
    os.makedirs(python3_exe_dir, exist_ok=True)
  if os.path.exists(python_exe):
    shutil.copy(python_exe, python3_exe_path)

def install_requirements(python_package_index):
  python_bin_path = os.path.join(VENV_PATH, "bin") if system!= "Windows" else os.path.join(VENV_PATH, "Scripts")
  activate_path = os.path.join(python_bin_path, "activate")
  python_path = os.path.join(python_bin_path, "python")
  index_url = ''
  if python_package_index:
    index_url = f'-i {python_package_index}'
  cmd = f'. {activate_path} {command_separator()} {python_path} -m pip install -r {requirements_path} {index_url}'
  try:
    subprocess.run(cmd, check=True, shell=True, stderr=subprocess.PIPE)
    if system == "Windows":
      copy_python3_exe()
  except subprocess.CalledProcessError as e:
    print(f"Failed to install requirements for python venv({VENV_PATH}). Error: {e.stderr.decode('utf-8')}")
    return -1
  return 0

def main():
  if create_venv():
    args = sys.argv[1:]
    python_package_index = args[0] if len(args) > 0 else ''
    return install_requirements(python_package_index)
  return 0

if __name__ == "__main__":
  main()
