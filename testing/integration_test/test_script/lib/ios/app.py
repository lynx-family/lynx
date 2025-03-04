# -*- coding: utf8 -*-
# Copyright 2021 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

from lynx_e2e.api.app import LynxApp as LynxAppBase

class LynxApp(LynxAppBase):
    app_spec = {
        "bundleId": "com.lynx.LynxExplorer",
    }
    popup_rules = []

    def __init__(self, *args, **kwargs):
        kwargs['app_spec'] = self.app_spec
        super(LynxApp, self).__init__(*args, **kwargs)

    def open_card(self, url):
        if url == '':
            return
        self.open_lynx_container(url)
