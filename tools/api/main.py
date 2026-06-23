# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

# /usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
from concurrent.futures import ThreadPoolExecutor
import os
import sys
from api_dump import (
    update_api_metadata,
    generate_api_doc,
)
from env_setup import guarantee_generated_files
from api_doc import generate_website_api_doc


def _update_platform_api_metadata(platform):
    return update_api_metadata(
        os.path.dirname(os.path.abspath(__file__)),
        platform,
    )


def _update_api_metadata_for_platforms(platforms):
    if len(platforms) == 1:
        return {platforms[0]: _update_platform_api_metadata(platforms[0])}

    results = {}
    with ThreadPoolExecutor(max_workers=len(platforms)) as executor:
        future_by_platform = {
            platform: executor.submit(_update_platform_api_metadata, platform)
            for platform in platforms
        }
        for platform, future in future_by_platform.items():
            results[platform] = future.result()
    return results


def main():
    """API Metadata Management CLI

    Handles command-line interface for:
    - Updating reference API metadata files
    - Comparing generated metadata with references

    Command Line Arguments:
        --update (-u): Update mode - overwrites reference files
        --platform (-p): Target platform(s) [all|ios|android|harmony]
        --doc: Generate API documentation

    Usage Examples:
        python main.py -u -p all    # Update all platforms
        python main.py -u -p android # Update Android only
    """
    # Initialize argument parser
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-u", "--update", action="store_true", default=False, help="Update API metadata"
    )
    parser.add_argument(
        "-p",
        "--platform",
        choices=["all", "ios", "android", "harmony"],
        default="all",
        help="Specify the platform",
    )
    parser.add_argument(
        "--doc", action="store_true", default=False, help="Generate API doc"
    )

    # Process command line arguments
    args = parser.parse_args()
    ios_result = True
    android_result = True
    harmony_result = True
    all_platforms = ["ios", "android", "harmony"]

    # Handle update operation
    if args.update:
        # Ensure generated files are up-to-date
        guarantee_generated_files()
        platforms = all_platforms if args.platform == "all" else [args.platform]
        platform_results = _update_api_metadata_for_platforms(platforms)
        ios_result = platform_results.get("ios", ios_result)
        android_result = platform_results.get("android", android_result)
        harmony_result = platform_results.get("harmony", harmony_result)
        sys.exit(0 if (ios_result and android_result and harmony_result) else 1)

    # Handle doc operation
    if args.doc:
        # Ensure generated files are up-to-date
        guarantee_generated_files()
        platform_list = []
        if args.platform == "all" or args.platform == "android":
            platform_list.append("android")
        if args.platform == "all" or args.platform == "ios":
            platform_list.append("ios")
        if args.platform == "all" or args.platform == "harmony":
            platform_list.append("harmony")
        result = generate_website_api_doc(platform_list)
        sys.exit(0 if result else 1)


if __name__ == "__main__":
    main()
