#!/usr/bin/env python3
# Copyright 2023 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

"""
usage: python3 tools/oliver/lynx-tasm/gn_to_cmake_oliver.py

This script generate oliver/lynx-tasm/CMakeLists.txt from lepus_cmd_exec targets.

"""

import argparse
import logging
import os
import platform
import shutil
import subprocess
import sys

CUR_FILE_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_PATH = os.path.join(CUR_FILE_DIR, '..', '..', '..', '..')

machine = platform.machine().lower()
machine = "x64" if machine == "x86_64" else machine

def copy_cmake_files(main_target):
  target_path = main_target.split(':')[0].replace('//', '')
  print("target_path: ", target_path)
  dest_full_path = os.path.join(ROOT_PATH, target_path)
  file_full_path = os.path.join(dest_full_path, 'CMakeLists_impl', 'oliver_lynx_tasm', 'CMakeLists.txt')
  shutil.copy(file_full_path, dest_full_path)

def gen_cmake_file(main_target, platform, debug, output_path):
  gn_path = os.path.join(ROOT_PATH, 'lynx', 'tools','gn_tools', 'gn')
  is_debug = 'true' if debug else 'false'

  args = 'disallow_undefined_symbol=false enable_security_protection=false use_flutter_cxx=false \
          is_debug=%s oliver_type=\\\"%s\\\" build_lepus_compile=true enable_air=false enable_inspector=true \
          enable_inspector_test=true compile_lepus_cmd=true build_lynx_lepus_node=true' % (is_debug, 'tasm')
  if platform == 'linux':
    args += ' emsdk_dir=\\\"/root/emsdk\\\"'
    
  if len(machine) > 0:
    args += " target_cpu=\\\"%s\\\"" % (machine)
  set_cmake_target = '--cmake-target=%s' % (main_target) if main_target else ''
  command = "{} gen {} --args=\"{}\" --ide=cmake {}".format(gn_path, output_path, args, set_cmake_target)

  print(command)
  result = subprocess.check_call(command, shell=True)
  return result

def generate(main_target, cmake_version, project_name, platform, debug):

  gn_out_path = os.path.join(ROOT_PATH, 'out/oliver_lynx_tasm')
  r = gen_cmake_file(main_target, platform, debug, gn_out_path)
  if r != 0:
    logging.error("generate cmake script failed!!")

  copy_cmake_files(main_target)

  return r

def main():
  main_target = '//lynx/oliver/lynx-tasm:lepus_cmd_exec'
  cmake_version = '3.0'
  project_name = 'lepus'
  platform = 'darwin'
  debug = True
  return generate(main_target, cmake_version, project_name, platform, debug)

if __name__ == "__main__":
  sys.exit(main())
