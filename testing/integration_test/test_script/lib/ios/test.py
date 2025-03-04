# -*- coding: utf8 -*-
# Copyright 2021 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

from lynx_e2e.api.testcase import TestCase

from .app import LynxApp
from ..common.utils import wait_for_equal
from ..test_runner.mixin.img_diff_mixin import ImageConfig, ImageDiffMixin
from ..test_runner.mixin.report_mixin import LogCache, LogRecordMixin
from ..test_runner.mixin.result_mixin import ResultMixin

class LynxTest(LogRecordMixin, ImageDiffMixin, ResultMixin, TestCase):
    """lynx test case base
    """
    app = {}

    platform = 'ios'

    def __init__(self):
        LogRecordMixin.__init__(self)
        ImageDiffMixin.__init__(self)
        ResultMixin.__init__(self)
        TestCase.__init__(self)

    def pre_test(self):
        self.start_step('--------Device Init--------')
        self.device = self.acquire_device()
        app = LynxApp(self.device)
        self.app = app
        self.log_cache = LogCache()
        super().pre_test()

    def crop_image_in_lynxview(self, device, view_rect, name, out_dir=""):
        from lynx_e2e.api.lynx_view import LynxView

        lynxview = self.app.get_lynxview('lynxview', LynxView)
        lynxview.screenshot(name)
        lynx_rect = lynxview.rect
        img_config = ImageConfig(
            x_min=view_rect.left - lynx_rect.left, x_max=view_rect.right - lynx_rect.left,
            y_min=view_rect.top - lynx_rect.top, y_max=view_rect.bottom - lynx_rect.top,
            resize_width=view_rect.width, resize_height=view_rect.height, out_dir=out_dir, width_scale=3, height_scale=3
        )
        self.add_img_config(name, img_config)

    def assert_existing(self, element, message='not exist', timeout=3):
        if isinstance(element, list):
            j = 0
            for ele in element:
                try:
                    ele.wait_for_existing(timeout)
                except Exception:
                    pass
                else:
                    j += 1
            if j == 0:
                raise Exception(message)
        else:
            try:
                element.wait_for_existing(timeout)
            except Exception:
                raise Exception(message)

    def assert_not_existing(self, element, message='exist'):
        try:
            if not element.existing:
                return True
        except Exception:
            raise Exception(message)

    def assert_visible(self, element, message='invisible'):
        try:
            element.wait_for_visible(5)
        except Exception:
            raise Exception(message)

    def assert_invisible(self, element, message='visible'):
        try:
            element.wait_for_invisible(5)
        except Exception:
            raise Exception(message)

    def assert_equal(self, element1, element2):
        if element1 == element2:
            return True
        else:
            raise Exception('{} is not equal with {}'.format(element1, element2))

    def assert_not_equal(self, element1, element2):
        if element1 != element2:
            return True
        else:
            raise Exception('{} is not equal with {}'.format(element1, element2))

    def wait_for_equal(self, message, obj, prop_name, expected, timeout=10):
        wait_for_equal(self, message, obj, prop_name, expected, timeout)

