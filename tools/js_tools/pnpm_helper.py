#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
import platform
import subprocess
import sys
import time

current_dir = os.path.dirname(os.path.realpath(__file__))
tools_dir = os.path.abspath(os.path.join(current_dir, '../'))
sys.path.append(tools_dir)
from buildtools_helper import get_buildtools_path

# Get development system
system = platform.system().lower()
is_win = system == "windows"
node_bin_path = os.path.join(
    get_buildtools_path(), "node" if is_win else "node/bin")


def get_pnpm_env():
    env = os.environ.copy()
    env["PATH"] = rf"{node_bin_path}{os.pathsep}{os.environ['PATH']}"
    return env

class SimpleFileLock:
    def __init__(self, path: str, timeout: float = 1.0, poll_interval: float = 0.2):
        print(f"init SimpleFileLock: {path}")
        self.path = path
        self.timeout = timeout
        self.poll_interval = poll_interval
        self._acquired = False

    def _pid_alive(self, pid: int) -> bool:
        if pid <= 0:
            return False
        try:
            os.kill(pid, 0)  # 进程不存在会抛异常；存在则返回
            return True
        except Exception:
            return False
    
    def acquire(self):
        print(f"acquire SimpleFileLock")
        start = time.monotonic()
        while True:
            try:
                fd = os.open(self.path, os.O_CREAT | os.O_EXCL | os.O_WRONLY)
                try:
                    os.write(fd, str(os.getpid()).encode())
                finally:
                    os.close(fd)
                self._acquired = True
                return
            except FileExistsError:
                print(f"lock file exists.")
                try:
                    with open(self.path, "rb") as f:
                        data = f.read().strip()
                    pid = int(data) if data else -1
                except Exception:
                    pid = -1
                if not self._pid_alive(pid):
                    try:
                        os.unlink(self.path)
                        continue
                    except Exception:
                        pass
                if (time.monotonic() - start) >= self.timeout:
                    raise TimeoutError(f"Timeout acquiring lock: {self.path}")
                time.sleep(self.poll_interval)

    def release(self):
        print(f"release SimpleFileLock")
        if self._acquired:
            try:
                os.unlink(self.path)
            except FileNotFoundError:
                pass
            self._acquired = False

    def __enter__(self):
        print(f"enter SimpleFileLock")
        self.acquire()
        return self

    def __exit__(self, exc_type, exc, tb):
        print(f"exit SimpleFileLock")
        self.release()

def run_pnpm_command(command, cwd, env=None):
    if env is None:
        env = get_pnpm_env().copy()

    env.setdefault("COREPACK_HOME", os.path.join(get_buildtools_path(), "corepack"))
    env.setdefault("COREPACK_ENABLE_NETWORK", "0")
    # corepack_exec = os.path.join(node_bin_path, "corepack.CMD" if is_win else "corepack")
    # pnpm_exec = os.path.join(node_bin_path, "pnpm.CMD" if is_win else "pnpm")
    npm_exec = os.path.join(node_bin_path, "npm.CMD" if is_win else "npm")
    command[0] = os.path.join(node_bin_path, "pnpm.CMD" if is_win else "pnpm")
    pnpm_command_str = ' '.join(command)
    npm_config_cmd = f"{npm_exec} config set strict-ssl false"
    # Force corepack to use the pnpm version specified by the build tool
    full_command = f"corepack prepare pnpm@7.33.6 --activate && {npm_config_cmd} && {pnpm_command_str}"
    subprocess.check_call(
        full_command,
        cwd=cwd,
        shell=True,
        env=env
    )

    # with SimpleFileLock(os.path.join(env["COREPACK_HOME"], "prepare.lock"), timeout=1):
    #     subprocess.check_call(
    #         [corepack_exec, "prepare", "pnpm@7.33.6", "--activate"],
    #         cwd=cwd,
    #         env=env
    #     )

    # cmd = [pnpm_exec] + command[1:] if command else [pnpm_exec]
    # subprocess.check_call(cmd, cwd=cwd, env=env)
# def run_pnpm_command(command, cwd, env=get_pnpm_env()):
#     command[0] = os.path.join(node_bin_path, "pnpm.CMD" if is_win else "pnpm")
#     subprocess.check_call(
#         command,
#         cwd=cwd,
#         env=env,
#     )