# -*- coding: UTF-8 -*-
# Copyright 2021 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import time
from lynx_e2e.api.lynx_view import LynxView

from .testcase import Testcase


class TestcaseCheckImage(Testcase):
    def __init__(self, name, url, attributes):
        super(TestcaseCheckImage, self).__init__(name, url, attributes)

    def run(self, test, platform):
        if 'sleep' in self.attr:
            test.log_record(f"sleep before exec: {self.attr['sleep']}")
            time.sleep(self.attr['sleep'])
        else:
            time.sleep(2)
        lynxview = test.app.get_lynxview('lynxview', LynxView)
        image_name = f"{self.image_prefix}{self.attr['image']}"
        lynxview.screenshot(image_name)
        test.diff_img(app=test.app, name=image_name)
