# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

# /usr/bin/env python3
# -*- coding: utf-8 -*-

from pathlib import Path
import sys
import shutil
import subprocess

_this_dir = Path(__file__).resolve().parent
_root_dir = _this_dir.parents[2]
_clang_format_binary = "clang-format.exe" if sys.platform == "win32" else "clang-format"


def _get_clang_format_path():
    # 1. Check pre-defined paths in the repository
    candidate_paths = [
        _root_dir / "buildtools" / "llvm" / _clang_format_binary,
        _root_dir / "lynx" / "buildtools" / "llvm" / _clang_format_binary,
    ]
    for path in candidate_paths:
        if path.exists():
            return str(path)

    # 2. Fallback to system PATH
    return shutil.which(_clang_format_binary) or _clang_format_binary


CLANG_FORMAT_PATH = _get_clang_format_path()


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


def sort_by_deprecated_and_alphabetical(configs):
    configs_sorted = configs.copy()

    valid_configs = [config for config in configs_sorted if not config.deprecated]
    deprecated_configs = [config for config in configs_sorted if config.deprecated]

    valid_configs.sort(key=lambda c: c.name.lower())
    deprecated_configs.sort(key=lambda c: c.name.lower())

    configs_sorted = valid_configs + deprecated_configs
    return configs_sorted
