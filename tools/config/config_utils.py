# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

# /usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import subprocess

_this_dir = os.path.dirname(os.path.abspath(__file__))
_root_dir = os.path.abspath(os.path.join(_this_dir, os.pardir, os.pardir, os.pardir))
if sys.platform == "win32":
    CLANG_FORMAT_PATH = os.path.join(
        _root_dir, "buildtools", "llvm", "clang-format.exe"
    )
    if not os.path.exists(CLANG_FORMAT_PATH):
        CLANG_FORMAT_PATH = os.path.join(
            _root_dir, "lynx", "buildtools", "llvm", "clang-format.exe"
        )
else:
    CLANG_FORMAT_PATH = os.path.join(
        _root_dir, "buildtools", "llvm", "bin", "clang-format"
    )
    if not os.path.exists(CLANG_FORMAT_PATH):
        CLANG_FORMAT_PATH = os.path.join(
            _root_dir, "lynx", "buildtools", "llvm", "bin", "clang-format"
        )


def clang_format(file: str, style="Google", file_extension=None) -> str:
    try:
        if file_extension is not None:
            process = subprocess.run(
                [
                    CLANG_FORMAT_PATH,
                    f"--style={style}",
                    f"--assume-filename={file_extension}",
                ],
                input=file,
                text=True,
                capture_output=True,
                check=True,
            )
        else:
            process = subprocess.run(
                [CLANG_FORMAT_PATH, f"--style={style}", "-i", file],
                text=True,
                capture_output=True,
                check=True,
            )
        return process.stdout
    except subprocess.CalledProcessError as e:
        print(f"clang format failed: {e.stderr}", file=sys.stderr)
        return file
    except FileNotFoundError:
        print(f"{CLANG_FORMAT_PATH} not found", file=sys.stderr)
        return file
