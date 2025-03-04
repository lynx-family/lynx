# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os
import subprocess
from typing import Callable

from core.builder.builder import Builder
from core.env.env import RTFEnv
from core.target.target import Target
from core.utils.log import Log


class GradleBuilder(Builder):
    def __init__(self, args: [str], workspace: str):
        super().__init__()
        self.args = args
        self.workspace = workspace

    def pre_action(self, skip: Callable[[], bool] = None):
        if skip is None or not skip():
            command = "./gradlew clean"
            subprocess.check_call(command, shell=True, cwd=self.workspace)

    def build(self, target: Target):
        for task in target.build_tasks:
            build_cmd = f"./gradlew {task} {' '.join(self.args)}"
            try:
                Log.info(f"{task} start build!")
                subprocess.check_call(build_cmd, shell=True, cwd=self.workspace)
                target.target_path = os.path.join(
                    RTFEnv.get_project_root_path(), target.params["apk"]
                )
                Log.success(f"{task} build success!")
            except Exception as e:
                Log.fatal(f"{task} build failed!\n{e}")
