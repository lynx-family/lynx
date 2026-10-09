# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.


import argparse
import os
import sys
import code_generator

def main(argv=None):
  parser = argparse.ArgumentParser()
  parser.add_argument('--spec-files', nargs='+', help="Ordered feature YAML specifications.")
  parser.add_argument('-l', '--lang', nargs='+', choices=['cpp', 'java', 'objc', 'typescript'])
  parser.add_argument('--output-root', help="Output root mirroring the source layout.")
  args = parser.parse_args([] if argv is None else argv)
  dir_name = os.path.dirname(os.path.abspath(__file__))

  all_features = code_generator.get_all_features(args.spec_files)
  output_root = args.output_root or os.path.join(dir_name, "../..")
  native_file = os.path.abspath(os.path.join(output_root, "core/services/feature_count/feature.h"))
  java_file = os.path.abspath(os.path.join(output_root, "platform/android/lynx_android/src/main/java/com/lynx/tasm/featurecount/LynxFeatureCounter.java"))
  objc_file = os.path.abspath(os.path.join(output_root, "platform/darwin/common/lynx/feature_count/LynxFeature.h"))
  typescript_file = os.path.abspath(os.path.join(output_root, "js_libraries/lynx-core/src/common/feature.ts"))
  code_generator.generate_code(native_file, java_file, objc_file, typescript_file, all_features, args.lang)

if __name__ == '__main__':
  main(sys.argv[1:])
