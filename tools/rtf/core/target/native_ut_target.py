# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os.path
import subprocess
from datetime import datetime
from core.env.env import RTFEnv
from core.target.target import Target
from core.utils.log import Log
from core.utils.shell_runner import run_command_with_error_log
from core.base.result import Err, Ok
from core.base.constants import Constants
from core.base.summary import Summary


class NativeUTTarget(Target):
    def __init__(self, params, name, gtest_filter=None, silent=False):
        self.gtest_filter = gtest_filter
        self.silent = silent
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
        gtest_filter_arg = ""
        if self.gtest_filter:
            gtest_filter_arg = f" --gtest_filter={self.gtest_filter}"
        if self.custom_run_cmd is not None:
            return (
                f'LLVM_PROFILE_FILE="{self.coverage_data_path}" {self.custom_run_cmd}{gtest_filter_arg}'
            )
        return f'LLVM_PROFILE_FILE="{self.coverage_data_path}" {self.target_path} {" ".join(self.args)}{gtest_filter_arg}'

    def run(self):
        Log.info(f"{self.name} start run")
        run_cmd = self.get_run_cmd()
        log_file = open(self.log_file, "w+")
        self.start_time = datetime.timestamp(datetime.now())
        self.process = subprocess.Popen(
            [run_cmd], shell=True, cwd=self.cwd, stderr=log_file, stdout=log_file
        )
        return Ok()

    def run_pre_actions(self):
        for action in self.pre_actions:
            Log.info(f"Run pre action {action} for {self.name}")
            result = run_command_with_error_log(
                action, cwd=self.cwd, silent=self.silent,
                error_msg=f"Run pre action ({action}) failed"
            )
            if result.returncode != 0:
                return Err(
                    Constants.CALL_COMMAND_ERR, f"Run pre action ({action}) failed"
                )
        return Ok()

    def get_summary(self):
        summary = Summary()
        summary.insert("name", self.name)
        if self.start_time is None:
            self.start_time = self.end_time
        state = "success"
        if self.has_error():
            state = "failure"
        if self.is_aborted:
            state = "aborted"
        if self.is_timeout:
            state = "timeout"
        if self.end_time is None or self.start_time is None:
            summary.insert("costTime", "0")
            state = "unknown"
        else:
            summary.insert(
                "costTime", f"{int((self.end_time - self.start_time) * 1000)}"
            )
        summary.insert("state", state)
        return summary
