# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# pylint: disable=import-error,print-statement,relative-import
"""Unit tests for idl_definitions.py."""

import unittest

from idl_definitions import IdlAttribute


class IdlAttributeTest(unittest.TestCase):
    def test_no_params(self):
        try:
            IdlAttribute()
        except Exception as exception:  # pylint: disable=broad-except
            self.fail('Creating an IdlAttribute with no parameters raised'
                      'an exception: {}.'.format(exception))
