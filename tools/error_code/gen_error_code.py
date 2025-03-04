# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import argparse
import os
import sys
import yaml
from generator.code_generator import *
from checker.spec_checker import *

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-l', '--lang', nargs='+', help="The language of generated error code file.")
    args, unknown = parser.parse_known_args()
    current_path = os.path.dirname(os.path.abspath(__file__))
    with open(os.path.join(current_path, "error_code.yaml"), "r") as f:
        spec = yaml.safe_load(f)
    spec_checker = SpecChecker(spec)
    if not spec_checker.check():
        print("Error code spec check failed")
        sys.exit(1)
    generator = ErrorCodeGenerator(spec, args.lang)
    generator.generate()

if __name__ == "__main__":
    main()