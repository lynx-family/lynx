# -*- coding: UTF-8 -*-
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

from .helpers import (
    HOMEPAGE_CONFIG,
    RAW_PARENT_URL,
    accept_route_error_alert,
    assert_nonempty_tag,
    assert_sparkling_capabilities,
    assert_tag_text,
    click_visible_tag,
    open_from_homepage,
    wait_for_visible_view,
)

config = dict(HOMEPAGE_CONFIG)


def run(test):
    test.start_step("Malformed canonical route reports an error without fallback")
    parent = open_from_homepage(
        test,
        RAW_PARENT_URL,
        "open-bundle-url",
        runtime="sparkling",
    )
    assert_sparkling_capabilities(test)
    parent_container_id = assert_nonempty_tag(
        test,
        "nav-container-id",
        forbidden={"absent"},
    )
    click_visible_tag(parent, "nav-open-malformed")
    accept_route_error_alert(test, "missing_target")
    assert_tag_text(test, "nav-route-result", "malformed:missing_target")
    wait_for_visible_view(test, "nav-role", "parent")
    assert_sparkling_capabilities(test)
    assert_tag_text(test, "nav-container-id", parent_container_id)
