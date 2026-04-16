# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
from typing import Callable

from core.builder.builder import Builder
from core.env.env import RTFEnv
from core.target.target import Target
from core.utils.log import Log
from core.utils.shell_runner import run_command_with_error_log
from core.base.result import Err, Ok
from core.base.constants import Constants


class GradleBuilder(Builder):
    def __init__(self, args: [str], workspace: str, silent: bool = False):
        super().__init__()
        self.args = args
        self.workspace = workspace
        self.silent = silent

    def pre_action(self, skip: Callable[[], bool] = None):
        if skip is None or not skip():
            command = "./gradlew clean"
            result = run_command_with_error_log(
                command, cwd=self.workspace, silent=self.silent, error_msg="gradlew clean failed"
            )
            if result.returncode != 0:
                return Err(Constants.CALL_COMMAND_ERR, f"run {command} failed")
        return Ok()

    def build(self, target: Target):
        for task in target.build_tasks:
            build_cmd = f"./gradlew {task} {' '.join(self.args)}"
            Log.info(f"{task} start build!")
            result = run_command_with_error_log(
                build_cmd, cwd=self.workspace, silent=self.silent, error_msg=f"{task} build failed"
            )
            if result.returncode != 0:
                return Err(Constants.CALL_COMMAND_ERR, f"{task} build failed!")
            target.target_path = os.path.join(
                RTFEnv.get_project_root_path(), target.params["apk"]
            )
            Log.success(f"{task} build success!")
            return Ok()
