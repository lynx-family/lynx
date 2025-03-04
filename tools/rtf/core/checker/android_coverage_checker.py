# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
from core.checker.coverage_checker import CoverageChecker
from core.utils.git_helper import GitHelper
from core.utils.log import Log
from core.utils.operation import Intersection, Difference
from core.utils.xml_reader import XmlReader


class AndroidCoverageChecker(CoverageChecker):
    def __init__(self):
        super().__init__()

    def __get_file_to_changed_map(self, count):
        changed_files = GitHelper.GetChangedFiles(count)
        need_check_file = [
            file
            for file in changed_files
            if file.endswith(".java") or file.endswith(".kt")
        ]
        file_2_changed_lines = {}
        for file in need_check_file:
            file_name = file.split("/")[-1]
            try:
                changed_lines = GitHelper.GetFileChangedLines(file, count)
                file_2_changed_lines[file_name] = {
                    "changed_lines": changed_lines,
                    "raw_path": file,
                }
            except Exception as e:
                Log.error(
                    f"Get changed lines for {file} failed. The file may have been removed or renamed."
                )
        return file_2_changed_lines

    def __get_uncovered_lines(self, source_file):
        uncovered_lines = []
        covered_lines = []
        lines = []
        for line in source_file:
            if line.tag != "line":
                continue
            line_number = int(line.attrib["nr"])
            lines.append(line_number)
            # <line nr="31" mi="0" ci="2" mb="0" cb="0"/>
            if line.attrib["ci"] != "0":
                covered_lines.append(line_number)
            else:
                uncovered_lines.append(line_number)
        return uncovered_lines, covered_lines, lines

    def check(self, args):
        self.job = args.job
        inputs = args.inputs
        if len(inputs) != 1:
            Log.fatal(f"please support coverage.xml file by --inputs!")
        file_2_changed_lines = self.__get_file_to_changed_map(args.count)
        root = XmlReader(inputs[0])
        result = []
        cur_all_need_covered_lines = 0
        cur_all_has_covered_lines = 0
        for child in root:
            if child.tag != "package":
                continue
            package_name = child.attrib["name"]
            for package_child in child:
                if package_child.tag != "sourcefile":
                    continue
                source_file_name = package_child.attrib["name"]
                if source_file_name in file_2_changed_lines.keys():
                    all_uncovered_lines, all_covered_lines, all_lines = (
                        self.__get_uncovered_lines(package_child)
                    )
                    # cur_xxx means the changes in this commit
                    cur_uncovered_lines = Intersection(
                        file_2_changed_lines[source_file_name]["changed_lines"],
                        all_uncovered_lines,
                    )
                    cur_need_covered_lines = Intersection(
                        file_2_changed_lines[source_file_name]["changed_lines"],
                        all_lines,
                    )
                    cur_has_covered_lines = Difference(
                        cur_need_covered_lines, cur_uncovered_lines
                    )
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
                                len(all_covered_lines) / len(all_lines)
                                if len(all_lines) != 0
                                else 1
                            ),
                            "source_file": source_file_name,
                            "raw_path": file_2_changed_lines[source_file_name][
                                "raw_path"
                            ],
                            "package_name": package_name,
                            "changed_lines": file_2_changed_lines[source_file_name][
                                "changed_lines"
                            ],
                        }
                    )
        cur_all_coverage_rate = (
            cur_all_has_covered_lines / cur_all_need_covered_lines
            if cur_all_need_covered_lines != 0
            else 1
        )

        self.summary(result, cur_all_coverage_rate, args.threshold)
