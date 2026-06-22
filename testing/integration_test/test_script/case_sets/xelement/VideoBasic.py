# -*- coding: UTF-8 -*-
# Copyright 2024 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import time

from case_sets.xelement import video_utils

config = {
    "type": "custom",
    "path": "automation/video/main",
    "platform": ["android", "ios"],
}


def run(test):
    lynxview = video_utils.get_lynxview(test)

    test.start_step("--------Test1: Wait for video ready;-------")
    status_view = lynxview.get_by_test_tag("status-text")
    test.wait_for_equal("Video not ready", status_view, "text", "ready")
    video_utils.wait_for_count_at_least(test, lynxview, "firstframe", 1)
    video_utils.wait_until(
        test,
        lynxview,
        "firstframe-duration",
        lambda text: float(text) > 0,
        "firstframe duration should be reported",
    )
    video_utils.click(lynxview, "btn-clear-signals")
    time.sleep(1.2)
    video_utils.assert_count_equals(lynxview, "timeupdate", 0)
    time.sleep(2)
    video_utils.capture_screenshot(test, lynxview, "ready")

    test.start_step("--------Test2: Test play;-------")
    video_utils.click(lynxview, "btn-clear-signals")
    video_utils.click(lynxview, "btn-play")
    test.wait_for_equal("Play failed", status_view, "text", "playing")
    video_utils.wait_for_contains(test, lynxview, "callback-log", "play_ok")
    video_utils.wait_for_count_at_least(test, lynxview, "playing", 1)
    video_utils.wait_for_count_at_least(test, lynxview, "timeupdate", 1)
    time.sleep(3)

    test.start_step("--------Test3: Test pause;-------")
    video_utils.click(lynxview, "btn-pause")
    test.wait_for_equal("Pause failed", status_view, "text", "paused")
    video_utils.wait_for_contains(test, lynxview, "callback-log", "pause_ok")
    video_utils.wait_for_count_at_least(test, lynxview, "paused", 1)
    video_utils.click(lynxview, "btn-clear-signals")
    time.sleep(1.2)
    video_utils.assert_count_equals(lynxview, "timeupdate", 0)
    time.sleep(2)

    test.start_step("--------Test4: Test seek;-------")
    video_utils.click(lynxview, "btn-clear-signals")
    video_utils.click(lynxview, "btn-seek")
    video_utils.wait_for_contains(test, lynxview, "callback-log", "seek_ok")
    time.sleep(1.2)
    video_utils.assert_count_equals(lynxview, "timeupdate", 0)
    video_utils.click(lynxview, "btn-play")
    test.wait_for_equal("Play after seek failed", status_view, "text",
                        "playing")
    video_utils.wait_until(
        test,
        lynxview,
        "time-text",
        lambda text: 2 <= video_utils.parse_current_time(text) < 6,
        "play after seek should continue from seek position",
        timeout=5,
    )

    test.start_step("--------Test5: Test stop;-------")
    video_utils.click(lynxview, "btn-stop")
    test.wait_for_equal("Stop failed", status_view, "text", "stopped")
    video_utils.wait_for_contains(test, lynxview, "callback-log", "stop_ok")
    video_utils.wait_for_count_at_least(test, lynxview, "stopped", 1)
    video_utils.click(lynxview, "btn-clear-signals")
    time.sleep(1.2)
    video_utils.assert_count_equals(lynxview, "timeupdate", 0)
    time.sleep(2)
    video_utils.capture_screenshot(test, lynxview, "stopped")

    test.start_step("--------Test6: Test play after stop;-------")
    video_utils.click(lynxview, "btn-play")
    test.wait_for_equal("Play after stop failed", status_view, "text", "playing")
    video_utils.wait_for_contains(test, lynxview, "callback-log", "play_ok")
    video_utils.wait_until(
        test,
        lynxview,
        "time-text",
        lambda text: 0 < video_utils.parse_current_time(text) < 5,
        "play after stop should restart from the beginning",
        timeout=5,
    )
    time.sleep(2)

    test.start_step("--------Test7: Test play after ended;-------")
    video_utils.click(lynxview, "btn-clear-signals")
    video_utils.click(lynxview, "btn-seek-near-end")
    video_utils.wait_for_contains(test, lynxview, "callback-log", "seek_ok")
    test.wait_for_equal("Video did not reach ended state", status_view, "text",
                        "ended", timeout=8)
    video_utils.wait_for_count_at_least(test, lynxview, "ended", 1)
    video_utils.assert_count_equals(lynxview, "looped", 0)
    video_utils.click(lynxview, "btn-clear-signals")
    time.sleep(1.2)
    video_utils.assert_count_equals(lynxview, "timeupdate", 0)
    video_utils.click(lynxview, "btn-play")
    test.wait_for_equal("Play after ended failed", status_view, "text",
                        "playing", timeout=5)
    video_utils.wait_for_contains(test, lynxview, "callback-log", "play_ok")
    video_utils.wait_for_count_at_least(test, lynxview, "playing", 1)
    video_utils.wait_until(
        test,
        lynxview,
        "time-text",
        lambda text: 0 < video_utils.parse_current_time(text) < 5,
        "play after ended should restart from the beginning",
        timeout=5,
    )
    time.sleep(2)

    test.start_step("--------Test8: Test loop on;-------")
    video_utils.click(lynxview, "btn-stop")
    video_utils.wait_for_text(test, lynxview, "status-text", "stopped")
    video_utils.click(lynxview, "btn-clear-signals")
    video_utils.click(lynxview, "btn-loop-on")
    video_utils.wait_for_contains(test, lynxview, "attr-state", "loop=true")
    time.sleep(1)

    test.start_step("--------Test9: Test play to end with loop;-------")
    video_utils.click(lynxview, "btn-play")
    test.wait_for_equal("Loop play failed", status_view, "text", "playing")
    video_utils.click(lynxview, "btn-seek-near-end")
    looped_state = lynxview.get_by_test_tag("looped-state")
    test.wait_for_equal("Loop event not observed", looped_state, "text",
                        "looped", timeout=15)
    video_utils.wait_for_count_at_least(test, lynxview, "looped", 1)
    video_utils.assert_count_equals(lynxview, "ended", 0)
    video_utils.wait_for_text(test, lynxview, "status-text", "playing")
    video_utils.wait_until(
        test,
        lynxview,
        "time-text",
        lambda text: 0 < video_utils.parse_current_time(text) < 5,
        "loop playback should continue from the beginning",
        timeout=5,
    )
    video_utils.capture_screenshot(test, lynxview, "loop")
