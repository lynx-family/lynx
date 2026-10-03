# -*- coding: UTF-8 -*-
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

from .helpers import (
    HOMEPAGE_CONFIG,
    RAW_PARENT_URL,
    assert_legacy_capabilities,
    open_from_homepage,
)

config = dict(HOMEPAGE_CONFIG)


def run(test):
    test.start_step("Raw bundle + Open stays on the Legacy container")
    parent = open_from_homepage(
        test,
        RAW_PARENT_URL,
        "open-bundle-url",
        runtime="lynx",
    )
    assert_legacy_capabilities(test, parent)
