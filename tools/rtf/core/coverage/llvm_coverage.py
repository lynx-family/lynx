# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import json
import os
import subprocess

from core.coverage.coverage import Coverage
from core.env.env import RTFEnv
from core.target.target import Target
from core.utils.log import Log


class LLVMCoverage(Coverage):
    def __init__(self, ignores: [str], output: str):
        super().__init__()
        self.ignores = ignores
        self.output = output
        self.profdata_name = "rtf-ut.profdata"

    def __get_profraw_str(self, targets: [Target]):
        profraw_str = ""
        for target in targets:
            if target.coverage:
                profraw_str = (
                    f"{' '.join(target.get_coverage_raw_data())} {profraw_str}"
                )
        profraw_str = profraw_str.strip()
        return profraw_str

    def __get_binary_target_str(self, targets: [Target]):
        exe_str = ""
        for target in targets:
            if target.coverage:
                exe_str = f"-object={target.target_path} {exe_str}"
        return exe_str

    def __get_ignored_str(self):
        ignore_str = ""
        for ignore_ in self.ignores:
            ignore_str = f"-ignore-filename-regex={ignore_} {ignore_str}"
        return ignore_str

    def __gen_html_report(self, targets: [Target]):
        profraw_str = self.__get_profraw_str(targets)
        if profraw_str == "":
            return
        llvm_profdata_cmd = (
            f"llvm-profdata merge -sparse {profraw_str} -o {self.profdata_name}"
        )
        subprocess.check_call(llvm_profdata_cmd, shell=True)
        html_report_path = os.path.join(
            RTFEnv.get_env("project_root_path"), self.output, "html"
        )
        if not os.path.exists(html_report_path):
            os.makedirs(html_report_path)
        binary_file = self.__get_binary_target_str(targets)
        if binary_file == "":
            return
        llvm_conv_cmd = f"llvm-cov show -output-dir={html_report_path} \
                            -format=html \
                            -instr-profile={self.profdata_name} \
                            -show-line-counts \
                            -show-instantiations \
                            -show-regions \
                            -show-line-counts-or-regions {binary_file} \
                            {self.__get_ignored_str()}"
        subprocess.check_call(llvm_conv_cmd, shell=True)

    def __gen_json_summary(self, targets: [Target]):
        json_report_path = os.path.join(
            RTFEnv.get_project_root_path(), self.output, "json"
        )
        if not os.path.exists(json_report_path):
            os.makedirs(json_report_path)
        binary_file = self.__get_binary_target_str(targets)
        if binary_file == "":
            return
        llvm_conv_cmd = f"llvm-cov export -format=text \
                    -summary-only\
                    -instr-profile={self.profdata_name} \
                    {self.__get_ignored_str()}    \
                    {binary_file} > {json_report_path}/coverage_summary.json"
        subprocess.check_call(llvm_conv_cmd, shell=True)
        with open(f"{json_report_path}/coverage_summary.json", "r") as rf:
            data = json.load(rf)
        with open(f"{json_report_path}/coverage_summary.json", "w") as wf:
            data_str = json.dumps(data)
            wf.write(data_str.replace(RTFEnv.get_project_root_path(), ""))
        with open(f"{json_report_path}/coverage_summary_total.json", "w") as wf:
            json.dump(data["data"][0]["totals"], wf)

    def __gen_lcov_summary(self, targets: [Target]):
        lcov_report_path = os.path.join(
            RTFEnv.get_project_root_path(), self.output, "lcov"
        )
        if not os.path.exists(lcov_report_path):
            os.makedirs(lcov_report_path)
        binary_file = self.__get_binary_target_str(targets)
        if binary_file == "":
            return
        llvm_conv_cmd = f"llvm-cov export -format=lcov \
                            -instr-profile={self.profdata_name} \
                            {self.__get_ignored_str()}    \
                            {binary_file} > {lcov_report_path}/lcov_summary.txt"
        subprocess.check_call(llvm_conv_cmd, shell=True)
        with open(f"{lcov_report_path}/lcov_summary.txt", "r") as rf:
            data = rf.read()
        with open(f"{lcov_report_path}/lcov_summary.txt", "w") as wf:
            wf.write(data.replace(RTFEnv.get_project_root_path(), ""))

    def gen_report(self, targets: [Target]):
        Log.info(f"Generate coverage to {self.output}")
        try:
            self.__gen_html_report(targets)
            self.__gen_json_summary(targets)
            self.__gen_lcov_summary(targets)
        except Exception as e:
            Log.fatal(f"Generate coverage report failed! {e}")
