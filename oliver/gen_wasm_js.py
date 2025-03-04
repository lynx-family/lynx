# Copyright 2022 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
"""
build static libary of wasm to js file by emsdk
"""

import argparse
import logging
import os
import shutil
import subprocess
import sys

def link_libs(emsdk_path, binary_path, output_name, libs_path):
  """
  merge static libary by emar
  """
  libs_name = []
  previous_path = os.getcwd()
  os.chdir(binary_path)
  path = os.path.join(binary_path, "lib{}.a".format(output_name))
  emar_path = os.path.join(emsdk_path, "upstream", "emscripten", "emar")
  cmd = "{} x lib{}.a".format(emar_path, output_name)
  logging.info(cmd)
  result = subprocess.call(cmd, shell=True)
  if result != 0:
    logging.error("execute command [{}] failed with error code {}".format(cmd, result))
    return result

  for name in libs_name:
    cmd = "{} x {}".format(emar_path, name)
    logging.info(cmd)
    result = subprocess.call(cmd, shell=True)
    if result != 0:
      logging.error("execute command [{}] failed with error code {}".format(cmd, result))
      return result

  cmd = "{} qc lib{}.a *.o".format(emar_path, output_name)
  logging.info(cmd)
  result = subprocess.call(cmd, shell=True)
  if result != 0:
    logging.error("execute command [{}] failed with error code {}".format(cmd, result))
    return result

  emranlib_path = os.path.join(emsdk_path, "upstream", "emscripten", "emranlib")
  cmd = "{} lib{}.a".format(emranlib_path, output_name)
  logging.info(cmd)
  result = subprocess.call(cmd, shell=True)
  if result != 0:
    logging.error("execute command [{}] failed with error code {}".format(cmd, result))
    return result

  cmd = "rm *.o"
  logging.info(cmd)
  result = subprocess.call(cmd, shell=True)
  if result != 0:
    logging.error("execute command [{}] failed with error code {}".format(cmd, result))
    return result
  os.chdir(previous_path)
  return 0

def gen_js_file(emsdk_path, binary_path, output_name, flags):
  logging.info("gen js file in path:{} by emsdk:{}".format(binary_path, emsdk_path))
  path = os.path.join(binary_path, "lib{}.a".format(output_name))
  name = os.path.join(binary_path, "{}.js".format(output_name))

  command = "{emsdk_path}/upstream/emscripten/emcc {path} \
            -o {name} \
            -g \
            {flags}".format(emsdk_path=emsdk_path, path=path, name=name, flags=flags)
  result = subprocess.call(command, shell=True)
  if result != 0:
    logging.error("execute command [{}] failed with error code {}".format(command, result))
    return result
  return 0

def main():
  parser = argparse.ArgumentParser()
  parser.add_argument("-e", "--emsdk_path")
  parser.add_argument("-b", "--binary_path")
  parser.add_argument("-o", "--output_name")
  parser.add_argument("-l", "--libs_path")
  parser.add_argument("--flags")
  args = parser.parse_args()
  emsdk_path = args.emsdk_path
  binary_path = args.binary_path
  output_name = args.output_name
  libs_path = args.libs_path
  flags = args.flags

  result = link_libs(emsdk_path, binary_path, output_name, libs_path)
  if result != 0:
    return result
  result = gen_js_file(emsdk_path, binary_path, output_name, flags)
  return result

if __name__ == '__main__':
  sys.exit(main())
