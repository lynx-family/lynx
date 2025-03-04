# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os.path
import subprocess
from datetime import datetime
from core.env.env import RTFEnv
from core.target.target import Target
from core.utils.log import Log


class NativeUTTarget(Target):
    def __init__(self, params, name):
        super().__init__(params, name)

    def init_self_info(self):
        if "with_subprocess" in self.params and self.params["with_subprocess"]:
            self.coverage_data_path = os.path.join(
                RTFEnv.get_env("project_root_path"), f"{self.name}%9m%p.profraw"
            )
        else:
            self.coverage_data_path = os.path.join(
                RTFEnv.get_env("project_root_path"), f"{self.name}.profraw"
            )
        self.cwd = (
            self.params["cwd"]
            if "cwd" in self.params
            else RTFEnv.get_env("project_root_path")
        )

        self.args = self.params["args"] if "args" in self.params else []
        self.custom_run_cmd = (
            self.params["custom_run_cmd"] if "custom_run_cmd" in self.params else None
        )

        self.retry = int(self.params["retry"]) if "retry" in self.params else 0

        self.build_tasks.append(self.name)

    def get_coverage_raw_data(self):
        if "with_subprocess" in self.params and self.params["with_subprocess"]:
            path = os.path.dirname(self.coverage_data_path)
            pattern = f"{self.name}*.profraw"
            find_cmd = f"find {path} -name '{pattern}'"
            find_result = subprocess.check_output([find_cmd], shell=True)
            return find_result.decode("utf-8").split("\n")
        else:
            return [self.coverage_data_path]

    def get_run_cmd(self):
        if self.custom_run_cmd is not None:
            return (
                f'LLVM_PROFILE_FILE="{self.coverage_data_path}" {self.custom_run_cmd}'
            )
        return f'LLVM_PROFILE_FILE="{self.coverage_data_path}" {self.target_path} {" ".join(self.args)}'

    def run(self):
        Log.info(f"{self.name} start run")
        run_cmd = self.get_run_cmd()
        log_file = open(self.log_file, "w+")
        self.start_time = datetime.timestamp(datetime.now())
        self.process = subprocess.Popen(
            [run_cmd], shell=True, cwd=self.cwd, stderr=log_file, stdout=log_file
        )
        return self

    def run_pre_actions(self):
        for action in self.pre_actions:
            Log.info(f"Run pre action {action} for {self.name}")
            result = subprocess.check_call(action, shell=True, cwd=self.cwd)
            if result != 0:
                Log.fatal(f"Run pre action {action} failed!")
