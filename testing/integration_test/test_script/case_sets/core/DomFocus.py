# -*- coding: UTF-8 -*-
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import json

from lynx_e2e.api.lynx_view import LynxView

config = {
    "type": "custom",
    "path": "automation/dom-focus/main",
}


def send_cdp(test, session_id, method, params=None):
    req_id = test.app.send_cdp_data(
        session_id=session_id,
        method=method,
        params=params or {},
    )
    raw_response = test.app.wait_for_cdp_id(req_id, raw=True)
    response = json.loads(raw_response["message"])
    if "error" in response:
        raise RuntimeError("%s failed: %s" % (method, response["error"]))
    return response.get("result", {})


def run(test):
    lynxview = test.app.get_lynxview("lynxview", LynxView)
    session_id = lynxview.get_session_id()

    focus_state = lynxview.get_by_test_tag("focus-state")
    blur_state = lynxview.get_by_test_tag("blur-state")
    input_a = lynxview.get_by_test_tag("focus-input-a")
    input_b = lynxview.get_by_test_tag("focus-input-b")

    test.start_step("--------Test1: Focus input A with DOM.focus;-------")
    send_cdp(test, session_id, "DOM.focus", {"nodeId": input_a.id})
    test.wait_for_equal(
        "DOM.focus did not focus input A!",
        focus_state,
        "text",
        "input-a",
    )

    test.start_step("--------Test2: Focus input B with DOM.focus;-------")
    send_cdp(test, session_id, "DOM.focus", {"nodeId": input_b.id})
    test.wait_for_equal(
        "DOM.focus did not focus input B!",
        focus_state,
        "text",
        "input-b",
    )
    test.wait_for_equal(
        "DOM.focus did not blur input A!",
        blur_state,
        "text",
        "input-a",
    )
