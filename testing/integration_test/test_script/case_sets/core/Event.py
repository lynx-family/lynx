# -*- coding: UTF-8 -*-
# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import time
from lynx_e2e.api.lynx_view import LynxView

from lib.common import utils

config = {
    "type": "custom",
    "path": "automation/event"
}


def run(test):
    lynxview = test.app.get_lynxview('lynxview', LynxView)
    count_view = lynxview.get_by_test_tag('count')
    test.start_step('--------Test1: Test view click;-------')
    lynxview.get_by_test_tag('button0').click()
    test.wait_for_equal('View click failed!', count_view, 'text', '1')
    time.sleep(5)

    test.start_step('--------Test2: Test text click;-------')
    lynxview.get_by_test_tag('button-text1').click()
    test.wait_for_equal('Text click failed!', count_view, 'text', '2')
    time.sleep(5)

    test.start_step('--------Test3: Test inline-text click;-----')
    button2 = lynxview.get_by_test_tag("button2")
    rect = button2.rect
    button2.click(offset_x=rect.width / 2 - 20, offset_y=0)
    time.sleep(1)
    utils.take_screenshot_check(test, "event_inline_text_", 1, rect)
