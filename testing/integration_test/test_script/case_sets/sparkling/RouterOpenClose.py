# -*- coding: UTF-8 -*-
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

from .helpers import (
    CANONICAL_PARENT_URL,
    HOMEPAGE_CONFIG,
    assert_nonempty_tag,
    assert_sparkling_capabilities,
    assert_tag_text,
    click_visible_tag,
    open_from_homepage,
    wait_for_visible_view,
)

config = dict(HOMEPAGE_CONFIG)


def run(test):
    test.start_step("router.open creates a Sparkling child")
    parent = open_from_homepage(test, CANONICAL_PARENT_URL, "open-bundle-url")
    assert_sparkling_capabilities(test)
    parent_container_id = assert_nonempty_tag(
        test,
        "nav-container-id",
        forbidden={"absent"},
    )

    click_visible_tag(parent, "nav-open-child")
    child = wait_for_visible_view(test, "nav-role", "child")
    assert_sparkling_capabilities(test)
    child_container_id = assert_nonempty_tag(
        test,
        "nav-container-id",
        forbidden={"absent", parent_container_id},
    )
    if child_container_id == parent_container_id:
        raise AssertionError("router.open reused the parent container ID")

    test.start_step("router.close returns to the existing parent")
    click_visible_tag(child, "nav-close-child")
    wait_for_visible_view(test, "nav-role", "parent")
    assert_tag_text(test, "nav-return-success", "returned:ok")
    assert_tag_text(test, "nav-container-id", parent_container_id)
