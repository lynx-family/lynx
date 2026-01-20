# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

# /usr/bin/env python3
# -*- coding: utf-8 -*-

import subprocess
import sys
from pathlib import Path


def check_and_run():
    try:
        import yaml
        import jinja2
        from gen_config import gen_lynx_config, parse_config, gen_types

        configs, compiler_options = parse_config()
        if not configs:
            sys.exit(-1)

        gen_lynx_config(configs, compiler_options)
        if len(sys.argv) > 1 and sys.argv[1] == "--all":
            gen_types(configs, compiler_options)

        sys.exit(0)
    except ImportError:
        print("Required dependencies not found. Running gen_config.py...")
        # The directory of this script
        this_dir = Path(__file__).resolve().parent
        # The project root directory
        root_dir = this_dir.parent.parent

        if sys.platform == "win32":
            python_executable = root_dir / ".venv" / "Scripts" / "python.exe"
            envsetup_script = root_dir / "tools" / "envsetup.ps1"
        else:
            python_executable = root_dir / ".venv" / "bin" / "python3"
            envsetup_script = root_dir / "tools" / "envsetup.sh"
        gen_config_script = this_dir / "gen_config.py"
        params = ""
        if len(sys.argv) > 1 and sys.argv[1] == "--all":
            params = " --all"

        # Execute envsetup_script and gen_config_script in the same shell environment
        if sys.platform == "win32":
            command = f'powershell -ExecutionPolicy Bypass -File "{envsetup_script}" & "{python_executable}" "{gen_config_script}""{params}"'
        else:
            command = f'bash -c "source {envsetup_script} && {python_executable} {gen_config_script}{params}"'

        print(f"Executing command: {command}")
        result = subprocess.run(command, shell=True, cwd=root_dir)

        if result.returncode != 0:
            print(f"Error: Failed to execute command. Exit code: {result.returncode}")
            sys.exit(1)


if __name__ == "__main__":
    check_and_run()
