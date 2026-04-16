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


class GnBuilder(Builder):
    def __init__(self, args: [str], output: str, silent: bool = False):
        super().__init__()
        self.args = args
        self.output = output
        self.silent = silent

    def pre_action(self, skip: Callable[[], bool] = None):
        gn_gen_cmd = f'gn gen {self.output} --args="{" ".join(self.args)}"'
        result = run_command_with_error_log(
            gn_gen_cmd, silent=self.silent, error_msg="gn gen failed"
        )
        if result.returncode != 0:
            return Err(Constants.CALL_COMMAND_ERR, f"run {gn_gen_cmd} failed")
        return Ok()

    def build(self, target: Target):
        Log.info(f"{target.name} start build!")
        for task in target.build_tasks:
            build_cmd = f"ninja -C {self.output} {task}"
            result = run_command_with_error_log(
                build_cmd, silent=self.silent, error_msg=f"{task} build failed"
            )
            if result.returncode != 0:
                return Err(Constants.CALL_COMMAND_ERR, f"{task} build failed!")
            if RTFEnv.env_map["os"] == "linux":
                target.target_path = os.path.join(
                    RTFEnv.get_project_root_path(),
                    self.output,
                    "exe.unstripped",
                    task,
                )
            else:
                target.target_path = os.path.join(
                    RTFEnv.get_project_root_path(), self.output, task
                )
            Log.success(f"{task} build success!")
            return Ok()
