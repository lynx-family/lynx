# -*- coding: UTF-8 -*-
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import os
import sys

search_path = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.append(search_path)

from case_sets.xelement.runner import run
from lib.ios.test import LynxTest


class xelementTest(LynxTest):
    timeout = 1800

    def run_test(self, test=None):
        if test is None:
            test = self
        test.start_step("--------Test: start to test xelement;-------")
        run(test=test)


if __name__ == "__main__":
    xelementTest.debug_run()
