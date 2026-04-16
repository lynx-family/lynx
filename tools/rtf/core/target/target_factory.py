# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
from core.target.android_target_factory import AndroidTargetFactory
from core.target.fuzzer_test_target import FuzzerTestTarget
from core.target.native_ut_target import NativeUTTarget
from core.base.result import Ok, Err
from core.base.constants import Constants


def TargetFactory(target_type: str, params, name: str, gtest_filter: str = None, silent: bool = False):
    target = None
    if target_type == "native-ut":
        target = NativeUTTarget(params, name, gtest_filter, silent)
    elif target_type == "android-ut":
        target = AndroidTargetFactory(params, name)
    elif target_type == "fuzzer-test":
        target = FuzzerTestTarget(params, name, silent)
    if target is None:
        return Err(Constants.TARGET_BUILD_ERR, f"Unsupport target_type :{target_type}")
    return Ok(target)
