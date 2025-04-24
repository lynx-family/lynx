# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

builder = {
    "default": {
        "args": [
            "--parallel",
            "--configure-on-demand",
            "-x lint",
            "-PabiList=x86",
            "-Penable_lynx_android_test=true",
            "-Penable_coverage=true",
        ],
        "workspace": "platform/android",
        "type": "gradle",
    }
}

coverage = {
    "output": "out/coverage",
    "source_files": [
        "lynx_android/src/main/java",
    ],
    "type": "jacoco",
}

targets = {
    "LynxAndroid": {"task": "LynxAndroid:assembleNoasanDebugAndroidTest"},
}
