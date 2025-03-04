# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

from core.checker.native_coverage_checker import NativeCoverageChecker


class iOSCoverageChecker(NativeCoverageChecker):
    def __init__(self):
        super().__init__()

    def is_target_file(self, file_name):
        if (
            file_name.endswith(".m")
            or file_name.endswith(".h")
            or file_name.endswith(".mm")
            or file_name.endswith(".swift")
        ):
            return True
        else:
            return False
