#!/usr/bin/env python3
# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

"""
This script help us quickly generate podspec files by gn script.

usage: generate_podspec_scripts_by_gn.py [-h] [--target TARGET] [--is-debug] [--enable-trace]

The optional value of TARGET can specify the GN target to generate it's podspec file. 
If the value of TARGET is not specified, all podspecs will be generated.
"""

import argparse
import os
import subprocess
import sys

def clean_gn_project_json_file(gn_out_dir):
  for dir in os.listdir(gn_out_dir):
    project_json_file = os.path.join(gn_out_dir, dir, 'project.json')
    if os.path.exists(project_json_file):
      os.remove(project_json_file)

def generate_compile_products(root_path, args, gn_args):
  is_debug = args.is_debug
  enable_trace = args.enable_trace
  target = args.target
    

  gn_args += f' target_os=\\\"ios\\\" '
  gn_args += f' is_debug=true ' if is_debug else f' is_debug=false '
  gn_args += f' enable_trace=\\\"perfetto\\\" ' if enable_trace else ''

  args = ' --args="%s"' % (gn_args)
  gn_out_path = os.path.join(root_path, 'out', 'gn_to_podspec')
  if os.path.exists(gn_out_path) and os.path.isdir(gn_out_path):
    clean_gn_project_json_file(gn_out_path)
  gn_path = os.path.join(root_path, 'lynx', 'tools', 'gn_tools', 'gn')
  set_podspec_target = '--podspec-target=%s' % (target) if target else ''
  gn_command = '%s gen %s %s --ide=podspec %s' % (gn_path, gn_out_path, args, set_podspec_target)

  print(gn_command)

  r = subprocess.call(gn_command, shell=True)
  return r

def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('--root', type=str, required=True, help='The root directory to search GN configuration')
  parser.add_argument('--target', type=str, required=False, help='The GN name of the podspec target you want to generate automatically')
  parser.add_argument('--is-debug', default=False, action='store_true', help='Whether to set the is_debug flag to true, which will be used by the gn script.')
  parser.add_argument('--enable-trace', default=False, action='store_true', help='Whether to set the enable_trace flag to true, which will be used by the gn script.')
  args = parser.parse_args()

  gn_args = f'use_xcode=true enable_air=true enable_testbench_replay=true enable_inspector=true \
              enable_napi_binding=true enable_lepusng_worklet=true \
              enable_recorder=true arm_use_neon=false build_lepus_compile=false'

  root_path = args.root

  return generate_compile_products(root_path, args, gn_args)

if __name__ == "__main__":
  sys.exit(main())