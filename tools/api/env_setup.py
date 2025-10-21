# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

# /usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import yaml
import subprocess

LYNX_ROOT_PATH = os.getcwd()
TOOLS_PATH = os.path.join(LYNX_ROOT_PATH, "tools")
ANDROID_API_PATH = os.path.join(LYNX_ROOT_PATH, "platform", "android", "api")
IOS_API_PATH = os.path.join(LYNX_ROOT_PATH, "platform", "darwin", "ios", "api")
HARMONY_API_PATH = os.path.join(LYNX_ROOT_PATH, "platform", "harmony", "api")
ERROR_CODE_PACKAGE_DIR = os.path.join(LYNX_ROOT_PATH, "tools", "error_code")
FEATURE_COUNT_PACKAGE_DIR = os.path.join(LYNX_ROOT_PATH, "tools", "feature_count")
TOOLS_DIR_PATH = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir))
CSS_GENERATOR_PATH = os.path.join(TOOLS_DIR_PATH, "css_generator")
PERFORMANCE_OBSERVER_PATH = os.path.join(
    TOOLS_DIR_PATH, "performance", "performance_observer"
)
API_DOC_ANNOTATION = """/*
 * This file is generated, do not edit.
 * @generated
 *
 * @generate-command: python3 tools/api/main.py -u
 *
 */\n
"""

API_CONFIG = None
API_CONFIG_PATH = os.path.abspath(
    os.path.join(LYNX_ROOT_PATH, "tools", "api", "config.yml")
)
with open(API_CONFIG_PATH, "r") as f:
    API_CONFIG = yaml.safe_load(f)
    DOXYGEN_PATH = os.path.normpath(
        os.path.join(LYNX_ROOT_PATH, API_CONFIG["path"]["doxygen"])
    )
    if not os.path.exists(DOXYGEN_PATH):
        DOXYGEN_PATH = os.path.join(
            LYNX_ROOT_PATH, os.pardir, API_CONFIG["path"]["doxygen"]
        )

    HANDLE_FAILED_INSTRUCTION = API_CONFIG["path"]["instruction_doc"]

    NODE_PATH = os.path.normpath(
        os.path.join(LYNX_ROOT_PATH, API_CONFIG["path"]["node"])
    )
    if not os.path.exists(NODE_PATH):
        NODE_PATH = os.path.join(LYNX_ROOT_PATH, os.pardir, API_CONFIG["path"]["node"])

    CLANG_FORMAT_PATH = os.path.normpath(
        os.path.join(LYNX_ROOT_PATH, API_CONFIG["path"]["llvm"], "bin", "clang-format")
    )
    if not os.path.exists(CLANG_FORMAT_PATH):
        CLANG_FORMAT_PATH = os.path.join(
            LYNX_ROOT_PATH, os.pardir, API_CONFIG["path"]["llvm"], "bin", "clang-format"
        )


def guarantee_generated_files():
    """
    Generate error code and feature count files.
    """
    sys.path.append(TOOLS_PATH)
    sys.path.append(ERROR_CODE_PACKAGE_DIR)
    from error_code.gen_error_code import main as gen_error_code

    sys.path.remove(ERROR_CODE_PACKAGE_DIR)
    gen_error_code()

    sys.path.append(FEATURE_COUNT_PACKAGE_DIR)
    from feature_count.generate_feature_count import main as generate_feature_count

    sys.path.remove(FEATURE_COUNT_PACKAGE_DIR)
    sys.path.remove(TOOLS_PATH)
    generate_feature_count()

    sys.path.append(TOOLS_DIR_PATH)
    sys.path.append(CSS_GENERATOR_PATH)
    from css_generator.css_parser_generator import main as generate_css_parser

    sys.path.remove(CSS_GENERATOR_PATH)
    generate_css_parser()

    try:
        process = subprocess.run(
            [
                sys.executable,
                os.path.join(
                    PERFORMANCE_OBSERVER_PATH, "generate_performance_entry.py"
                ),
            ],
            check=True,
            capture_output=True,
            text=True,
        )
        print(process.stdout)
    except subprocess.CalledProcessError as e:
        print(f"generate performance entry failed: {e.stderr}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    guarantee_generated_files()
