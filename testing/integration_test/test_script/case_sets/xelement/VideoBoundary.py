# -*- coding: UTF-8 -*-
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import time

from case_sets.xelement import video_utils


def assert_failed_callback(lynxview, method, msg=None):
    video_utils.assert_text_contains(lynxview, "callback-log",
                                     "%s_fail" % method)
    video_utils.assert_text_contains(lynxview, "callback-log",
                                     "success=false")
    video_utils.assert_text_contains(lynxview, "callback-log",
                                     "errorCode=")
    if msg:
        video_utils.assert_text_contains(lynxview, "callback-log",
                                         "msg=%s" % msg)


def assert_bridge_failed_callback(lynxview, method, code):
    video_utils.assert_text_contains(lynxview, "callback-log",
                                     "%s_fail" % method)
    video_utils.assert_text_contains(lynxview, "callback-log",
                                     '"code":%s' % code)


config = {
    "type": "custom",
    "path": "automation/video/main",
    "platform": "android",
}


def run(test):
    lynxview = video_utils.get_lynxview(test)
    video_utils.wait_for_text(test, lynxview, "status-text", "ready")

    test.start_step("--------Test1: Switching src stops previous playback;-------")
    video_utils.click(lynxview, "btn-play")
    video_utils.wait_for_text(test, lynxview, "status-text", "playing")
    video_utils.click(lynxview, "btn-src-secondary")
    video_utils.wait_for_count_at_least(test, lynxview, "buffering", 1,
                                        timeout=10)
    video_utils.wait_for_text(test, lynxview, "status-text", "ready",
                              timeout=20)
    video_utils.wait_for_contains(test, lynxview, "attr-state",
                                  "src=secondary")
    video_utils.wait_for_count_at_least(test, lynxview, "firstframe", 1)
    video_utils.capture_screenshot(test, lynxview, "src_secondary")

    test.start_step("--------Test2: Invalid src dispatches error;-------")
    video_utils.click(lynxview, "btn-src-invalid")
    video_utils.wait_for_text(test, lynxview, "status-text", "error",
                              timeout=20)
    video_utils.wait_for_count_at_least(test, lynxview, "error", 1)
    video_utils.wait_until(
        test,
        lynxview,
        "last-error",
        lambda text: text != "none" and ":" in text,
        "invalid src should report error details",
    )

    test.start_step("--------Test3: Empty src makes play callback fail;-------")
    video_utils.click(lynxview, "btn-src-empty")
    video_utils.wait_for_text(test, lynxview, "status-text", "empty")
    video_utils.click(lynxview, "btn-clear-signals")
    video_utils.click(lynxview, "btn-play")
    video_utils.wait_for_text(test, lynxview, "status-text", "error")
    video_utils.wait_for_contains(test, lynxview, "callback-log",
                                  "play_fail")
    assert_failed_callback(lynxview, "play", "missing video source")
    video_utils.wait_for_count_at_least(test, lynxview, "error", 1)
    video_utils.click(lynxview, "btn-clear-signals")
    video_utils.click(lynxview, "btn-stop")
    video_utils.wait_for_text(test, lynxview, "status-text", "stopped")
    video_utils.wait_for_contains(test, lynxview, "callback-log",
                                  "stop_ok")

    test.start_step(
        "--------Test4: Src switch cancels in-flight serial method;-------")
    video_utils.click(lynxview, "btn-src-primary")
    video_utils.wait_for_text(test, lynxview, "status-text", "ready",
                              timeout=20)
    video_utils.click(lynxview, "btn-mode-queue")
    video_utils.wait_for_contains(test, lynxview, "attr-state", "mode=queue")
    video_utils.click(lynxview, "btn-play-pause-then-src-empty")
    video_utils.wait_for_contains(test, lynxview, "callback-log",
                                  "play_fail", timeout=10)
    assert_failed_callback(lynxview, "play",
                           "request canceled by src change")
    video_utils.wait_for_contains(test, lynxview, "callback-log",
                                  "pause_fail", timeout=10)
    assert_failed_callback(lynxview, "pause",
                           "request canceled by src change")
    video_utils.click(lynxview, "btn-clear-signals")
    video_utils.click(lynxview, "btn-stop")
    video_utils.wait_for_text(test, lynxview, "status-text", "stopped")
    video_utils.wait_for_contains(test, lynxview, "callback-log",
                                  "stop_ok")

    test.start_step("--------Test5: Seek failure callbacks;-------")
    video_utils.click(lynxview, "btn-src-primary")
    video_utils.wait_for_text(test, lynxview, "status-text", "ready",
                              timeout=20)
    time.sleep(1)
    video_utils.click(lynxview, "btn-clear-signals")
    video_utils.click(lynxview, "btn-seek-out-of-range")
    video_utils.wait_for_contains(test, lynxview, "callback-log",
                                  "seek_fail")
    assert_failed_callback(lynxview, "seek", "position out of range")
    video_utils.click(lynxview, "btn-clear-signals")
    video_utils.click(lynxview, "btn-seek-missing-position")
    video_utils.wait_for_contains(test, lynxview, "callback-log",
                                  "seek_fail")
    assert_failed_callback(lynxview, "seek", "missing position param")
    video_utils.click(lynxview, "btn-clear-signals")
    video_utils.click(lynxview, "btn-seek-null-params")
    video_utils.wait_for_contains(test, lynxview, "callback-log",
                                  "seek_fail")
    assert_failed_callback(lynxview, "seek", "missing position param")
    video_utils.click(lynxview, "btn-clear-signals")
    video_utils.click(lynxview, "btn-seek-string-position")
    video_utils.wait_for_contains(test, lynxview, "callback-log",
                                  "seek_fail")
    assert_failed_callback(lynxview, "seek",
                           "position param must be a number")

    test.start_step(
        "--------Test6: Null params and unknown method callbacks;-------")
    video_utils.click(lynxview, "btn-clear-signals")
    video_utils.click(lynxview, "btn-play-null-params")
    video_utils.wait_for_text(test, lynxview, "status-text", "playing")
    video_utils.wait_for_contains(test, lynxview, "callback-log",
                                  "play_ok")
    video_utils.wait_for_contains(test, lynxview, "callback-log",
                                  "success=true")
    video_utils.click(lynxview, "btn-stop")
    video_utils.wait_for_text(test, lynxview, "status-text", "stopped")
    video_utils.click(lynxview, "btn-clear-signals")
    video_utils.click(lynxview, "btn-method-unknown")
    video_utils.wait_for_contains(test, lynxview, "callback-log",
                                  "unknownVideoMethod_fail")
    assert_bridge_failed_callback(lynxview, "unknownVideoMethod", 3)
