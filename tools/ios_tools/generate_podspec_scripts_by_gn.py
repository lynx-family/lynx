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
import re

root_path = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.append(root_path)
from tools_shared.ios_tools.generate_podspec_scripts_from_gn import generate_compile_products

def get_max_lynx_version(config_h_path):
    max_major, max_minor = -1, -1
    pattern = re.compile(r'#define\s+LYNX_VERSION_(\d+)_(\d+)\s+tasm::V_\1_\2')
    if not os.path.exists(config_h_path):
        return None
    with open(config_h_path, 'r') as f:
        for line in f:
            match = pattern.search(line)
            if match:
                major, minor = int(match.group(1)), int(match.group(2))
                if (major, minor) > (max_major, max_minor):
                    max_major, max_minor = major, minor
    if max_major != -1:
        return f"{max_major}.{max_minor}"
    return None

def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('--root', type=str, required=True, help='The root directory to search GN configuration')
  parser.add_argument('--target', type=str, required=False, help='The GN name of the podspec target you want to generate automatically')
  parser.add_argument('--is-debug', default=False, action='store_true', help='Whether to set the is_debug flag to true, which will be used by the gn script.')
  parser.add_argument('--enable-trace', default=False, action='store_true', help='Whether to set the enable_trace flag to true, which will be used by the gn script.')
  parser.add_argument('--enable-autosync-version', default=False, action='store_true', help='Enable automatic sync of lynx_version from config.h to darwin.gni.')
  args = parser.parse_args()

  args.gn_args = f'use_xcode=true enable_testbench_replay=true enable_inspector=true \
              enable_napi_binding=true enable_lepusng_worklet=true \
              enable_recorder=true arm_use_neon=false build_lepus_compile=false'

  root_path = args.root
  args.target_exclude_patterns = []

  darwin_gni_path = os.path.join(root_path, 'build_overrides', 'darwin.gni')
  original_gni_content = None
  
  if args.enable_autosync_version:
      config_h_path = os.path.join(root_path, 'core', 'renderer', 'tasm', 'config.h')
      max_version = get_max_lynx_version(config_h_path)
      
      if max_version and os.path.exists(darwin_gni_path):
          with open(darwin_gni_path, 'r') as f:
              original_gni_content = f.read()
          
          new_gni_content = re.sub(r'(lynx_version\s*=\s*)"[^"]+"', r'\g<1>"' + max_version + '"', original_gni_content)
          with open(darwin_gni_path, 'w') as f:
              f.write(new_gni_content)

  try:
      return generate_compile_products(root_path, args)
  finally:
      if original_gni_content is not None:
          with open(darwin_gni_path, 'w') as f:
              f.write(original_gni_content)

if __name__ == "__main__":
  sys.exit(main())
