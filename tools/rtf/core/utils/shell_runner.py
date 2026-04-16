# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import subprocess

from core.utils.log import Log


def run_command(cmd, shell=True, cwd=None, silent=False):
    """Run a shell command. When silent, stdout is discarded and stderr is captured.

    In silent mode, stdout is discarded immediately to avoid buffering large build
    outputs in memory. stderr is captured for error reporting.
    """
    kwargs = {"shell": shell}
    if cwd is not None:
        kwargs["cwd"] = cwd
    if silent:
        result = subprocess.run(
            cmd, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, **kwargs
        )
    else:
        result = subprocess.run(cmd, **kwargs)
    return result


def run_command_with_error_log(cmd, shell=True, cwd=None, silent=False, error_msg=None):
    """Run a shell command and log stderr on failure when silent."""
    result = run_command(cmd, shell=shell, cwd=cwd, silent=silent)
    if result.returncode != 0 and silent and result.stderr:
        stderr_text = result.stderr.decode("utf-8") if isinstance(result.stderr, bytes) else result.stderr
        Log.error(f"{error_msg or 'Command failed'}:\n{stderr_text}")
    return result
