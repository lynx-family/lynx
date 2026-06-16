#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
import sys
import shutil

def main():
    # Get the directory where the current script is located
    script_dir = os.path.dirname(os.path.abspath(__file__))

    # Move the weak-node-api package to the current directory
    source_dir = os.path.join(script_dir, "node_modules", "@lynx-js", "weak-node-api")
    target_dir = script_dir

    print(f"Copying {source_dir} to {target_dir}")

    if not os.path.isdir(source_dir):
        print(
            "Error: weak-node-api dependency is missing under the local workspace "
            f"package: {source_dir}"
        )
        print(
            "Please ensure pnpm install includes the workspace package "
            '"./lynx/third_party/weak-node-api" so that '
            '"@lynx-js/weak-node-api" is installed before running GN/Ninja.'
        )
        sys.exit(1)

    # Copy the directory contents directly to current directory
    try:
        # Only copy specific files/directories
        items_to_copy = ["generated", "headers", "prebuilt", "shim"]
        missing_items = []

        for item in items_to_copy:
            src_item = os.path.join(source_dir, item)
            dst_item = os.path.join(target_dir, item)

            if not os.path.exists(src_item):
                missing_items.append(src_item)
                continue

            # Remove existing item before copying to avoid errors
            if os.path.exists(dst_item):
                if os.path.islink(dst_item):
                    os.unlink(dst_item)
                elif os.path.isdir(dst_item):
                    shutil.rmtree(dst_item)
                else:
                    os.remove(dst_item)
                    
            if os.path.isdir(src_item):
                shutil.copytree(src_item, dst_item, symlinks=True)
            else:
                shutil.copy2(src_item, dst_item)

        if missing_items:
            print("Error: weak-node-api package is incomplete. Missing required items:")
            for item in missing_items:
                print(f"  - {item}")
            sys.exit(1)

        weak_napi_headers = ["weak_napi_defines.h", "weak_napi_undefs.h"]
        shim_dir = os.path.join(target_dir, "shim")
        os.makedirs(shim_dir, exist_ok=True)
        for header in weak_napi_headers:
            src_header = os.path.join(target_dir, "headers", header)
            dst_header = os.path.join(shim_dir, header)
            if not os.path.exists(src_header):
                print(f"Error: weak-node-api header is missing: {src_header}")
                sys.exit(1)
            shutil.copy2(src_header, dst_header)

        print(f"Successfully copied specific items from {source_dir} to {target_dir}")
    except Exception as e:
        print(f"Error copying items from {source_dir} to {target_dir}: {e}")
        sys.exit(1)

    # Output some content according to BUILD.gn requirements
    # BUILD.gn uses "list lines", so we need to output some lines
    # Here we can output files/directories under node_modules or just a success message
    return ["pnpm_install_success", "weak_node_api_moved"]

if __name__ == "__main__":
    outputs = main()
    for output in outputs:
        print(output)
