# -*- coding: UTF-8 -*-
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

from case_sets.xelement import video_utils

config = {
    "type": "custom",
    "path": "automation/video/main",
    "platform": "android",
}


def run(test):
    lynxview = video_utils.get_lynxview(test)
    video_utils.wait_for_text(test, lynxview, "status-text", "ready")

    test.start_step("--------Test1: Queue mode executes all operations in order;-------")
    video_utils.click(lynxview, "btn-mode-queue")
    video_utils.wait_for_contains(test, lynxview, "attr-state", "mode=queue")
    video_utils.click(lynxview, "btn-burst-play-pause-stop")
    video_utils.wait_for_text(test, lynxview, "status-text", "stopped",
                              timeout=15)
    video_utils.wait_for_contains(test, lynxview, "callback-log", "stop_ok")
    video_utils.assert_text_order(
        lynxview,
        "callback-log",
        ["play_ok", "pause_ok", "stop_ok"],
    )
    video_utils.assert_text_order(
        lynxview,
        "signal-log",
        [
            "event:playing",
            "callback:play_ok",
            "event:paused",
            "callback:pause_ok",
            "event:stopped",
            "callback:stop_ok",
        ],
    )

    test.start_step("--------Test2: Latest mode keeps only latest pending operation;-------")
    video_utils.click(lynxview, "btn-clear-signals")
    video_utils.click(lynxview, "btn-mode-latest")
    video_utils.wait_for_contains(test, lynxview, "attr-state", "mode=latest")
    video_utils.click(lynxview, "btn-burst-play-pause-stop")
    video_utils.wait_for_text(test, lynxview, "status-text", "stopped",
                              timeout=15)
    video_utils.wait_for_contains(test, lynxview, "callback-log", "stop_ok")
    video_utils.assert_text_not_contains(lynxview, "callback-log", "pause_ok")

    test.start_step("--------Test3: Direct mode executes without queue waiting;-------")
    video_utils.click(lynxview, "btn-clear-signals")
    video_utils.click(lynxview, "btn-mode-direct")
    video_utils.wait_for_contains(test, lynxview, "attr-state", "mode=direct")
    video_utils.click(lynxview, "btn-burst-play-stop")
    video_utils.wait_for_contains(test, lynxview, "callback-log", "play_ok")
    video_utils.wait_for_contains(test, lynxview, "callback-log", "stop_ok")
    video_utils.assert_text_order(
        lynxview,
        "callback-log",
        ["play_ok", "stop_ok"],
    )

    video_utils.click(lynxview, "btn-clear-signals")
    video_utils.click(lynxview, "btn-seek")
    video_utils.wait_for_contains(test, lynxview, "callback-log", "seek_ok")
    video_utils.click(lynxview, "btn-stop")
    video_utils.wait_for_text(test, lynxview, "status-text", "stopped")
    video_utils.wait_for_contains(test, lynxview, "callback-log", "stop_ok")
    video_utils.capture_screenshot(test, lynxview, "mode_direct")
