# -*- coding: UTF-8 -*-
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

from .helpers import (
    HOMEPAGE_CONFIG,
    MAPPED_LEGACY_URL,
    assert_nonempty_tag,
    assert_sparkling_capabilities,
    assert_tag_text,
    open_from_homepage,
)

config = dict(HOMEPAGE_CONFIG)


def run(test):
    test.start_step("Legacy URL options map into a Sparkling context")
    open_from_homepage(
        test,
        MAPPED_LEGACY_URL,
        "open-bundle-url",
        runtime="sparkling",
    )
    assert_sparkling_capabilities(test)
    expected_props = {
        "nav-title": "Mapped Title",
        "nav-hidden-nav": "yes",
        "nav-fullscreen": "no",
        "nav-back-button-style": "dark",
        "nav-initial-page": "details",
        "nav-custom-flag": "preserved",
        "nav-theme": "page-theme",
    }
    for tag, expected in expected_props.items():
        assert_tag_text(test, tag, expected)
    assert_nonempty_tag(test, "nav-is-notch-screen", forbidden={"absent"})
