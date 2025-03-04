# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import os
import sys

CURRENT_PATH = os.path.dirname(os.path.abspath(__file__))
ROOT_PATH = os.path.join(CURRENT_PATH, "../../../..")
sys.path.append(os.path.join(ROOT_PATH, "tools/build_jni"))
from jni_generator import Options
from jni_generator import GenerateJNIHeader

def main():
  root_java_path = sys.argv[1]
  jni_files = sys.argv[2]
  output_dir = sys.argv[3]


  jni_gen_file = os.path.join(ROOT_PATH, "tools/build_jni/jni_generator.py")

  java_classes_file = []
  with open(jni_files, 'r') as java_classes_file:
    for line in java_classes_file:
      line = line.strip()
      if line:
        file_name = os.path.basename(line)
        jni_file_name = os.path.splitext(file_name)[0] + "_jni.h"
        input_file = os.path.join(root_java_path, line)
        output_file = os.path.join(output_dir, jni_file_name)

        options = Options(True)
        GenerateJNIHeader(input_file, output_file, options)
        print(f"python3 {jni_gen_file} {input_file} {output_file}")
  return 0

def usage():
  print('''
Usage: prebuild_jni.py: [java_class_dir] [jni_files] [output_dir]
''')

if __name__ == "__main__":
  print(sys.argv)
  if len(sys.argv) < 4:
    usage()
    sys.exit(-1)

  sys.exit(main())