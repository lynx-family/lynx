#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
import shutil
import subprocess
import sys

# Get arguments passed from GN
root_out_dir = sys.argv[1]
root_dir = sys.argv[2]
target_gen_dir = sys.argv[3]

# Use absolute paths for better reliability
current_dir = os.getcwd()
print(f"Current directory: {current_dir}")
print(f"Root out dir: {root_out_dir}")
print(f"Root dir: {root_dir}")
print(f"Target gen dir: {target_gen_dir}")

# Simplified path handling logic
# Since we're already in the out/Default directory, set root_out_dir to current directory
root_out_dir = current_dir

# For root_dir, construct it properly based on current directory structure
# We need to go up two levels from out/Default to reach the project root
root_dir = os.path.abspath(os.path.join(current_dir, '../../'))

# For target_gen_dir, construct it properly in the gen directory
# The expected path should be: current_dir/gen/explorer/darwin/macos/lynx_explorer
target_gen_dir = os.path.abspath(os.path.join(current_dir, 'gen', 'explorer', 'darwin', 'macos', 'lynx_explorer'))

print(f"Abs root out dir: {root_out_dir}")
print(f"Abs root dir: {root_dir}")
print(f"Abs target gen dir: {target_gen_dir}")

# Get environment variables for conditional builds
skip_card_build = os.environ.get('SKIP_CARD_BUILD', 'false').lower() == 'true'
integration_test = os.environ.get('INTEGRATION_TEST', 'false').lower() == 'true'

print(f"Skip card build: {skip_card_build}")
print(f"Integration test: {integration_test}")

# Build homepage card (similar to iOS version)
homepage_dir = os.path.join(root_dir, 'explorer', 'homepage')
bundle_src = os.path.join(homepage_dir, 'dist', 'main.lynx.bundle')
# Create homepage subdirectory and update bundle destination path
homepage_resource_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'Resource', 'homepage')
bundle_dst = os.path.join(homepage_resource_dir, 'main.lynx.bundle')
generated_resource_dir = os.path.join(target_gen_dir, 'Resource')
generated_homepage_resource_dir = os.path.join(generated_resource_dir, 'homepage')
generated_showcase_resource_dir = os.path.join(generated_resource_dir, 'showcase')
generated_bundle_dst = os.path.join(generated_homepage_resource_dir, 'main.lynx.bundle')
showcase_resource_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'Resource', 'showcase')

print(f"Homepage directory: {homepage_dir}")
print(f"Bundle source: {bundle_src}")
print(f"Homepage resource directory: {homepage_resource_dir}")
print(f"Bundle destination: {bundle_dst}")
print(f"Generated resource directory: {generated_resource_dir}")

# Execute homepage build commands
try:
    print(f"Current directory before homepage build: {os.getcwd()}")
    
    # Build home page card with explicit working directory
    subprocess.check_call(['bash', '-c', 'pnpm install --no-frozen-lockfile && pnpm run build'], cwd=homepage_dir)
    print("Homepage built successfully")
    
    # Verify bundle source exists before copying
    if os.path.exists(bundle_src):
        print(f"Bundle source exists: {bundle_src}, size: {os.path.getsize(bundle_src)} bytes")
        # Ensure homepage resource directory exists
        os.makedirs(homepage_resource_dir, exist_ok=True)
        print(f"Homepage resource directory created: {homepage_resource_dir}")
        # Copy bundle file to resource directory with new name
        subprocess.check_call(['cp', '-v', bundle_src, bundle_dst])
        print(f"Bundle copied to {bundle_dst} with new name")
        os.makedirs(generated_homepage_resource_dir, exist_ok=True)
        shutil.copy2(bundle_src, generated_bundle_dst)
        print(f"Bundle copied to generated resource {generated_bundle_dst}")
        # Verify copy was successful
        if os.path.exists(bundle_dst):
            print(f"Bundle copy successful, destination size: {os.path.getsize(bundle_dst)} bytes")
        else:
            print(f"ERROR: Bundle copy failed, destination does not exist: {bundle_dst}")
            raise Exception("Bundle copy failed")
    else:
        print(f"ERROR: Bundle source does not exist: {bundle_src}")
        raise Exception("Bundle source file missing")
    
    # Build showcase cards if not skipped
    if not skip_card_build:
        showcase_script = os.path.join(root_dir, 'explorer', 'showcase', 'build_and_copy.py')
        print(f"Building showcase cards with script: {showcase_script}")
        print(f"Current directory before showcase build: {os.getcwd()}")
        # Execute showcase build script with explicit working directory
        subprocess.check_call(['python3', showcase_script], cwd=os.path.join(root_dir, 'explorer', 'showcase'))
        if os.path.exists(generated_showcase_resource_dir):
            shutil.rmtree(generated_showcase_resource_dir)
        shutil.copytree(showcase_resource_dir, generated_showcase_resource_dir)
        print(f"Showcase cards copied to generated resource {generated_showcase_resource_dir}")
        print("Showcase cards built successfully")
    else:
        if os.path.exists(generated_showcase_resource_dir):
            shutil.rmtree(generated_showcase_resource_dir)
        if os.path.exists(showcase_resource_dir):
            shutil.copytree(showcase_resource_dir, generated_showcase_resource_dir)
        else:
            os.makedirs(generated_showcase_resource_dir, exist_ok=True)
        print("Skipping showcase card build as SKIP_CARD_BUILD is true")
    
    # Build integration test demo pages if enabled
    if integration_test:
        integration_script = os.path.join(root_dir, 'testing', 'integration_test', 'demo_pages', 'build_and_copy.py')
        print(f"Building integration test demo pages with script: {integration_script}")
        subprocess.check_call(['python3', integration_script])
        print("Integration test demo pages built successfully")
    else:
        print("Skipping integration test demo pages build as INTEGRATION_TEST is false")
    
    # Create stamp file to mark completion
    stamp_file = os.path.join(target_gen_dir, 'build_card_resources.stamp')
    print(f"Creating stamp file: {stamp_file}")
    # Ensure target_gen_dir exists before creating stamp file
    os.makedirs(target_gen_dir, exist_ok=True)
    with open(stamp_file, 'w') as f:
        f.write('')
    print(f"Stamp file created: {stamp_file}")
    
    print("Build resources completed successfully!")
except subprocess.CalledProcessError as e:
    print(f"Error executing command: {e}")
    sys.exit(1)
except Exception as e:
    print(f"Unexpected error: {e}")
    sys.exit(1)

