#!/usr/bin/env python3
# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import argparse
import os
import subprocess
import sys

current_dir = os.path.dirname(os.path.realpath(__file__))
tools_dir = os.path.abspath(os.path.join(current_dir, '../'))
sys.path.append(tools_dir)
from buildtools_helper import get_buildtools_path

def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('--ide', help='Specify the output file type. cmake or podspec :Converts the cmake_target/podspec_target into the corresponding cmake/podspec script)')
  parser.add_argument('--cmake-target', help='Specifies a single cmake_target to convert into a cmake script')
  parser.add_argument('--podspec-target', help='Specifies a single podspec_target to convert into a podfile script')
  parser.add_argument('--args', help='GN build arguments')
  args, remaining_args = parser.parse_known_args()
  
  file_type = args.ide
  cmake_target = args.cmake_target
  podspec_target = args.podspec_target
  gn_args = args.args.split(" ") if args.args else []
  new_gn_args = [item for item in gn_args if item]
  gn_args = new_gn_args
  
  gn_cmd = 'gn'
  if sys.platform.startswith(('cygwin', 'win')):
    gn_cmd += ".exe"
  cmd = [f'{os.path.join(get_buildtools_path(), "gn", gn_cmd)}']
  for new_arg in remaining_args:
    cmd.append(new_arg.replace('"', '\\"'))
  cmd.append('--args=%s' % ' '.join(gn_args))
  
  if file_type == "cmake":
    cmd.append('--ide=json')
    gn_to_cmake_script_path = os.path.join(current_dir, "gn_to_cmake_script.py")
    cmd.append(f'--json-ide-script={gn_to_cmake_script_path}')
    if cmake_target:
      cmd.append(f"--json-ide-script-args={cmake_target}")
  elif file_type == "podspec":
    cmd.append('--ide=json')
    gn_to_podspec_script_path = os.path.join(current_dir, "gn_to_podspec_script.py")
    cmd.append(f'--json-ide-script={gn_to_podspec_script_path}')
    if podspec_target:
      cmd.append(f"--json-ide-script-args={podspec_target}")
  else:
    if file_type:
      cmd.append(f"--ide={file_type}")
  return subprocess.call(cmd)

if __name__ == "__main__":
  sys.exit(main())