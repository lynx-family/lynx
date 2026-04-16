#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import os
import sys
import platform
import subprocess
import re
import hashlib
import tarfile
import tempfile
import urllib.request
from pathlib import Path
from typing import Optional


PNPM9_VERSION = "9.0.0"
PNPM9_URL = f"https://registry.npmjs.org/pnpm/-/pnpm-{PNPM9_VERSION}.tgz"
PNPM9_SHA256 = "bdfc9a7b372b5c462176993e586492603e20da5864d2f8881edc2462482c76fa"


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


def _resolve_target_dir(args: list[str]) -> Path:
    cur = Path.cwd()
    target = cur
    index = 0
    while index < len(args):
        arg = args[index]
        value: Optional[str] = None
        if arg in {'-C', '--dir', '--prefix'} and index + 1 < len(args):
            value = args[index + 1]
            index += 1
        elif arg.startswith('-C='):
            value = arg.split('=', 1)[1]
        elif arg.startswith('--dir='):
            value = arg.split('=', 1)[1]
        elif arg.startswith('--prefix='):
            value = arg.split('=', 1)[1]

        if value:
            candidate = Path(value)
            target = (candidate if candidate.is_absolute() else (cur / candidate)).resolve()
        index += 1

    return target if target.is_dir() else target.parent


def _walk_to_repo_root(target_dir: Path, repo_root: Path):
    cur = target_dir.resolve()
    while True:
        yield cur
        if cur == repo_root or cur.parent == cur:
            return
        cur = cur.parent


def _read_lockfile_major(lockfile_path: Path) -> Optional[int]:
    try:
        for line in lockfile_path.read_text(encoding='utf-8').splitlines():
            if line.startswith('lockfileVersion:'):
                raw = line.split(':', 1)[1].strip().strip('"\'')
                match = re.match(r'(\d+)', raw)
                if match:
                    return int(match.group(1))
                return None
    except OSError:
        return None
    return None


def _select_pnpm_major(target_dir: Path, repo_root: Path) -> int:
    for cur in _walk_to_repo_root(target_dir, repo_root):
        lockfile_major = _read_lockfile_major(cur / 'pnpm-lock.yaml')
        if lockfile_major is not None:
            return lockfile_major
    return 7


def _compute_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open('rb') as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()


def _download_file(url: str, destination: Path) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    with urllib.request.urlopen(url) as response, destination.open('wb') as handle:
        while True:
            chunk = response.read(1024 * 1024)
            if not chunk:
                return
            handle.write(chunk)


def _resolve_pnpm9_cjs(buildtools_path: Path) -> Path:
    bundled_pnpm_cjs = buildtools_path / 'pnpm' / PNPM9_VERSION / 'package' / 'bin' / 'pnpm.cjs'
    if bundled_pnpm_cjs.exists():
        return bundled_pnpm_cjs

    cache_root = Path(
        os.environ.get(
            'LYNX_PNPM_WRAPPER_CACHE_DIR',
            os.path.join(tempfile.gettempdir(), 'lynx-pnpm-wrapper'),
        )
    )
    version_root = cache_root / PNPM9_VERSION
    tarball_path = version_root / f'pnpm-{PNPM9_VERSION}.tgz'
    cached_pnpm_cjs = version_root / 'package' / 'bin' / 'pnpm.cjs'

    if not cached_pnpm_cjs.exists():
        if not tarball_path.exists() or _compute_sha256(tarball_path) != PNPM9_SHA256:
            print(
                f'Info: pnpm {PNPM9_VERSION} is not bundled; downloading official tarball from {PNPM9_URL}',
                file=sys.stderr,
            )
            _download_file(PNPM9_URL, tarball_path)
        actual_sha256 = _compute_sha256(tarball_path)
        if actual_sha256 != PNPM9_SHA256:
            print(
                f'Error: pnpm {PNPM9_VERSION} tarball sha256 mismatch: expected {PNPM9_SHA256}, got {actual_sha256}',
                file=sys.stderr,
            )
            sys.exit(1)
        version_root.mkdir(parents=True, exist_ok=True)
        with tarfile.open(tarball_path, mode='r:gz') as archive:
            archive.extractall(path=version_root)

    if not cached_pnpm_cjs.exists():
        print(f'Error: Could not prepare pnpm {PNPM9_VERSION} package at {cached_pnpm_cjs}', file=sys.stderr)
        sys.exit(1)

    return cached_pnpm_cjs


def _resolve_pnpm_command(buildtools_path: Path, pnpm_major: int) -> list[str]:
    if pnpm_major >= 9:
        return ['node', str(_resolve_pnpm9_cjs(buildtools_path))]

    return [str(_resolve_pnpm_executable(buildtools_path))]


def _should_ignore_workspace(target_dir: Path, repo_root: Path, pnpm_major: int, argv: list[str]) -> bool:
    if pnpm_major < 9 or target_dir == repo_root:
        return False
    if not (target_dir / 'pnpm-lock.yaml').exists():
        return False
    if (target_dir / 'pnpm-workspace.yaml').exists():
        return False
    if '--ignore-workspace' in argv:
        return False

    for arg in argv:
        if not arg.startswith('-'):
            return arg in {'install', 'i'}
    return False


def main():
    # Determine repo root (start from this script's directory)
    script_dir = Path(__file__).resolve().parent
    repo_root = _find_repo_root(script_dir)

    buildtools_path = _get_buildtools_path(repo_root)

    # Prepare environment: add node/bin (or node on Windows) to PATH
    env = os.environ.copy()
    node_path = buildtools_path / ('node' if platform.system() == 'Windows' else 'node/bin')
    env['PATH'] = str(node_path) + os.pathsep + env.get('PATH', '')
    target_dir = _resolve_target_dir(sys.argv[1:])
    pnpm_major = _select_pnpm_major(target_dir, repo_root)
    pnpm_cmd = _resolve_pnpm_command(buildtools_path, pnpm_major)

    # Build the command: pass-through all args to pnpm
    args = pnpm_cmd + sys.argv[1:]
    if _should_ignore_workspace(target_dir, repo_root, pnpm_major, sys.argv[1:]):
        args.append('--ignore-workspace')

    # If pnpm_exec is a PowerShell script on Windows, run via powershell
    if (
        platform.system().lower() == 'windows'
        and len(pnpm_cmd) == 1
        and Path(pnpm_cmd[0]).suffix.lower() == '.ps1'
    ):
        args = [
            'powershell',
            '-ExecutionPolicy', 'Bypass',
            '-File', pnpm_cmd[0]
        ] + sys.argv[1:]

    try:
        subprocess.check_call(args, env=env)
    except subprocess.CalledProcessError as e:
        # Propagate non-zero exit codes
        sys.exit(e.returncode)


if __name__ == '__main__':
    main()
