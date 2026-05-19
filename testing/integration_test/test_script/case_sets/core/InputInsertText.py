# -*- coding: UTF-8 -*-
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import json

from lynx_e2e.api.lynx_view import LynxView

INPUT_A_TEXT = "first"
INPUT_B_TEXT = "second"
INPUT_C_TEXT = "value 123!?"
IGNORED_TEXT = "ignored before focus"

config = {
    "type": "custom",
    "path": "automation/input_insert_text/main",
    "platform": ["android", "ios"],
}


def send_cdp(test, session_id, method, params=None):
    request_id = test.app.send_cdp_data(
        session_id=session_id,
        method=method,
        params=params or {},
    )
    raw_response = test.app.wait_for_cdp_id(request_id, raw=True)
    response = json.loads(raw_response["message"])
    if "error" in response:
        raise RuntimeError("%s failed: %s" % (method, response["error"]))
    return response.get("result", {})


def assert_text(lynxview, tag, expected, message):
    actual = lynxview.get_by_test_tag(tag).text
    if actual != expected:
        raise AssertionError("%s Expected %r, got %r." % (message, expected, actual))


def run(test):
    lynxview = test.app.get_lynxview("lynxview", LynxView)
    input_a = lynxview.get_by_test_tag("insert-text-input-a")
    input_b = lynxview.get_by_test_tag("insert-text-input-b")
    input_c = lynxview.get_by_test_tag("insert-text-input-c")
    value_a = lynxview.get_by_test_tag("insert-text-value-a")
    value_b = lynxview.get_by_test_tag("insert-text-value-b")
    value_c = lynxview.get_by_test_tag("insert-text-value-c")
    input_count = lynxview.get_by_test_tag("insert-text-input-count")
    last_input = lynxview.get_by_test_tag("insert-text-last-input")

    test.assert_existing(input_a, "Input A element is not existing!")
    test.assert_existing(input_b, "Input B element is not existing!")
    test.assert_existing(input_c, "Input C element is not existing!")
    test.assert_existing(value_a, "Input A value element is not existing!")
    test.assert_existing(value_b, "Input B value element is not existing!")
    test.assert_existing(value_c, "Input C value element is not existing!")
    test.assert_existing(input_count, "Input count element is not existing!")
    test.assert_existing(last_input, "Last input element is not existing!")

    session_id = lynxview.get_session_id()

    test.start_step("--------Test1: Input.insertText without focused input is ignored;-------")
    send_cdp(test, session_id, "Input.insertText", {"text": IGNORED_TEXT})
    assert_text(
        lynxview,
        "insert-text-value-a",
        "",
        "Input A should stay empty before focus!",
    )
    assert_text(
        lynxview,
        "insert-text-value-b",
        "",
        "Input B should stay empty before focus!",
    )
    assert_text(
        lynxview,
        "insert-text-value-c",
        "",
        "Input C should stay empty before focus!",
    )
    assert_text(
        lynxview,
        "insert-text-input-count",
        "0",
        "Input event count should stay zero before focus!",
    )
    assert_text(
        lynxview,
        "insert-text-last-input",
        "none",
        "Last input should stay none before focus!",
    )

    test.start_step("--------Test2: Insert text into focused input A;-------")
    send_cdp(test, session_id, "DOM.focus", {"nodeId": input_a.id})
    send_cdp(test, session_id, "Input.insertText", {"text": INPUT_A_TEXT})
    test.wait_for_equal(
        "Input.insertText did not update input A!",
        value_a,
        "text",
        INPUT_A_TEXT,
    )
    test.wait_for_equal(
        "Input A input event was not recorded!",
        input_count,
        "text",
        "1",
    )
    test.wait_for_equal(
        "Input A was not recorded as the last input target!",
        last_input,
        "text",
        "input-a",
    )
    assert_text(
        lynxview,
        "insert-text-value-b",
        "",
        "Input B should stay empty after inserting into input A!",
    )
    assert_text(
        lynxview,
        "insert-text-value-c",
        "",
        "Input C should stay empty after inserting into input A!",
    )

    test.start_step("--------Test3: Switch focus and insert text into input B;-------")
    send_cdp(test, session_id, "DOM.focus", {"nodeId": input_b.id})
    send_cdp(test, session_id, "Input.insertText", {"text": INPUT_B_TEXT})
    test.wait_for_equal(
        "Input.insertText did not update input B!",
        value_b,
        "text",
        INPUT_B_TEXT,
    )
    test.wait_for_equal(
        "Input B input event was not recorded!",
        input_count,
        "text",
        "2",
    )
    test.wait_for_equal(
        "Input B was not recorded as the last input target!",
        last_input,
        "text",
        "input-b",
    )
    assert_text(
        lynxview,
        "insert-text-value-a",
        INPUT_A_TEXT,
        "Input A should keep its value after inserting into input B!",
    )
    assert_text(
        lynxview,
        "insert-text-value-c",
        "",
        "Input C should stay empty after inserting into input B!",
    )

    test.start_step("--------Test4: Insert spaces, punctuation, and digits into input C;-------")
    send_cdp(test, session_id, "DOM.focus", {"nodeId": input_c.id})
    send_cdp(test, session_id, "Input.insertText", {"text": INPUT_C_TEXT})
    test.wait_for_equal(
        "Input.insertText did not preserve spaces, punctuation, and digits!",
        value_c,
        "text",
        INPUT_C_TEXT,
    )
    test.wait_for_equal(
        "Input C input event was not recorded!",
        input_count,
        "text",
        "3",
    )
    test.wait_for_equal(
        "Input C was not recorded as the last input target!",
        last_input,
        "text",
        "input-c",
    )
