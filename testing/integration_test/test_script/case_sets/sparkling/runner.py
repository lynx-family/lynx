# -*- coding: UTF-8 -*-
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import os
import sys
import time

from lynx_e2e.api.config import settings

sys.path.append(settings.PROJECT_ROOT)

from lib.test_runner.case_set import CaseSet
from lib.test_runner.plugins.devtool_connect_plugin import DevtoolConnectPlugin
from lib.test_runner.test_runner import TestRunner


class SparklingTestRunner(TestRunner):
    """Restart Explorer without letting lynx-e2e destroy WebDriver.

    lynx-e2e 0.0.12 implements ``LynxDebugger.disconnect`` by closing the
    debugger socket *and* quitting the Appium driver.  The generic runner calls
    that method before every restart, so an iOS case set that deliberately
    starts each case from a clean homepage cannot reconnect.  Keep this
    workaround scoped to the Sparkling suite and let the generic final cleanup
    retain ownership of the WebDriver session.
    """

    @staticmethod
    def _disconnect_devtool_preserving_appium_session(app):
        # This reaches into lynx-e2e internals (``connector.usb_socket`` and
        # ``connector.device.device_driver.stop_forward``) because 0.0.12 exposes
        # no public "disconnect the debugger but keep the Appium session" API.
        # Every access is guarded with ``getattr`` and the block swallows
        # AttributeError/OSError so a lynx-e2e upgrade that renames or removes
        # these degrades to a no-op instead of crashing the whole suite. Replace
        # this shim once an upstream API exists (tracked as a follow-up).
        debugger = getattr(app, "lynxDebug", None)
        if debugger is None:
            return

        connector = getattr(debugger, "connector", None)
        try:
            if connector is None:
                return
            usb_socket = getattr(connector, "usb_socket", None)
            if usb_socket is not None:
                usb_socket.close()
            device = getattr(connector, "device", None)
            device_driver = getattr(device, "device_driver", None)
            sock_port = getattr(connector, "sock_port", None)
            stop_forward = getattr(device_driver, "stop_forward", None)
            if callable(stop_forward) and sock_port is not None:
                stop_forward(sock_port)
        except (AttributeError, OSError):
            # lynx-e2e internals moved (upgrade) or the socket is already gone;
            # the generic final cleanup still owns the WebDriver session.
            pass
        finally:
            app.lynxDebug = None

    def restart_and_connect_app(self):
        self._test.log_info("restart app!")
        self._disconnect_devtool_preserving_appium_session(self._test.app)
        self._test.app.restart()
        time.sleep(3)
        self._test.app.connect_app_to_lynx_server()


def run(test):
    """Run the iOS-only Sparkling Explorer routing suite."""
    runner = SparklingTestRunner(test)
    # Explorer creates its real homepage as part of every app restart. The
    # generic TestRunner would otherwise OpenCard the same homepage bundle a
    # second time, leaving two indistinguishable DevTool sessions for the E2E
    # resolver to correlate with the one displayed native LynxView.
    runner.set_open_card(False)
    runner.add_plugin(DevtoolConnectPlugin(test))
    runner.add_case(CaseSet(case_set_path=os.path.dirname(__file__)))
    runner.run_test()
