#!/usr/bin/env python3
# Copyright 2023 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

"""
usage: python3 tools/oliver/lynx-tasm/gn_to_cmake_oliver.py

This script generate oliver/lynx-tasm/CMakeLists.txt from lepus_cmd_exec targets.

"""

import logging
import os
import platform
import subprocess
import sys

CUR_FILE_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_PATH = os.path.join(CUR_FILE_DIR, '..', '..', '..', '..')

machine = platform.machine().lower()
machine = "x64" if machine == "x86_64" else machine

def gen_cmake_file(main_target, root_path, platform, gn_args=''):
  gn_path = os.path.join(root_path, 'lynx', 'tools','gn_tools', 'gn')
  if platform == 'linux':
    gn_args += ' emsdk_dir=\\\"/root/emsdk\\\"'
    
  if len(machine) > 0:
    gn_args += " target_cpu=\\\"%s\\\"" % (machine)
  set_cmake_target = '--cmake-target=%s' % (main_target) if main_target else ''
  gn_out_path = os.path.join(root_path, 'out/oliver_lynx_tasm')
  command = "{} gen {} --args=\"{}\" --ide=cmake {}".format(gn_path, gn_out_path, gn_args, set_cmake_target)

  print(command)
  result = subprocess.check_call(command, shell=True)
  if result != 0:
    logging.error("generate cmake script failed!!")
  return result

def main():
  main_target = '//lynx/oliver/lynx-tasm:lepus_cmd_exec'
  platform = 'darwin'
  gn_args = 'disallow_undefined_symbol=false enable_security_protection=false use_flutter_cxx=false \
          is_debug=true oliver_type=\\\"tasm\\\" build_lepus_compile=true enable_air=false enable_inspector=true \
          enable_inspector_test=true compile_lepus_cmd=true build_lynx_lepus_node=true'
  
  return gen_cmake_file(main_target, ROOT_PATH, platform, gn_args)

if __name__ == "__main__":
  sys.exit(main())
