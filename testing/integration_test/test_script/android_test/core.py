# -*- coding: UTF-8 -*-
# Copyright 2022 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import sys
import os

search_path = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(search_path)
from lib.android.test import LynxTest
from case_sets.core.runner import run


class coreTest(LynxTest):
    """
    Run case: python3 manage.py runtest android_test.case_sets.core
    """
    timeout = 1800

    def run_test(self, test=None):
        if test is None:
            test = self
        test.start_step('--------Test: start to test core;-------')
        run(test=test)


if __name__ == '__main__':
   coreTest.debug_run()
