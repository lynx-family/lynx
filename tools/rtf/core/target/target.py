# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os.path
import subprocess
from core.env.env import RTFEnv
from core.utils.log import Log


class Target:
    def __init__(self, params, name):
        if "owners" not in params or len(params["owners"]) == 0:
            Log.fatal(f'You must add owners for {name}. eg `owners:["a",...]`')
        self.owners = params["owners"]
        self.params = params
        self.name = name
        self.target_path = None
        self.coverage_data_path = None
        self.enable = self.params["enable"] if "enable" in self.params else True
        self.start_time = None
        self.process = None
        self.global_info = {}
        self.log_file = os.path.join(
            RTFEnv.get_env("project_root_path"), f"{self.name}.log"
        )
        self.retry = 0
        self.retry_count = 0
        self.coverage = self.params["coverage"] if "coverage" in self.params else True
        self.build_tasks = []
        self.pre_actions = (
            self.params["pre_actions"] if "pre_actions" in self.params else []
        )
        self.init_self_info()

    # This method is used for a subclass to initialize its own private members
    # or override the base class member variables.
    def init_self_info(self):
        pass

    def insert_global_info(self, key: str, value):
        self.global_info[key] = value

    def get_coverage_raw_data(self):
        pass

    def kill(self):
        if self.process:
            self.process.terminate()

    def is_end(self):
        if self.process is None:
            return True
        if self.process.poll() is not None:
            return True
        return False

    def wait(self):
        if self.process:
            self.process.wait()

    def has_crash(self):
        if self.has_error():
            if (
                self.process.returncode == -11  # SIGSEGV - Segmentation Fault
                or self.process.returncode == -6  # SIGABRT - Abort
                or self.process.returncode == -4  # SIGILL - Illegal Instruction
                # TODO(zhaosong.lmm):
                # Currently unclear why this test receives a SIGTRAP signal in the case of a null pointer.
                or self.process.returncode == -5  # SIGTRAP
            ):
                return True
        return False

    def has_error(self):
        if self.process is None:
            return False
        if self.process.poll() is None:
            return False
        if self.process.returncode == 0:
            return False
        return True

    def print_log(self):
        log_file = open(self.log_file, "r")
        Log.error(f"{self.name} log: \n{log_file.read()}")

    def run(self):
        pass

    def run_pre_actions(self):
        pass
