# -*- coding: UTF-8 -*-
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

from .helpers import (
    CANONICAL_PARENT_URL,
    HOMEPAGE_CONFIG,
    assert_sparkling_capabilities,
    assert_tag_text,
    open_from_homepage,
)

config = dict(HOMEPAGE_CONFIG)


def run(test):
    test.start_step("A canonical scheme owns the full Sparkling container")
    open_from_homepage(test, CANONICAL_PARENT_URL, "open-bundle-url")
    assert_tag_text(test, "nav-role", "parent")
    assert_sparkling_capabilities(test)
