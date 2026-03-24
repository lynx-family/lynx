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
import sys
import os

root_path = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.append(root_path)
from tools_shared.ios_tools.generate_podspec_scripts_from_gn import generate_compile_products



def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('--root', type=str, required=True, help='The root directory to search GN configuration')
  parser.add_argument('--target', type=str, required=False, help='The GN name of the podspec target you want to generate automatically')
  parser.add_argument('--is-debug', default=False, action='store_true', help='Whether to set the is_debug flag to true, which will be used by the gn script.')
  parser.add_argument('--enable-trace', default=False, action='store_true', help='Whether to set the enable_trace flag to true, which will be used by the gn script.')
  args = parser.parse_args()

  args.gn_args = f'use_xcode=true enable_testbench_replay=true enable_inspector=true \
              enable_napi_binding=true enable_lepusng_worklet=true \
              enable_recorder=true arm_use_neon=false build_lepus_compile=false'

  root_path = args.root
  args.target_exclude_patterns = []

  return generate_compile_products(root_path, args)

if __name__ == "__main__":
  sys.exit(main())
