# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
from core.checker.android_coverage_checker import AndroidCoverageChecker
from core.checker.ios_coverage_checker import iOSCoverageChecker
from core.checker.native_coverage_checker import NativeCoverageChecker


def CheckerFactory(check_type: str):
    if check_type == "android":
        return AndroidCoverageChecker()
    elif check_type == "ios":
        return iOSCoverageChecker()
    elif check_type == "cpp":
        return NativeCoverageChecker()
    else:
        return None
