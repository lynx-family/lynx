#!/usr/bin/env python3
# Copyright 2023 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

"""
This script is part of the gn script test case
"""

import sys

SUCCESS_CODE = 0
ERROR_CODE = 1

def main(args):
  if len(args) != 2:
    print("You must provide two arguments, (target_name, suffix)")
    print(ERROR_CODE)
  else:
    target_name = args[0]
    suffix = args[1]
    if target_name.endswith(suffix):
        print(f"check {target_name} with success!")
        print(SUCCESS_CODE)
    else:
        print(f"{target_name} must end with ${suffix}.")
        print(ERROR_CODE)

if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))