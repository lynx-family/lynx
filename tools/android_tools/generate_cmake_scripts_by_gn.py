#!/usr/bin/env python3
# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

"""
This script generate CMakeLists.txt from GN cmake_target for gradle.

"""

import argparse
import sys
import os
import json
import subprocess
import shutil

ANDROID_GN_ARGS_FILE_PATH = "out/android/gn_args"
ANDROID_GN_ARGS_FILE_NAME = "gn_args.json"
GN_OUT_DIR_PATH = "out/gn_to_cmake"


def android_target_cpu(variant_type):
  target_cpu = ''
  if 'x86_64' in variant_type:
    target_cpu = 'x64'
  elif 'armeabi-v7a' in variant_type or 'armeabi' in variant_type:
    target_cpu = 'arm'
  elif 'mips' in variant_type:
    target_cpu = 'mipsel'
  elif 'x86' in variant_type:
    target_cpu = 'x86'
  elif 'arm64-v8a' in variant_type:
    target_cpu = 'arm64'
  elif 'mips64' in variant_type:
    target_cpu = 'mips64el'
  else:
    print("Please pass the android ABI set by gradle as a part of the map key.")
  return target_cpu


def format_gn_arg(key, value):
  arg_str = ''
  if value == 'true' or value == 'false':
    arg_str = ' %s=%s' % (key, value)
  else:
    arg_str = ' %s=\\\"%s\\\"' % (key, value)
  return arg_str

def clean_gn_project_json_file(gn_out_dir):
  project_json_file = os.path.join(gn_out_dir, 'project.json')
  if os.path.exists(project_json_file):
    os.remove(project_json_file)

def clean_all_products(root_dir):
  gn_args_file_dir = os.path.join(root_dir, ANDROID_GN_ARGS_FILE_PATH)
  if os.path.exists(gn_args_file_dir):
    shutil.rmtree(gn_args_file_dir)
  gn_out_dir = os.path.join(root_dir, GN_OUT_DIR_PATH)
  cmake_targets = []
  if not os.path.exists(gn_out_dir):
    return 0
  for dir in os.listdir(gn_out_dir):
    cmake_targets_dir = os.path.join(gn_out_dir, dir, 'cmake_targets')
    for target in os.listdir(cmake_targets_dir):
      if target in cmake_targets:
        continue
      cmake_targets.append(target)
      cmake_target_path = os.path.join(cmake_targets_dir, target)
      with open(cmake_target_path, 'r') as file:
        contents = file.readlines()
        if len(contents) > 0:
          cmake_target = contents[0].replace('\n', '')
          CMakeLists_impl_dir = cmake_target.split(':')[0][2:]
          CMakeLists_impl_path = os.path.join(root_dir, CMakeLists_impl_dir, 'CMakeLists_impl')
          if os.path.exists(CMakeLists_impl_path):
            shutil.rmtree(CMakeLists_impl_path)
        file.close()
  shutil.rmtree(gn_out_dir)
  return 0

def run_gn_script(args, root_dir, target_exclude_patterns:list=None):
  target = args.target
  is_clean = args.clean
  if is_clean:
    return clean_all_products(root_dir)
  project_name = args.project_name if args.project_name else ''
  gn_args_file_path = os.path.join(root_dir, ANDROID_GN_ARGS_FILE_PATH, project_name, ANDROID_GN_ARGS_FILE_NAME)
  gn_args_map = {}
  if not os.path.exists(gn_args_file_path):
    print("%s not found, please run `tools/android_tools/write_gn_args.py --gn-args GN_ARGS` first in your gradle scripts." % gn_args_file_path)
    return -1
  with open(gn_args_file_path, "r+") as gn_args_file:
    gn_args_map = json.load(gn_args_file)
    gn_args_file.close()
  os.remove(gn_args_file_path)
  
  r = 0
  current_dir = os.path.dirname(os.path.realpath(__file__))
  gn_path = os.path.join(current_dir, '..', 'gn_tools', 'gn_wrapper.py')
  gn_out_dir = os.path.join(root_dir, GN_OUT_DIR_PATH)
  for gn_args_key in gn_args_map.keys():
    gn_args = ""
    gn_args_list = gn_args_map[gn_args_key]
    for key in gn_args_list:
      gn_args += format_gn_arg(key, gn_args_list[key])
    gn_args += ' target_cpu=\\\"%s\\\"' % (android_target_cpu(gn_args_key))
    gn_args += ' target_os=\\\"android\\\"'
    gn_args += ' use_ndk_cxx=true'
    gn_args += ' android_full_debug=true'
    if target_exclude_patterns is not None:
      patterns = []
      for pattern in target_exclude_patterns:
        patterns.append(f'\\\"{pattern}\\\"')
      gn_args +=' target_exclude_patterns=[%s]' % (','.join(patterns))
    gn_out_path = os.path.join(gn_out_dir, project_name, gn_args_key)
    if os.path.exists(gn_out_path) and os.path.isdir(gn_out_path):
      clean_gn_project_json_file(gn_out_path)
    set_cmake_target = '--cmake-target=%s' % (target) if target else ''
    cmd = 'python3 %s gen %s --args="%s" --ide="cmake" %s' % (gn_path, gn_out_path, gn_args, set_cmake_target)
    r |= subprocess.call(cmd, shell=True)
  return r


def main():
  parser = argparse.ArgumentParser(description='')
  parser.add_argument('-t', '--target', type=str, required=False, help='The GN name of the cmake target you want to generate automatically.')
  parser.add_argument('-n', '--project-name', type=str, required=False, help='Inject the project name to isolate GN args among different projects.')
  parser.add_argument('--clean', action='store_true', required=False, help='Delete all products.')
  args, unknown = parser.parse_known_args()
  root_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(__file__))))
  return run_gn_script(args, root_dir)

if __name__ == "__main__":
  sys.exit(main())