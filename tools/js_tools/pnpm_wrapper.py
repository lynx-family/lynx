#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import os
import sys
import platform
import subprocess
from pathlib import Path


def _find_repo_root(start: Path) -> Path:
    """
    Find repository root by walking up until a directory containing 'buildtools'.
    Fallback to walking 6 levels max.
    """
    cur = start.resolve()
    for _ in range(6):
        if (cur / 'buildtools').exists():
            return cur
        if cur.parent == cur:
            break
        cur = cur.parent
    # Fallback for known layout: lynx/tools/js_tools -> lynx -> repo root
    return start.resolve().parent.parent.parent


def _get_buildtools_path(repo_root: Path) -> Path:
    bt = repo_root / 'buildtools'
    if not bt.exists():
        print('Error: Could not find buildtools directory', file=sys.stderr)
        sys.exit(1)
    return bt


def _resolve_pnpm_executable(buildtools_path: Path) -> Path:
    """
    Resolve the pnpm executable/script inside buildtools/pnpm.
    Try multiple candidates for Windows/non-Windows.
    """
    pnpm_dir = buildtools_path / 'pnpm'
    candidates = []

    if platform.system().lower() == 'windows':
        # Prefer .cmd or .exe on Windows
        candidates = [
            pnpm_dir / 'pnpm.cmd',
            pnpm_dir / 'pnpm.exe',
            pnpm_dir / 'pnpm.ps1',
            pnpm_dir / 'pnpm',
        ]
    else:
        candidates = [
            pnpm_dir / 'pnpm',
            pnpm_dir / 'pnpm.sh',
        ]

    for c in candidates:
        if c.exists():
            return c

    print(f'Error: Could not find pnpm executable in {pnpm_dir}', file=sys.stderr)
    sys.exit(1)


def main():
    # Determine repo root (start from this script's directory)
    script_dir = Path(__file__).resolve().parent
    repo_root = _find_repo_root(script_dir)

    buildtools_path = _get_buildtools_path(repo_root)

    # Prepare environment: add node/bin (or node on Windows) to PATH
    env = os.environ.copy()
    node_path = buildtools_path / ('node' if platform.system() == 'Windows' else 'node/bin')
    env['PATH'] = str(node_path) + os.pathsep + env.get('PATH', '')

    pnpm_exec = _resolve_pnpm_executable(buildtools_path)

    # Build the command: pass-through all args to pnpm
    args = [str(pnpm_exec)] + sys.argv[1:]

    # If pnpm_exec is a PowerShell script on Windows, run via powershell
    if platform.system().lower() == 'windows' and pnpm_exec.suffix.lower() == '.ps1':
        args = [
            'powershell',
            '-ExecutionPolicy', 'Bypass',
            '-File', str(pnpm_exec)
        ] + sys.argv[1:]

    try:
        subprocess.check_call(args, env=env)
    except subprocess.CalledProcessError as e:
        # Propagate non-zero exit codes
        sys.exit(e.returncode)


if __name__ == '__main__':
    main()