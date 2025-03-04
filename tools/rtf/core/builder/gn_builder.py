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


class GnBuilder(Builder):
    def __init__(self, args: [str], output: str):
        super().__init__()
        self.args = args
        self.output = output

    def pre_action(self, skip: Callable[[], bool] = None):
        gn_gen_cmd = f'gn gen {self.output} --args="{" ".join(self.args)}"'
        subprocess.check_call(gn_gen_cmd, shell=True)

    def build(self, target: Target):
        Log.info(f"{target.name} start build!")
        for task in target.build_tasks:
            build_cmd = f"ninja -C {self.output} {task}"
            try:
                subprocess.check_call(build_cmd, shell=True)
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
            except Exception as e:
                Log.fatal(f"{task} build failed!\n{e}")
