# -*- coding: UTF-8 -*-
import time

from .basic_plugin import ProcessPlugin


class DevtoolConnectPlugin(ProcessPlugin):
    def __init__(self, test, open_ui=True):
        super().__init__(test)
        self._open_ui = open_ui

    def pre_test(self):
        self.start_connect()
        # self._test.app.disconnect()
        # # Need to restart app after enable devtool, devtool will take effect on the second start
        # self._test.app.restart()
        # time.sleep(5)
        # self.start_connect()
        # After connecting to the devtool server, wait for 2 seconds to ensure the availability of the connection.
        time.sleep(2)

    def start_connect(self):
        self._test.app.connect_app_to_lynx_server()
    
    def pre_run(self, case):
        if not self._open_ui:
            self._test.log_info('open card with UI operation: ' + case.url)
            self._test.app.open_card_with_ui_operation(case.url)

    def post_run(self, _):
        if not self._open_ui:
            self._test.device.back()
            