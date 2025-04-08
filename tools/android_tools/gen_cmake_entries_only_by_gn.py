#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import argparse
import sys
import os

from generate_cmake_scripts_by_gn import run_gn_to_cmake

def generate_cmake_entry_scripts(args, root_dir, build_lynx_dylib=False):
  gn_args = args.gn_args
  args_map = {}
  if gn_args:
    for argument in gn_args.split(','):
      key = argument.split('=')[0]
      value = argument.split('=')[1]
      args_map[key] = value
  gn_args_map = {
    'CMakeList_entries': args_map
  }
  print(gn_args_map)

  return run_gn_to_cmake(root_dir, '', gn_args_map, '', build_lynx_dylib, True)

def main():
  parser = argparse.ArgumentParser(description='')
  parser.add_argument('-args', '--gn-args', type=str, required=False, help='GN args.')
  args, unknown = parser.parse_known_args()
  root_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(__file__))))
  return generate_cmake_entry_scripts(args, root_dir, True)

if __name__ == "__main__":
  sys.exit(main())