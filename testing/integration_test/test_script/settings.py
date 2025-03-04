# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import os

platform = os.environ.get('platform')
PROJECT_NAME = "lynx"
PROJECT_ROOT = os.path.dirname(os.path.abspath(__file__))
E2E_DRIVER_PATH = "lynx_e2e._impl.appium"
TEST_TARGET = "lynx-e2e"

# For Appium
ANDROID_ADB_SERVER = ("127.0.0.1", 5037)
RUNNER_TASK_INFO = {}
