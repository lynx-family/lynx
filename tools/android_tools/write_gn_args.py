#!/usr/bin/env python3
# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import argparse
import os
import json
import sys
CUR_FILE_DIR = os.path.dirname(os.path.abspath(__file__))
from generate_cmake_scripts_by_gn import ANDROID_GN_ARGS_FILE_NAME, ANDROID_GN_ARGS_FILE_PATH

def write_gn_args(args, root_dir):
  try:
    gn_args = json.loads(args.gn_args)
  except json.JSONDecodeError as e:
    print(f"Error decoding JSON: {e}")
    return -1
  
  project_name = args.project_name if args.project_name else ''
  out_file = os.path.join(root_dir, ANDROID_GN_ARGS_FILE_PATH, project_name, ANDROID_GN_ARGS_FILE_NAME)
  project = {}
  out_file_dir = os.path.dirname(out_file)
  if not os.path.exists(out_file_dir):
    os.makedirs(out_file_dir, exist_ok=True)
  
  cmake_targets_set = set()
  if not os.path.exists(out_file):
    open(out_file, "x")
  else:
    with open(out_file, "r+") as file:
      project = json.load(file)
      file.close()
  for variant_type in gn_args.keys():
    if variant_type in project.keys():
      project[variant_type].update(gn_args[variant_type])
    else:
      project[variant_type] = gn_args[variant_type]

  file = open(out_file, 'w+')
  json.dump(project, file, indent=4)
  file.close()

def main():
  parser = argparse.ArgumentParser(description='')
  parser.add_argument('--gn-args', type=str, required=True, help='JSON string data')
  parser.add_argument('-n', '--project-name', type=str, required=False, help='Inject the project name to isolate GN args among different projects.')
  args, unknown = parser.parse_known_args()
  root_dir = os.path.dirname(os.path.dirname(os.path.dirname(CUR_FILE_DIR)))
  write_gn_args(args, root_dir)

if __name__ == "__main__":
  sys.exit(main())
