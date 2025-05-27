#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import argparse
import os
import platform
import subprocess
import sys
import shutil

current_dir = os.path.dirname(os.path.abspath(__file__))
lynx_dir = os.path.dirname(os.path.dirname(current_dir))

requirements_path = os.path.join(current_dir, "requirements.txt")

system = platform.system()

def create_venv(python_bin_path, root_dir):
  venv_path = os.path.join(root_dir, ".venv")
  if os.path.exists(venv_path):
    for dirpath, dirnames, filenames in os.walk(python_bin_path):
      for filename in filenames:
        if filename in ['python', 'python.exe']:
          print("python venv already exists, reuse it. venv path: ", venv_path)
          return True
  print("python venv not exists, create it. venv path: ", venv_path)
  try:
    cmd_prefix = ''
    # avoid conflict with other venv like mechanism.
    # TODO(yongjie): delete it later.
    if system != "Windows":
      cmd_prefix = 'unset PYTHONPATH && '
    # Use python.exe to create venv on windows, to avoid the problem that "The system cannot find the file specified".
    cmd = f'{cmd_prefix}"{sys.executable.replace("python3.exe", "python.exe")}" -m venv {venv_path}'
    subprocess.run(cmd, check=True, shell=True, stderr=subprocess.PIPE)
    return True
  except subprocess.CalledProcessError as e:
    print(f"Failed to create virtual environment of python. Error: {e.stderr.decode('utf-8')}")
    return False

def copy_python3_exe(root_dir):
  python_exe = os.path.join(root_dir, ".venv", "Scripts", "python.exe")
  python3_exe_dir = os.path.join(root_dir, ".venv", "bin")
  python3_exe_path = os.path.join(python3_exe_dir, "python3.exe")
  if os.path.exists(python3_exe_path):
    return
  if not os.path.exists(python3_exe_dir):
    os.makedirs(python3_exe_dir, exist_ok=True)
  if os.path.exists(python_exe):
    shutil.copy(python_exe, python3_exe_path)

def install_requirements(python_bin_path, python_package_index, root_dir):
  python_path = os.path.join(python_bin_path, "python")
  index_url = ''
  if python_package_index:
    index_url = f'-i {python_package_index}'
  cmd = f'{python_path} -m pip install -r {requirements_path} {index_url}'
  try:
    if system == "Windows":
      copy_python3_exe(root_dir)
    subprocess.run(cmd, check=True, shell=True, stderr=subprocess.PIPE)
  except subprocess.CalledProcessError as e:
    venv_path = os.path.join(root_dir, ".venv")
    print(f"Failed to install requirements for python venv({venv_path}). Error: {e.stderr.decode('utf-8')}")
    return -1
  return 0

def main():
  parser = argparse.ArgumentParser(description='Setup Python virtual environment.')
  parser.add_argument('python_package_index', nargs='?', default='', help='Python package index URL')
  parser.add_argument('--root_dir', default=os.path.dirname(lynx_dir), help='Root directory for the virtual environment')
  args = parser.parse_args()
  print("python_package_index: ", args.python_package_index)
  print("root_dir: ", args.root_dir)

  python_bin_path = os.path.join(args.root_dir, ".venv", "bin") if system != "Windows" else os.path.join(args.root_dir, ".venv", "Scripts")
  if create_venv(python_bin_path, args.root_dir):
      return install_requirements(python_bin_path, args.python_package_index, args.root_dir)
  return 0

if __name__ == "__main__":
  main()
