# -*- coding: UTF-8 -*-
# Copyright 2022 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
"""
@author: wangjianliang
"""
import os
import sys

from lynx_e2e.api.config import settings
sys.path.append(settings.PROJECT_ROOT)

from lib.test_runner.case_set import CaseSet

from lib.test_runner.test_runner import TestRunner
from lib.test_runner.plugins.devtool_connect_plugin import DevtoolConnectPlugin


def run(test):
    """
    Run command:
    android: python3 manage.py runtest androidlynxtest.case_sets.core
    ios: python3 manage.py runtest ioslynxtest.case_sets.core
    """
    runner = TestRunner(test)
    runner.add_plugin(DevtoolConnectPlugin(test))
    
    runner.add_case(CaseSet(case_set_path=os.path.dirname(__file__)))
    runner.run_test()
