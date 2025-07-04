#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import argparse
import os
import sys

def build(version, jssdk_main_dest_path, jssdk_debug_dest_path, lynx_core_build_tools_path):
    """
    Build the JavaScript SDK.
    """
    # Get the current directory of the script
    current_dir = os.path.dirname(os.path.realpath(__file__))
    # Calculate the root path
    root_path = os.path.abspath(os.path.join(current_dir, '../../../../'))
    os.environ['PATH'] = f"{root_path}/buildtools/node/bin:{os.environ['PATH']}"
    # Change to the build tools directory
    os.chdir(lynx_core_build_tools_path)
    # Execute the build script
    os.system(f"python3 ./build.py --platform android --release_output {jssdk_main_dest_path}/lynx_core.js --dev_output {jssdk_debug_dest_path}/lynx_core_dev.js --version {version}")


def clear(jssdk_main_dest_path, jssdk_debug_dest_path):
    """
    Clear the generated JavaScript SDK files.
    """
    # Check if the main JavaScript file exists and remove it
    main_file_path = os.path.join(jssdk_main_dest_path, "lynx_core.js")
    if os.path.isfile(main_file_path):
        os.remove(main_file_path)
    # Check if the debug JavaScript file exists and remove it
    debug_file_path = os.path.join(jssdk_debug_dest_path, "lynx_core_dev.js")
    if os.path.isfile(debug_file_path):
        os.remove(debug_file_path)


def main():
    parser = argparse.ArgumentParser(description='Build and clear JavaScript SDK.')
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--build', action='store_true', help='Build the JavaScript SDK')
    group.add_argument('--clear', action='store_true', help='Clear the generated JavaScript SDK files')
    parser.add_argument('--version', required=True, help='Version of the SDK')
    parser.add_argument('--jssdkMainDestPath', required=True, help='Destination path for the main JavaScript SDK')
    parser.add_argument('--jssdkDebugDestPath', required=True, help='Destination path for the debug JavaScript SDK')
    parser.add_argument('--lynxCoreBuildToolsPath', required=True, help='Path to the Lynx core build tools')

    print(sys.argv)
    args = parser.parse_args()

    if args.build:
        print("build")
        build(args.version, args.jssdkMainDestPath, args.jssdkDebugDestPath, args.lynxCoreBuildToolsPath)
    elif args.clear:
        print("clear")
        clear(args.jssdkMainDestPath, args.jssdkDebugDestPath)
    
    return 0


if __name__ == "__main__":
    sys.exit(main())

    