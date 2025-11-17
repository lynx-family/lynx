# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

# /usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import subprocess
import sys


def check_and_run():
    try:
        import yaml
        import jinja2
        from gen_config import (
            gen_page_config_decode,
            gen_lynx_config,
            parse_config,
            gen_config_types,
            gen_compile_options,
            gen_compile_options_types,
        )

        configs = parse_config()
        if not configs:
            sys.exit(-1)

        gen_page_config_decode(configs)
        gen_lynx_config(configs)
        gen_compile_options()
        gen_config_types(configs)
        gen_compile_options_types()
        sys.exit(0)
    except ImportError:
        print("Required dependencies not found. Running gen_config.py...")
        # The directory of this script
        this_dir = os.path.dirname(os.path.abspath(__file__))
        # The project root directory
        root_dir = os.path.abspath(
            os.path.join(this_dir, os.pardir, os.pardir, os.pardir)
        )

        if sys.platform == "win32":
            python_executable = os.path.join(root_dir, ".venv", "Scripts", "python.exe")
            envsetup_script = os.path.join(root_dir, "tools", "envsetup.ps1")
        else:
            python_executable = os.path.join(root_dir, ".venv", "bin", "python3")
            envsetup_script = os.path.join(root_dir, "tools", "envsetup.sh")
        gen_config_script = os.path.join(this_dir, "gen_config.py")

        # Execute envsetup_script and gen_config_script in the same shell environment
        if sys.platform == "win32":
            command = f'powershell -ExecutionPolicy Bypass -File "{envsetup_script}" & "{python_executable}" "{gen_config_script}"'
        else:
            command = f'bash -c "source "{envsetup_script}" && "{python_executable}" "{gen_config_script}""'

        print(f"Executing command: {command}")
        result = subprocess.run(command, shell=True, cwd=root_dir)

        if result.returncode != 0:
            print(f"Error: Failed to execute command. Exit code: {result.returncode}")
            sys.exit(1)


if __name__ == "__main__":
    check_and_run()
