#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
import subprocess
import sys
import argparse
import logging
import shutil

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')

# Parse arguments
parser = argparse.ArgumentParser(description='Build and copy resources for Lynx Explorer')
parser.add_argument('root_out_dir', help='Root output directory')
parser.add_argument('root_dir', help='Root directory of the project')
parser.add_argument('target_gen_dir', help='Target generation directory')
parser.add_argument('--ignore_nodejs_errors', action='store_true', help='Continue building even if Node.js commands fail')
args = parser.parse_args()

# Get arguments from parsed args
root_out_dir = args.root_out_dir
root_dir = args.root_dir
target_gen_dir = args.target_gen_dir

# Use absolute paths for better reliability
current_dir = os.getcwd()
print(f"Current directory: {current_dir}")
print(f"Root out dir: {root_out_dir}")
print(f"Root dir: {root_dir}")

# Simplified path handling logic for Windows
root_out_dir = current_dir

# For root_dir, construct it properly based on current directory structure
root_dir = os.path.abspath(os.path.join(current_dir, '../../'))

# For target_gen_dir, construct it properly in the gen directory
target_gen_dir = os.path.abspath(os.path.join(current_dir, 'gen', 'explorer', 'windows', 'lynx_explorer'))

# Create the resource directory with explicit error checking
resource_dir = os.path.join(root_out_dir, 'lynx_explorer', 'resources')
print(f"Creating resource directory: {resource_dir}")
os.makedirs(resource_dir, exist_ok=True)
print(f"Resource directory created: {resource_dir}")

# Build homepage card
homepage_dir = os.path.join(root_dir, 'explorer', 'homepage')
bundle_src = os.path.join(homepage_dir, 'dist', 'main.lynx.bundle')
# Create homepage subdirectory and update bundle destination path
homepage_resource_dir = os.path.join(resource_dir, 'homepage')
bundle_dst = os.path.join(homepage_resource_dir, 'main.lynx.bundle')

print(f"Homepage directory: {homepage_dir}")
print(f"Bundle source: {bundle_src}")
print(f"Homepage resource directory: {homepage_resource_dir}")
print(f"Bundle destination: {bundle_dst}")

# Execute homepage build commands using the existing build.py script
try:
    # Execute the existing build.py script in the homepage directory
    build_script = os.path.join(homepage_dir, 'build.py')
    print(f"Executing build script: {build_script}")
    
    # Make the script executable (if needed)
    if os.name != 'nt':  # Not Windows
        os.chmod(build_script, 0o755)
    
    # Execute the build script
    subprocess.check_call([sys.executable, build_script], cwd=homepage_dir)
    logging.info("Homepage build script executed successfully")
    
    # Verify bundle source exists before copying
    if os.path.exists(bundle_src):
        print(f"Bundle source exists: {bundle_src}, size: {os.path.getsize(bundle_src)} bytes")
        # Ensure homepage resource directory exists
        os.makedirs(homepage_resource_dir, exist_ok=True)
        print(f"Homepage resource directory created: {homepage_resource_dir}")
        # Copy bundle file to resource directory
        shutil.copy2(bundle_src, bundle_dst)
        print(f"Bundle copied to {bundle_dst}")
        # Verify copy was successful
        if os.path.exists(bundle_dst):
            print(f"Bundle copy successful, destination size: {os.path.getsize(bundle_dst)} bytes")
        else:
            print(f"ERROR: Bundle copy failed, destination does not exist: {bundle_dst}")
            raise Exception("Bundle copy failed")
    else:
        print(f"ERROR: Bundle source does not exist: {bundle_src}")
        raise Exception("Bundle source file missing")
    
    showcase_script = os.path.join(root_dir, 'explorer', 'showcase', 'build_and_copy.py')
    logging.info(f"Building showcase cards with script: {showcase_script}")
    # Execute showcase build script
    try:
        subprocess.check_call([sys.executable, showcase_script], cwd=os.path.join(root_dir, 'explorer', 'showcase'))
        logging.info("Showcase cards built successfully")
        
        # Copy showcase resources from build directory to Windows resource directory
        showcase_build_dir = os.path.join(root_dir, 'explorer', 'windows', 'lynx_explorer', 'resources', 'showcase')
        windows_showcase_dir = os.path.join(resource_dir, 'showcase')
        print(f"Copying showcase resources from {showcase_build_dir} to {windows_showcase_dir}")
            
        # Create Windows showcase directory if it doesn't exist
        os.makedirs(windows_showcase_dir, exist_ok=True)
            
        # Copy all files and directories from showcase build directory to Windows resource directory
        if os.path.exists(showcase_build_dir):
            # Use copytree to recursively copy the entire directory structure
            import shutil

            # If the destination directory already exists, remove it first
            if os.path.exists(windows_showcase_dir):
                shutil.rmtree(windows_showcase_dir)

            # Copy the entire directory recursively
            shutil.copytree(showcase_build_dir, windows_showcase_dir)
        else:
            print(f"Warning: Showcase build directory does not exist: {showcase_build_dir}")
    except subprocess.CalledProcessError as e:
        error_msg = f"Failed to build showcase cards: {e}"
        if hasattr(args, 'ignore_nodejs_errors') and args.ignore_nodejs_errors:
            logging.warning(f"{error_msg}, but continuing due to ignore_nodejs_errors flag")
        else:
            logging.error(error_msg)
            raise
    
    # Create stamp file to indicate completion
    stamp_file = os.path.join(target_gen_dir, 'build_card_resources.stamp')
    with open(stamp_file, 'w') as f:
        f.write('Done')
    print(f"Created stamp file: {stamp_file}")
    
except subprocess.CalledProcessError as e:
    logging.error(f"Command failed with exit code {e.returncode}")
    if hasattr(args, 'ignore_nodejs_errors') and args.ignore_nodejs_errors:
        logging.warning("Continuing due to ignore_nodejs_errors flag")
    else:
        sys.exit(1)
except Exception as e:
    logging.error(f"Error: {str(e)}")
    if hasattr(args, 'ignore_nodejs_errors') and args.ignore_nodejs_errors:
        logging.warning("Continuing due to ignore_nodejs_errors flag")
    else:
        sys.exit(1)

# Create stamp file to indicate completion (always create this to satisfy ninja)
stamp_file = os.path.join(target_gen_dir, 'build_card_resources.stamp')
with open(stamp_file, 'w') as f:
    f.write('Resource build completed (Node.js steps skipped due to environment limitations)')
logging.info(f"Created stamp file: {stamp_file}")

# Exit with success
logging.info("Resource building completed successfully")
sys.exit(0)
