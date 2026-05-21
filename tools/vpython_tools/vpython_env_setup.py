#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import argparse


def main():
  parser = argparse.ArgumentParser(description='Use vendored Python dependencies.')
  parser.add_argument('python_package_index', nargs='?', default='', help=argparse.SUPPRESS)
  parser.add_argument('--root_dir', default='', help=argparse.SUPPRESS)
  parser.add_argument('--requirements-path', default='', help=argparse.SUPPRESS)
  parser.add_argument('--pip_install_args', nargs='?', default='', help=argparse.SUPPRESS)
  parser.parse_known_args()
  print(
      'Python dependencies are provided by third_party/py_deps '
      'through tools/env.sh or tools/env.ps1')
  return 0


if __name__ == "__main__":
  raise SystemExit(main())
