# -*- coding: utf8 -*-
# Copyright 2021 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.
import time

from lynx_e2e.api.app import LynxApp as LynxAppBase

class LynxApp(LynxAppBase):
    app_spec = {
        "package_name": "com.lynx.explorer",  # app package name
        "init_device": True,  # whether to wake up device
        "process_name": "",  # main process name of app
        "start_activity": ".LynxViewShellActivity",  # leave it empty to be detected automatically
        "grant_all_permissions": True,  # grant all permissions before starting app
        "clear_data": False,  # pm clear app data
        "kill_process": True  # whether to kill previously started app
    }
    popup_rules = []

    def __init__(self, *args, **kwargs):
        kwargs['app_spec'] = self.app_spec
        super(LynxApp, self).__init__(*args, **kwargs)

    def open_card(self, url):
        if url == '':
            return
        self.open_lynx_container(url)
