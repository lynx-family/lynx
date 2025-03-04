# -*- coding: UTF-8 -*-
# Copyright 2021 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

from importlib import import_module

from .testcase import Testcase


class TestcaseCustom(Testcase):

    def run(self, test, _):
        module_name = f"case_sets.{self.attr['category']}.{self.name}"
        module = import_module(module_name)
        custom_run = getattr(module, 'run')
        custom_run(test)
