# -*- coding: UTF-8 -*-
# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import time
from lynx_e2e.api.lynx_view import LynxView

from lib.common import utils

config = {
    "type": "custom",
    "path": "showcase/text/text_event"
}


def run(test):
    time.sleep(3)
    lynxview = test.app.get_lynxview('lynxview', LynxView)
    test.start_step('--------Test1: Check flattern text screenshot;-------')
    flattern_text_element = lynxview.get_by_test_tag('flatten-text')
    utils.take_screenshot_check(test, "text_flattern_element", "", flattern_text_element.rect)

    test.start_step('--------Test2: Check text event;-------')
    text_bindlayout_element = lynxview.get_by_text('Test text bindlayout event....')
    test.assert_existing(text_bindlayout_element, 'Text bindlayout event element is not existing!')
