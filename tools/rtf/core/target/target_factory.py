# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
from core.target.android_target_factory import AndroidTargetFactory
from core.target.fuzzer_test_target import FuzzerTestTarget
from core.target.native_ut_target import NativeUTTarget


def TargetFactory(target_type: str, params, name: str):
    if target_type == "native-ut":
        return NativeUTTarget(params, name)
    elif target_type == "android-ut":
        return AndroidTargetFactory(params, name)
    elif target_type == "fuzzer-test":
        return FuzzerTestTarget(params, name)
    else:
        return None
