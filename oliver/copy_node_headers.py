# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
"""
find node header path & node-addon-api header path. copy header to gn gen folder
"""

import argparse
import logging
import os
import shutil
import subprocess
import sys

def find_and_copy_addon_api_header(work_dir):
  path = os.path.abspath(os.path.dirname(__file__))
  find_cmd = "node -p \"require('node-addon-api').include\" WORKING_DIRECTORY {}".format(work_dir)
  target_path = os.path.join(path, work_dir)
  previous_path = os.getcwd()

  os.chdir(target_path)
  output_bytes = subprocess.check_output(find_cmd, shell=True)
  encoding = 'utf-8'
  output_str = output_bytes.decode(encoding)
  output_str = output_str.replace("\n", "")
  output_str = output_str.replace("\"", "")
  os.chdir(previous_path)
  dst = os.path.join("gen", "node-addon-api")
  if not os.path.exists(dst):
    logging.info("node-addon-api header path:{}".format(output_str))
    shutil.copytree(output_str, dst)
  return 0

def main():
  parser = argparse.ArgumentParser()
  parser.add_argument("work_dir")
  args = parser.parse_args()
  work_dir = args.work_dir

  result = find_and_copy_addon_api_header(work_dir)
  return result

if __name__ == '__main__':
  sys.exit(main())
