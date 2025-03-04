# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
from core.checker.coverage_checker import CoverageChecker
from core.utils.git_helper import GitHelper
from core.utils.lcov_reader import LCovReader
from core.utils.log import Log
from core.utils.operation import Intersection


class NativeCoverageChecker(CoverageChecker):
    def __init__(self):
        super().__init__()

    def is_target_file(self, file_name):
        if (
            file_name.endswith(".cc")
            or file_name.endswith(".h")
            or file_name.endswith(".cpp")
        ):
            return True
        else:
            return False

    def __get_file_to_changed_map(self, count):
        changed_files = GitHelper.GetChangedFiles(count)
        need_check_file = [file for file in changed_files if self.is_target_file(file)]
        file_2_changed_lines = {}
        for file in need_check_file:
            file_name = file.split("/")[-1]
            try:
                changed_lines = GitHelper.GetFileChangedLines(file, count)
                file_2_changed_lines[file_name] = {
                    "changed_lines": changed_lines,
                    "raw_path": "/" + file,
                }
            except Exception as e:
                Log.error(
                    f"Get changed lines for {file} failed. The file may have been removed or renamed."
                )
        return file_2_changed_lines

    def __get_uncovered_lines(self, content, file_name):
        meta_data = content[file_name]["DA"]
        uncovered_lines = []
        covered_lines = []
        lines = []
        for da in meta_data:
            data = da.split(",")
            line = int(data[0])
            if int(data[1]) == 0:
                uncovered_lines.append(line)
            else:
                covered_lines.append(line)
            lines.append(line)
        return uncovered_lines, covered_lines, lines

    def is_need_check_file(self, file_name, file_list):
        for file in file_list:
            if file_name in file:
                return True, file
        return False, -1

    def check(self, args):
        self.job = args.job
        if len(args.inputs) != 1:
            Log.fatal(f"please support lcov.txt file by --inputs!")
        content = LCovReader(args.inputs[0])
        file_2_change_lines = self.__get_file_to_changed_map(args.count)
        # ignore some files which not in content
        need_check_files = {}
        for file in file_2_change_lines.keys():
            file_name = file_2_change_lines[file]["raw_path"]
            need_check, file_path_in_lcov_file = self.is_need_check_file(
                file_name, content.keys()
            )
            if need_check:
                need_check_files[file] = file_2_change_lines[file]
                need_check_files[file]["lcov_path"] = file_path_in_lcov_file

        result = []
        cur_all_need_covered_lines = 0
        cur_all_has_covered_lines = 0
        for file in need_check_files.keys():
            file_name = need_check_files[file]["raw_path"]
            lcov_file_name = need_check_files[file]["lcov_path"]
            changed_lines = need_check_files[file]["changed_lines"]
            uncovered_lines, covered_lines, lines = self.__get_uncovered_lines(
                content, lcov_file_name
            )
            cur_uncovered_lines = Intersection(changed_lines, uncovered_lines)
            cur_need_covered_lines = Intersection(changed_lines, lines)
            cur_has_covered_lines = Intersection(changed_lines, covered_lines)
            cur_all_need_covered_lines += len(cur_need_covered_lines)
            cur_all_has_covered_lines += len(cur_has_covered_lines)
            result.append(
                {
                    "cur_uncovered_lines": cur_uncovered_lines,
                    "cur_need_covered_lines": cur_need_covered_lines,
                    "cur_has_covered_lines": cur_has_covered_lines,
                    "cur_file_coverage_rate": (
                        len(cur_has_covered_lines) / len(cur_need_covered_lines)
                        if len(cur_need_covered_lines) != 0
                        else 1
                    ),
                    "coverage_rate": (
                        len(covered_lines) / len(lines) if len(lines) != 0 else 1
                    ),
                    "source_file": file,
                    "raw_path": file_name,
                    "changed_lines": changed_lines,
                }
            )
        cur_all_coverage_rate = (
            cur_all_has_covered_lines / cur_all_need_covered_lines
            if cur_all_need_covered_lines != 0
            else 1
        )
        self.summary(result, cur_all_coverage_rate, args.threshold)
