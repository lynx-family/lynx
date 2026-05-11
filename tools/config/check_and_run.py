#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

# -*- coding: utf-8 -*-

"""
A wrapper script that attempts to run gen_config.py directly.
If dependencies are missing, it falls back to running via subprocess
with environment setup.
"""

from __future__ import annotations

import shlex
import subprocess
import sys
from pathlib import Path
from typing import List, Tuple, Union

# Directories relative to this script
SCRIPT_DIR = Path(__file__).resolve().parent
ROOT_DIR = SCRIPT_DIR.parent.parent

# Platform-specific paths
IS_WINDOWS = sys.platform == "win32"
PYTHON_EXECUTABLE = Path(sys.executable) if sys.executable else Path("python3")
ENV_SCRIPT = (
    ROOT_DIR / "tools" / "env.ps1"
    if IS_WINDOWS
    else ROOT_DIR / "tools" / "env.sh"
)
GEN_CONFIG_SCRIPT = SCRIPT_DIR / "gen_config.py"


def should_generate_all() -> bool:
    """Check if --all flag is provided."""
    return len(sys.argv) > 1 and sys.argv[1] == "--all"


def run_directly() -> bool:
    """
    Attempt to run gen_config.py directly using imported modules.
    Returns True if successful, False if dependencies are missing.
    """
    try:
        import yaml  # noqa: F401
        import jinja2  # noqa: F401
        from gen_config import gen_lynx_config, parse_config, gen_types
    except ImportError:
        return False

    configs, compiler_options = parse_config()
    if not configs:
        sys.exit(-1)

    gen_lynx_config(configs, compiler_options)
    if should_generate_all():
        gen_types(configs, compiler_options)

    return True


def build_fallback_command(params: List[str]) -> Tuple[Union[str, List[str]], bool]:
    """
    Build the command for running gen_config.py with environment setup.
    Returns a tuple of (command, shell_flag).
    """
    if IS_WINDOWS:
        # PowerShell command
        # We construct the PowerShell command string to execute in the new scope
        # Syntax: & 'path step1'; & 'path step2' args
        # Note: We use single quotes for paths to handle spaces
        # We rely on subprocess.run passing the entire command block safely to -Command

        # Prepare params string for inside the PowerShell command
        # We need to escape single quotes if present in params, though rare for current usage
        safe_params = " ".join(f"'{p}'" for p in params)

        ps_command = (
            f"& '{ENV_SCRIPT}' -- '{PYTHON_EXECUTABLE}' '{GEN_CONFIG_SCRIPT}' {safe_params}"
        ).strip()

        # Return list for shell=False
        return [
            "powershell",
            "-NoProfile",
            "-ExecutionPolicy",
            "Bypass",
            "-Command",
            ps_command,
        ], False
    else:
        # Bash command
        # Use shlex to safely quote paths and arguments
        # base_cmd = f"source {ENV_SCRIPT} -- && {PYTHON_EXECUTABLE} {GEN_CONFIG_SCRIPT}"

        safe_env = shlex.quote(str(ENV_SCRIPT))
        safe_python = shlex.quote(str(PYTHON_EXECUTABLE))
        safe_gen_config = shlex.quote(str(GEN_CONFIG_SCRIPT))
        safe_params = " ".join(shlex.quote(p) for p in params)

        full_cmd = f"source {safe_env} -- && {safe_python} {safe_gen_config} {safe_params}".strip()

        # Return string for shell=True (needed for 'source')
        return f'/bin/bash -c "{full_cmd}"', True


def run_via_subprocess() -> None:
    """Run gen_config.py via subprocess when direct import fails."""
    print("Required dependencies not found. Running gen_config.py via subprocess...")

    if not GEN_CONFIG_SCRIPT.exists():
        print(f"Error: Script not found at {GEN_CONFIG_SCRIPT}")
        sys.exit(1)

    params = ["--all"] if should_generate_all() else []
    command, use_shell = build_fallback_command(params)

    # For display purposes, join list if it's a list
    display_cmd = command if isinstance(command, str) else " ".join(command)
    print(f"Executing command: {display_cmd}")

    try:
        result = subprocess.run(command, shell=use_shell, cwd=ROOT_DIR)
        sys.exit(result.returncode)
    except Exception as e:
        print(f"Error: Failed to execute subprocess: {e}")
        sys.exit(1)


def main() -> None:
    """Main entry point."""
    if not run_directly():
        run_via_subprocess()


if __name__ == "__main__":
    main()
