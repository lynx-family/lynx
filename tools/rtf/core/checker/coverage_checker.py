# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
from core.checker.check import Checker
from core.utils.log import RobotMessageLog


class CoverageChecker(Checker):
    def __init__(self):
        super().__init__()
        self.job = None

    def check(self, inputs):
        pass

    def summary(self, results, cur_all_coverage_rate, threshold):
        local_log = RobotMessageLog()
        local_log.info(f"Coverage Check for {self.job}")
        has_error = False
        if cur_all_coverage_rate < threshold:
            has_error = True
            local_log.error(
                f"Code coverage for this commit is {cur_all_coverage_rate * 100:.2f}%(<{threshold * 100}%)"
            )
        else:
            local_log.success(
                f"Code coverage for this commit is {cur_all_coverage_rate * 100:.2f}%"
            )
        for result in results:
            if result["cur_file_coverage_rate"] < threshold:
                local_log.warning(
                    f"In this change {result['source_file']}'s coverage({result['cur_file_coverage_rate'] * 100:.2f}%) "
                    f"does not meet the "
                    f"requirements(>={threshold * 100}%), lines ({sorted(result['cur_uncovered_lines'])}) are not "
                    f"covered!"
                )
            else:
                local_log.success(
                    f"In this change {result['source_file']}'s coverage({result['cur_file_coverage_rate'] * 100:.2f}%) check "
                    f"passed!"
                )
        if not has_error:
            local_log.success("Coverage check passed!")
            local_log.save()
        else:
            local_log.fatal("Coverage check failed!")
