# -*- coding: UTF-8 -*-
# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

from lynx_e2e.api.lynx_view import LynxView

from lib.common import utils

config = {
    "type": "custom",
    "path": "showcase/list/base"
}


def run(test):
    lynxview = test.app.get_lynxview('lynxview', LynxView)
    test.start_step('--------Test1: Screenshot and image compare;-------')
    utils.take_screenshot_check(test, "list_base", "", lynxview.rect)
