# -*- coding: utf-8 -*-
# Copyright 2021 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import argparse
import fnmatch
import os
import sys


def clean_files(patterns):
    root_dir = os.path.dirname(os.path.abspath(__file__))
    for item in os.listdir(root_dir):
        for pattern in patterns:
            if fnmatch.fnmatch(item, pattern):
                os.remove(item)
                break


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--pattern", nargs="*", dest="patterns",
                        help="pattern to match file")
    args = parser.parse_args(sys.argv[1:])
    if not args.patterns:
        args.patterns = ["*.log", "*.png", "*.jpg", "*.jpeg", "testbench*.txt", "ui_scene_*", "*_dump.json", "*.zip", "*.exec"]
    clean_files(args.patterns)
