# -*- coding: UTF-8 -*-
# Copyright 2026 The Lynx Authors. All rights reserved.
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
    video_utils.wait_for_text(test, lynxview, "status-text", "ready")

    test.start_step("--------Test1: Test volume and muted attrs;-------")
    video_utils.click(lynxview, "btn-volume-low")
    video_utils.wait_for_contains(test, lynxview, "attr-state", "volume=0.3")
    video_utils.click(lynxview, "btn-muted-on")
    video_utils.wait_for_contains(test, lynxview, "attr-state", "muted=true")
    video_utils.click(lynxview, "btn-muted-off")
    video_utils.wait_for_contains(test, lynxview, "attr-state", "muted=false")
    video_utils.click(lynxview, "btn-volume-high")
    video_utils.wait_for_contains(test, lynxview, "attr-state", "volume=1.0")

    test.start_step("--------Test2: Test object-fit attrs with screenshots;-------")
    video_utils.click(lynxview, "btn-play")
    video_utils.wait_for_text(test, lynxview, "status-text", "playing")
    video_utils.click(lynxview, "btn-object-contain")
    video_utils.wait_for_contains(test, lynxview, "attr-state", "fit=contain")
    video_utils.capture_screenshot(test, lynxview, "fit_contain")
    video_utils.click(lynxview, "btn-object-cover")
    video_utils.wait_for_contains(test, lynxview, "attr-state", "fit=cover")
    video_utils.capture_screenshot(test, lynxview, "fit_cover")
    video_utils.click(lynxview, "btn-object-fill")
    video_utils.wait_for_contains(test, lynxview, "attr-state", "fit=fill")
    video_utils.capture_screenshot(test, lynxview, "fit_fill")

    test.start_step("--------Test3: Test speed attr affects playback progress;-------")
    video_utils.click(lynxview, "btn-pause")
    video_utils.wait_for_text(test, lynxview, "status-text", "paused")
    video_utils.click(lynxview, "btn-speed-half")
    video_utils.wait_for_contains(test, lynxview, "attr-state", "speed=0.5")
    video_utils.click(lynxview, "btn-seek-start")
    video_utils.click(lynxview, "btn-clear-signals")
    video_utils.click(lynxview, "btn-play")
    video_utils.wait_for_text(test, lynxview, "status-text", "playing")
    time.sleep(2.5)
    slow_time = video_utils.parse_current_time(
        video_utils.get_text(lynxview, "time-text"))

    video_utils.click(lynxview, "btn-pause")
    video_utils.wait_for_text(test, lynxview, "status-text", "paused")
    video_utils.click(lynxview, "btn-speed-double")
    video_utils.wait_for_contains(test, lynxview, "attr-state", "speed=2.0")
    video_utils.click(lynxview, "btn-seek-start")
    video_utils.click(lynxview, "btn-clear-signals")
    video_utils.click(lynxview, "btn-play")
    video_utils.wait_for_text(test, lynxview, "status-text", "playing")
    time.sleep(2.5)
    fast_time = video_utils.parse_current_time(
        video_utils.get_text(lynxview, "time-text"))
    if fast_time < 3.0 or fast_time <= slow_time + 1.5:
        raise AssertionError("speed=2.0 should advance faster than speed=0.5; "
                             "slow=%s fast=%s" % (slow_time, fast_time))

    test.start_step("--------Test4: Test timeupdate interval attr;-------")
    video_utils.click(lynxview, "btn-pause")
    video_utils.wait_for_text(test, lynxview, "status-text", "paused")
    video_utils.click(lynxview, "btn-speed-normal")
    video_utils.wait_for_contains(test, lynxview, "attr-state", "speed=1.0")
    video_utils.click(lynxview, "btn-timeupdate-slow")
    video_utils.wait_for_contains(test, lynxview, "attr-state", "requestedInterval=1.0")
    video_utils.click(lynxview, "btn-seek-start")
    video_utils.click(lynxview, "btn-clear-signals")
    video_utils.click(lynxview, "btn-play")
    video_utils.wait_for_text(test, lynxview, "status-text", "playing")
    video_utils.wait_for_count_at_least(test, lynxview, "timeupdate", 3,
                                        timeout=8)
    video_utils.wait_until(
        test,
        lynxview,
        "min-timeupdate-delta",
        lambda text: float(text) >= 0.85,
        "timeupdate interval should be close to 1 second",
        timeout=3,
    )

    test.start_step("--------Test5: Test invalid timeupdate interval keeps previous valid value;-------")
    video_utils.click(lynxview, "btn-pause")
    video_utils.wait_for_text(test, lynxview, "status-text", "paused")
    video_utils.click(lynxview, "btn-timeupdate-zero")
    video_utils.wait_for_contains(test, lynxview, "attr-state", "requestedInterval=0.0")
    video_utils.click(lynxview, "btn-seek-start")
    video_utils.click(lynxview, "btn-clear-signals")
    video_utils.click(lynxview, "btn-play")
    video_utils.wait_for_text(test, lynxview, "status-text", "playing")
    time.sleep(3.6)
    video_utils.assert_count_at_most(lynxview, "timeupdate", 5)
    video_utils.wait_until(
        test,
        lynxview,
        "min-timeupdate-delta",
        lambda text: float(text) >= 0.85,
        "invalid timeupdate interval should keep the previous valid interval",
        timeout=3,
    )

    test.start_step("--------Test6: Test tiny positive timeupdate interval is accepted;-------")
    video_utils.click(lynxview, "btn-stop")
    video_utils.wait_for_text(test, lynxview, "status-text", "stopped")
    video_utils.click(lynxview, "btn-timeupdate-fast")
    video_utils.wait_for_text(test, lynxview, "interval-state", "0.001")
    video_utils.click(lynxview, "btn-seek-start")
    video_utils.click(lynxview, "btn-clear-signals")
    video_utils.click(lynxview, "btn-play")
    video_utils.wait_for_text(test, lynxview, "status-text", "playing")
    time.sleep(1.2)
    fast_count = video_utils.parse_count(
        video_utils.get_text(lynxview, "event-counts"), "timeupdate")
    if fast_count < 30:
        raise AssertionError("tiny positive timeupdate interval should not be "
                             "clamped to a large engineering lower bound; "
                             "count=%s" % fast_count)
