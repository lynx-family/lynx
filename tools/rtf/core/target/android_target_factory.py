# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
from core.target.android_ut_target import (
    AndroidUTTarget,
    AndroidTargetType,
    AndroidUTApplicationTarget,
)


def AndroidTargetFactory(params, name: str):
    android_target_type = params["type"] if "type" in params else "library"
    if android_target_type == AndroidTargetType.LIBRARY.value:
        return AndroidUTTarget(params, name)
    elif android_target_type == AndroidTargetType.APPLICATION.value:
        return AndroidUTApplicationTarget(params, name)
    else:
        return None
