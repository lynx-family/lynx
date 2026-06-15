#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import argparse
import os
import shutil


def copy_flattened_headers(source_dir, output_dir, overwrite):
    if not os.path.isdir(source_dir):
        return
    os.makedirs(output_dir, exist_ok=True)
    for current_dir, _, files in os.walk(source_dir):
        for file_name in sorted(files):
            if not file_name.endswith(".h"):
                continue
            source = os.path.join(current_dir, file_name)
            target = os.path.join(output_dir, file_name)
            if overwrite or not os.path.exists(target):
                shutil.copy2(source, target)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--lynx-root", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--stamp", required=True)
    args = parser.parse_args()

    output_root = os.path.abspath(args.output)
    if os.path.exists(output_root):
        shutil.rmtree(output_root)

    lynx_root = os.path.abspath(args.lynx_root)
    lynx_output = os.path.join(output_root, "Lynx")
    lynxbase_output = os.path.join(output_root, "LynxBase")

    common_lynx = os.path.join(lynx_root, "platform", "darwin", "common", "lynx")
    # CocoaPods/Xcode flatten these headers for framework-style imports such as
    # <Lynx/LynxError.h>. Recreate the same layout for direct GN/Ninja builds.
    copy_flattened_headers(os.path.join(common_lynx, "public"), lynx_output, True)
    copy_flattened_headers(common_lynx, lynx_output, False)
    copy_flattened_headers(os.path.join(lynx_root, "base", "platform", "darwin"), lynxbase_output,
                           True)

    os.makedirs(os.path.dirname(os.path.abspath(args.stamp)), exist_ok=True)
    with open(args.stamp, "w") as stamp:
        stamp.write("ok\n")


if __name__ == "__main__":
    main()
