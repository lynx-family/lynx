#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import os
import platform
import subprocess
import sys


def find_compiler(script_dir):
    compiler_suffix = ".exe" if platform.system() == "Windows" else ""
    current_dir = script_dir
    while True:
        compiler = os.path.join(
            current_dir,
            "buildtools",
            "llvm",
            "bin",
            "clang++" + compiler_suffix,
        )
        if os.path.isfile(compiler):
            return compiler
        parent_dir = os.path.dirname(current_dir)
        if parent_dir == current_dir:
            raise FileNotFoundError(
                "Unable to find the repository clang++ from " + script_dir
            )
        current_dir = parent_dir


def main():
    if len(sys.argv) < 4:
        print(
            "Usage: run.py <tool> <source> <tool arguments...>",
            file=sys.stderr,
        )
        return 1

    tool = sys.argv[1]
    source = sys.argv[2]
    if not os.path.exists(tool) or os.path.getmtime(source) > os.path.getmtime(
            tool):
        tool_dir = os.path.dirname(tool)
        if tool_dir:
            os.makedirs(tool_dir, exist_ok=True)
        environment = os.environ.copy()
        if platform.system() == "Darwin":
            environment.pop("SDKROOT", None)
            command = [
                "xcrun",
                "--sdk",
                "macosx",
                "clang++",
                "-std=c++17",
                source,
                "-o",
                tool,
            ]
        else:
            compiler = find_compiler(os.path.abspath(os.path.dirname(__file__)))
            command = [compiler, "-std=c++17", source, "-o", tool]
        subprocess.check_call(command, env=environment)

    for output in sys.argv[4:]:
        output_dir = os.path.dirname(output)
        if output_dir:
            os.makedirs(output_dir, exist_ok=True)
    return subprocess.call([os.path.abspath(tool)] + sys.argv[3:])


if __name__ == "__main__":
    sys.exit(main())
