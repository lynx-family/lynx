#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
import sys
import shutil
# Add the lynx tools directory to Python path
def add_lynx_tools_path():
    # Get the project root directory
    project_root = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
    lynx_tools_path = os.path.join(project_root, "tools")
    sys.path.append(lynx_tools_path)
# Add lynx tools path to import pnpm_helper
add_lynx_tools_path()
# Import pnpm_helper from js_tools
from js_tools.pnpm_helper import run_pnpm_command
def main():
    # Get the directory where the current script is located
    script_dir = os.path.dirname(os.path.abspath(__file__))
    
    # Get the project root directory
    project_root = os.path.abspath(os.path.join(script_dir, "..", ".."))
    
    print(f"Project root: {project_root}")
    
    # Execute pnpm install command using pnpm_helper
    print(f"Running pnpm install in {script_dir}")
    pnpm_args = ['pnpm', 'install', '--ignore-workspace']
    run_pnpm_command(pnpm_args, script_dir)
    
    # Move the weak-node-api package to the current directory
    source_dir = os.path.join(script_dir, "node_modules", "@lynx-js", "weak-node-api")
    target_dir = script_dir
    
    print(f"Copying {source_dir} to {target_dir}")
    
    # Copy the directory contents directly to current directory
    try:
        # Only copy specific files/directories
        items_to_copy = ["generated", "headers", "prebuilt"]
        
        for item in items_to_copy:
            src_item = os.path.join(source_dir, item)
            dst_item = os.path.join(target_dir, item)
            
            if not os.path.exists(src_item):
                print(f"Warning: Source item {src_item} does not exist, skipping.")
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
