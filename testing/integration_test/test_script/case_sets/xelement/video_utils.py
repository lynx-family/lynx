# -*- coding: UTF-8 -*-
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import json
import os
import re
import time

from lynx_e2e.api.config import settings
from lynx_e2e.api.logger import EnumLogLevel
from lynx_e2e.api.lynx_view import LynxView


def get_lynxview(test):
    return test.app.get_lynxview("lynxview", LynxView)


def refresh_document_tree(lynxview):
    driver = lynxview.get_lynx_driver()
    if hasattr(driver, "_document_tree"):
        driver._document_tree = None


def get_text(lynxview, tag):
    refresh_document_tree(lynxview)
    element = lynxview.get_by_test_tag(tag)
    if hasattr(element, "text"):
        return element.text
    return element.get_attribute("text")


def click(lynxview, tag):
    if os.environ.get("platform") == "ios" and _invoke_ios_action(
            lynxview, tag):
        return
    lynxview.get_by_test_tag(tag).click()


def _invoke_ios_action(lynxview, tag):
    driver = lynxview.get_lynx_driver()
    expression = (
        "typeof globalThis.__videoE2EAction === 'function' && "
        "globalThis.__videoE2EAction(%s)" % json.dumps(tag)
    )
    try:
        result = driver._debugger.eval(expression, driver.get_session_id())
    except Exception as e:
        if hasattr(lynxview, "log"):
            lynxview.log(
                EnumLogLevel.WARNING,
                "iOS action eval failed: tag=%s, expression=%s, error=%s" %
                (tag, expression, e),
            )
        return False
    return result.get("result", {}).get("value") is True


def wait_for_text(test, lynxview, tag, expected, timeout=20):
    return wait_until(
        test,
        lynxview,
        tag,
        lambda text: text == expected,
        "%s should be %s" % (tag, expected),
        timeout=timeout,
    )


def wait_until(test, lynxview, tag, predicate, message, timeout=20):
    deadline = time.time() + timeout
    last_text = ""
    while time.time() < deadline:
        last_text = get_text(lynxview, tag)
        if predicate(last_text):
            return last_text
        time.sleep(0.2)
    capture_screenshot(test, lynxview, "wait_failure")
    last_text = get_text(lynxview, tag)
    if predicate(last_text):
        return last_text
    raise AssertionError("%s. Last %s text: %s" % (message, tag, last_text))


def wait_for_contains(test, lynxview, tag, expected, timeout=20):
    return wait_until(
        test,
        lynxview,
        tag,
        lambda text: expected in text,
        "%s should contain %s" % (tag, expected),
        timeout=timeout,
    )


def assert_text_contains(lynxview, tag, expected):
    text = get_text(lynxview, tag)
    if expected not in text:
        raise AssertionError("%s should contain %s, got %s" %
                             (tag, expected, text))


def assert_text_not_contains(lynxview, tag, unexpected):
    text = get_text(lynxview, tag)
    if unexpected in text:
        raise AssertionError("%s should not contain %s, got %s" %
                             (tag, unexpected, text))


def assert_text_occurrences(lynxview, tag, expected, count):
    text = get_text(lynxview, tag)
    actual = text.count(expected)
    if actual != count:
        raise AssertionError(
            "%s should contain %s exactly %s times, got %s in %s" %
            (tag, expected, count, actual, text))


def assert_text_order(lynxview, tag, expected_entries):
    text = get_text(lynxview, tag)
    cursor = -1
    for entry in expected_entries:
        next_index = text.find(entry, cursor + 1)
        if next_index < 0:
            raise AssertionError("%s missing %s after index %s: %s" %
                                 (tag, entry, cursor, text))
        cursor = next_index


def wait_for_count_at_least(test, lynxview, key, expected, timeout=10):
    return wait_until(
        test,
        lynxview,
        "event-counts",
        lambda text: parse_count(text, key) >= expected,
        "%s count should be at least %s" % (key, expected),
        timeout=timeout,
    )


def assert_count_equals(lynxview, key, expected):
    text = get_text(lynxview, "event-counts")
    actual = parse_count(text, key)
    if actual != expected:
        raise AssertionError("%s count expected %s, got %s in %s" %
                             (key, expected, actual, text))


def assert_count_at_most(lynxview, key, expected):
    text = get_text(lynxview, "event-counts")
    actual = parse_count(text, key)
    if actual > expected:
        raise AssertionError("%s count expected at most %s, got %s in %s" %
                             (key, expected, actual, text))


def parse_count(counts_text, key):
    match = re.search(r"(^|;)%s=(\d+)" % re.escape(key), counts_text)
    if not match:
        raise AssertionError("Missing %s in event counts: %s" %
                             (key, counts_text))
    return int(match.group(2))


def parse_current_time(time_text):
    match = re.match(r"([0-9]+(?:\.[0-9]+)?) /", time_text)
    if not match:
        raise AssertionError("Unexpected time text: %s" % time_text)
    return float(match.group(1))


def capture_screenshot(test, lynxview, suffix):
    screenshot_dir = os.path.join(settings.PROJECT_ROOT, "screenshots",
                                  test.platform)
    os.makedirs(screenshot_dir, exist_ok=True)
    case = test.current_case
    image_name = "%s%s_%s.png" % (case.image_prefix, case.name, suffix)
    image_path = os.path.join(screenshot_dir, image_name)
    lynxview.screenshot(image_path)
    test.log_record("video screenshot: %s" % suffix,
                    EnumLogLevel.INFO,
                    attachments={
                        "test_img": image_path,
                        "case_name": case.name,
                    })
    return image_path
