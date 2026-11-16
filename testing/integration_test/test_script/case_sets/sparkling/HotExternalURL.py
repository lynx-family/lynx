# -*- coding: UTF-8 -*-
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

from .helpers import (
    CANONICAL_PARENT_URL,
    HOMEPAGE_CONFIG,
    MALFORMED_CANONICAL_URL,
    accept_route_error_alert,
    assert_sparkling_capabilities,
    assert_tag_text,
    open_external_url,
    wait_for_visible_view,
)

config = dict(HOMEPAGE_CONFIG)


def run(test):
    test.start_step("A foreground Explorer receives hot external URLs")
    wait_for_visible_view(test, "bundle-url-input")

    open_external_url(test, MALFORMED_CANONICAL_URL)
    accept_route_error_alert(test, "missing_target")
    wait_for_visible_view(test, "bundle-url-input")

    open_external_url(test, CANONICAL_PARENT_URL)
    assert_tag_text(test, "nav-role", "parent")
    assert_sparkling_capabilities(test)
