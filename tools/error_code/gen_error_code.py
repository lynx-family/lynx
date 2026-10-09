# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import argparse
import os
import sys
import yaml
from generator.code_generator import *
from checker.spec_checker import *

def load_spec(spec_files):
    spec = None
    for spec_file in spec_files:
        with open(spec_file, "r", encoding="utf-8") as f:
            next_spec = yaml.safe_load(f)
        if not isinstance(next_spec, dict) or "sections" not in next_spec:
            raise ValueError("Expected an error code specification with sections: " + spec_file)
        if spec is None:
            spec = next_spec
        elif next_spec["sections"] is not None:
            spec["sections"].extend(next_spec["sections"])
    return spec


def main(argv=None):
    parser = argparse.ArgumentParser()
    parser.add_argument('-l', '--lang', nargs='+', choices=['cpp', 'java', 'oc', 'ts', 'ets'],
                        help="The language of generated error code file.")
    parser.add_argument('--spec-files', nargs='+', help="Ordered error code YAML specifications.")
    parser.add_argument('--output-root', help="Output root mirroring the source layout.")
    args, unknown = parser.parse_known_args(argv)
    current_path = os.path.dirname(os.path.abspath(__file__))
    spec = load_spec(args.spec_files or [os.path.join(current_path, "error_code.yaml")])
    spec_checker = SpecChecker(spec)
    if not spec_checker.check():
        print("Error code spec check failed")
        sys.exit(1)
    generator = ErrorCodeGenerator(spec, args.lang, args.output_root)
    generator.generate()

if __name__ == "__main__":
    main()
