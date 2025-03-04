# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
import sys

def check_license_files(root_folder='third_party'):
    """
    Check if all folders under 'root_folder' contain a 'LICENSE' file.
    """
    missing_license = []

    # Iterate through each direct sub-folder in the root folder
    for folder in os.listdir(root_folder):
        folder_path = os.path.join(root_folder, folder)
        # Check if it's a directory
        if os.path.isdir(folder_path):
            # Check if 'LICENSE' file exists in the current directory
            if 'LICENSE' not in os.listdir(folder_path):
                missing_license.append(folder_path)

    return missing_license

def main():
    missing = check_license_files()
    if missing:
        print(f"Folders missing LICENSE file: {missing}")
        return 1
    else:
        print("All folders contain a LICENSE file.")
        return 0

if __name__ == "__main__":
    sys.exit(main())
