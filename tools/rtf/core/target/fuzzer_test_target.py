# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os.path
import subprocess
from datetime import datetime
from core.env.env import RTFEnv
from core.target.native_ut_target import NativeUTTarget
from core.utils.log import Log


class FuzzerTestTarget(NativeUTTarget):
    def __init__(self, params, name):
        super().__init__(params, name)

    def init_self_info(self):
        super().init_self_info()
        assert "config" in self.params, "Error: fuzzer target miss params `config`"
        config = self.params["config"]
        self.rss_limit_md = config["rss_limit_md"] if "rss_limit_md" in config else 100
        self.max_total_time = (
            config["max_total_time"] if "max_total_time" in config else 10
        )
        self.corpus_path = (
            config["corpus"]
            if "corpus" in config
            else os.path.join(
                RTFEnv.get_project_root_path(),
                f"fuzzer_{self.name}_corpus_{datetime.datetime.now().timestamp()}",
            )
        )
        if not os.path.exists(self.corpus_path):
            Log.warning(f"Corpus not exists! will create tmp dir {self.corpus_path}")
            os.makedirs(self.corpus_path)
        self.artifacts = (
            config["artifact"]
            if "artifact" in config
            else os.path.join(
                RTFEnv.get_project_root_path(),
                f"fuzzer_{self.name}_artifact_{datetime.datetime.now().timestamp()}",
            )
        )
        if not os.path.exists(self.artifacts):
            Log.warning(f"Artifacts not exists! will create tmp dir {self.artifacts}")
            os.makedirs(self.artifacts)

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
        return f'LLVM_PROFILE_FILE="{self.coverage_data_path}" {self.target_path} {self.corpus_path} -rss_limit_mb={self.rss_limit_md} -artifact_prefix={self.artifacts} -max_total_time={self.max_total_time}'
