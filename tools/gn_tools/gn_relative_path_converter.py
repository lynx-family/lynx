#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
import re
import sys
import argparse
from pathlib import Path

# GN path prefixes that don't need processing
skip_prefixes = [
    "//PODS_ROOT",
    "//PODS_CONFIGURATION_BUILD_DIR",
    "//TARGET_BUILD_DIR",
    "//PODS_TARGET_SRCROOT",
    "//build_overrides", 
    "//build",
    "//$lynx_dir",
    "//${lynx_dir}",
    "//third_party",
    "//config.gni",
    "//tools/idl_parser",
    "//out/",
]

def convert_absolute_to_relative_path(absolute_path, current_file_dir, project_root):
    """
    Convert absolute paths starting with '//' to paths relative to current_file_dir.
    
    Args:
        absolute_path: Path starting with '//', e.g. //lynx/core/BUILD.gn.
        current_file_dir: Directory of current file.
        project_root: Project root directory path.
    
    Returns:
        Converted relative path
    """
    # Remove leading '//'.
    for skip_prefix in skip_prefixes:
        if absolute_path.startswith(skip_prefix):
            return absolute_path
    path_without_prefix = absolute_path[2:]
    
    # Build complete absolute path.
    full_absolute_path = os.path.join(project_root, path_without_prefix)
    
    # Calculate relative path.
    try:
        relative_path = os.path.relpath(full_absolute_path, current_file_dir)
        return relative_path
    except ValueError:
        return absolute_path

def process_file(file_path, project_root):
    """
    Process a single file, convert '//' paths within it.
    
    Args:
        file_path: Path of file to process.
        project_root: Project root directory path.
    
    Returns:
        (whether file was modified, number of replacements).
    """
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        
        original_content = content
        current_file_dir = os.path.dirname(file_path)

        
        # Match paths starting with '//' (excluding '//' comments),
        # Use regex to match '//' paths in non-comment lines.
        lines = content.split('\n')
        modified_lines = []
        total_replacements = 0
        
        for line_num, line in enumerate(lines):
            # Skip empty lines and pure comment lines.
            stripped_line = line.strip()
            if not stripped_line or stripped_line.startswith('#'):
                modified_lines.append(line)
                continue
            
            # Match '//' paths within quotes (both double and single quotes),
            # Example: "//platform/darwin/ios/lynx/clay/LynxClayHelper.h".
            pattern_quotes = r'(["\'])(//[a-zA-Z0-9_:{}/$+\-?&%=~]+(?:\.[a-zA-Z0-9_]+)?)(\1)'
            
            def replace_path_in_quotes(match):
                nonlocal total_replacements
                quote_char = match.group(1)  # Quote character (" or ')
                absolute_path = match.group(2)  # Path starting with '//'
                
                # Convert path.
                relative_path = convert_absolute_to_relative_path(
                    absolute_path, current_file_dir, project_root
                )
                total_replacements += 1
                return f"{quote_char}{relative_path}{quote_char}"
            
            # Process paths in quotes first.
            modified_line = re.sub(pattern_quotes, replace_path_in_quotes, line)
            
            # Then process '//' paths not in quotes.
            # Use more precise pattern to match standalone '//' paths.
            pattern_standalone = r'(^|\s)(//[a-zA-Z0-9_:{}/$+\-?&%=~]+(?:\.[a-zA-Z0-9_]+)?)(\s|$|[,)])'
            
            def replace_path_standalone(match):
                nonlocal total_replacements
                prefix = match.group(1)  # Prefix (space or line start)
                absolute_path = match.group(2)  # Path starting with '//'
                suffix = match.group(3)  # Suffix (space, line end, comma, right parenthesis, etc.)
                
                # Convert path.
                relative_path = convert_absolute_to_relative_path(
                    absolute_path, current_file_dir, project_root
                )
                total_replacements += 1
                return f"{prefix}{relative_path}{suffix}"
            
            modified_line = re.sub(pattern_standalone, replace_path_standalone, modified_line)
            modified_lines.append(modified_line)
        
        modified_content = '\n'.join(modified_lines)
        
        if modified_content != original_content:
            with open(file_path, 'w', encoding='utf-8') as f:
                f.write(modified_content)
            return True, total_replacements
        
        return False, total_replacements
        
    except Exception as e:
        print(f"Error processing file {file_path}: {e}")
        return False, 0

def find_gn_files(directory):
    """
    Find all .gn and .gni files in directory.
    
    Args:
        directory: Directory to search.
    
    Returns:
        List of file paths.
    """
    gn_files = []
    
    for root, dirs, files in os.walk(directory):
        # Skip hidden directories and node_modules etc.
        dirs[:] = [d for d in dirs if not d.startswith('.') and d != 'node_modules']
        
        for file in files:
            if file.endswith(('.gn', '.gni')):
                gn_files.append(os.path.join(root, file))
    
    return gn_files


def main():
    parser = argparse.ArgumentParser(description='Batch process "//" paths in .gn and .gni files')
    parser.add_argument('directory', nargs='?', default='.', 
                      help='Specify subdirectory to process GN files (default: current directory).')
    parser.add_argument('--root', required=True, help='Project root directory path.')
    
    args = parser.parse_args()
    
    # Get absolute paths.
    target_directory = os.path.abspath(args.directory)
    project_root = os.path.abspath(args.root)
    
    if not os.path.exists(target_directory):
        print(f"Error: Directory {target_directory} does not exist.")
        return 1
    
    if not os.path.exists(project_root):
        print(f"Error: Project root directory {project_root} does not exist.")
        return 1
    
    print(f"Starting directory scan: {target_directory}.")
    print(f"Project root: {project_root}.")
    print("-" * 60)
    
    # Find all .gn and .gni files.
    gn_files = find_gn_files(target_directory)
    
    if not gn_files:
        print("No .gn or .gni files found.")
        return 0
    
    print(f"Found {len(gn_files)} .gn/.gni files.")
    print("-" * 60)
    
    processed_files = 0
    total_replacements = 0
    
    # Process each file.
    for i, file_path in enumerate(gn_files, 1):
        print(f"[{i}/{len(gn_files)}] Processing file: {file_path}.")
        
        modified, replacements = process_file(file_path, project_root)
        
        if modified:
            processed_files += 1
            total_replacements += replacements
            print(f"  ✓ Modified, replaced {replacements} locations.")
        else:
            print(f"  - No modification needed.")
    
    print("-" * 60)
    print(f"Processing complete!")
    print(f"Scanned {len(gn_files)} files.")
    print(f"Processed {processed_files} files.")
    print(f"Total {total_replacements} path replacements.")

    return 0

if __name__ == '__main__':
    sys.exit(main())
