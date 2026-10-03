#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import os
import platform
import shutil
import subprocess
import sys
import tempfile


def find_visual_studio():
    install_dir = os.environ.get("GYP_MSVS_OVERRIDE_PATH")
    if not install_dir:
        return None
    vcvars = os.path.join(install_dir, "VC", "Auxiliary", "Build", "vcvars64.bat")
    return vcvars if os.path.isfile(vcvars) else None


def find_compiler(script_dir):
    compiler_names = (
        ("clang++.exe", "clang.exe", "clang-cl.exe")
        if platform.system() == "Windows"
        else ("clang++",)
    )
    current_dir = script_dir
    while True:
        for compiler_name in compiler_names:
            compiler = os.path.join(
                current_dir,
                "buildtools",
                "llvm",
                "bin",
                compiler_name,
            )
            if os.path.isfile(compiler):
                return compiler
        parent_dir = os.path.dirname(current_dir)
        if parent_dir == current_dir:
            for compiler_name in compiler_names:
                compiler = shutil.which(compiler_name)
                if compiler:
                    return compiler
            raise FileNotFoundError("Unable to find a host C++ compiler")
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
            vcvars = find_visual_studio() if platform.system() == "Windows" else None
            if vcvars:
                with tempfile.NamedTemporaryFile(
                        mode="w", suffix=".bat", delete=False) as batch_file:
                    batch_file.write(f'@call "{vcvars}"\n')
                    batch_file.write("@if errorlevel 1 exit /b %errorlevel%\n")
                    batch_file.write("@cl.exe /std:c++17 /EHsc "
                                     f'"{source}" /Fe:"{tool}"\n')
                    batch_path = batch_file.name
                try:
                    subprocess.check_call(
                        ["cmd.exe", "/d", "/c", batch_path],
                        env=environment,
                    )
                finally:
                    os.remove(batch_path)
                command = None
            else:
                compiler = find_compiler(
                    os.path.abspath(os.path.dirname(__file__))
                )
                if os.path.basename(compiler).lower() == "clang-cl.exe":
                    command = [compiler, "/std:c++17", source, "/Fe:" + tool]
                else:
                    command = [compiler, "-std=c++17", source, "-o", tool]
        if command:
            subprocess.check_call(command, env=environment)

    for output in sys.argv[4:]:
        output_dir = os.path.dirname(output)
        if output_dir:
            os.makedirs(output_dir, exist_ok=True)
    return subprocess.call([os.path.abspath(tool)] + sys.argv[3:])


if __name__ == "__main__":
    sys.exit(main())
