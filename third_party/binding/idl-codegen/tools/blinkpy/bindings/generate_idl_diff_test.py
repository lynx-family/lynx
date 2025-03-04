#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import unittest

from blinkpy.bindings import generate_idl_diff
from blinkpy.bindings.generate_idl_diff import DIFF_TAG
from blinkpy.bindings.generate_idl_diff import DIFF_TAG_DELETED
from blinkpy.bindings.generate_idl_diff import DIFF_TAG_ADDED

testdata_path = os.path.join(
    os.path.dirname(os.path.realpath(__file__)), 'testdata')
old_data_path = os.path.join(testdata_path, 'old_blink_idls.json')
new_data_path = os.path.join(testdata_path, 'new_blink_idls.json')


class TestGenerateIDLDiff(unittest.TestCase):
    def setUp(self):
        old = generate_idl_diff.load_json_file(old_data_path)
        new = generate_idl_diff.load_json_file(new_data_path)
        self.diff = generate_idl_diff.interfaces_diff(old, new)

    def test_deleted_interface(self):
        self.assertTrue('AnimationEffectReadOnly' in self.diff)
        deleted_interface = self.diff.get('AnimationEffectReadOnly')
        self.assertIsNotNone(deleted_interface)
        self.assertEqual(deleted_interface.get(DIFF_TAG), DIFF_TAG_DELETED)

    def test_added_interface(self):
        self.assertTrue('AnimationEvent' in self.diff)
        added_interface = self.diff.get('AnimationEvent')
        self.assertIsNotNone(added_interface)
        self.assertEqual(added_interface.get(DIFF_TAG), DIFF_TAG_ADDED)

    def test_changed_interface(self):
        self.assertTrue('ANGLEInstancedArrays' in self.diff)
        changed_interface = self.diff.get('ANGLEInstancedArrays')
        self.assertIsNotNone(changed_interface)
        self.assertIsNone(changed_interface.get(DIFF_TAG))

    def test_unchanged_interface(self):
        self.assertFalse('AbstractWorker' in self.diff)

    def test_unchanged_consts(self):
        changed_interface = self.diff['ANGLEInstancedArrays']
        members = changed_interface['Consts']
        for member in members:
            self.assertEqual(member['Name'], 'VERTEX_ATTRIB_ARRAY_DIVISOR')
            self.assertEqual(member['Type'], 'unsigned long')
            self.assertEqual(member['Value'], '0x88FE')

    def test_changed_attribute(self):
        changed_interface = self.diff['ANGLEInstancedArrays']
        members = changed_interface['Attributes']
        for member in members:
            if member.get(DIFF_TAG) == DIFF_TAG_DELETED:
                deleted = member
            elif member.get(DIFF_TAG) == DIFF_TAG_ADDED:
                added = member
            else:
                unchanged = member
        self.assertEqual(deleted['Name'], 'animVal')
        self.assertEqual(deleted['Type'], 'SVGAngle')
        self.assertEqual(deleted['ExtAttributes'], [])
        self.assertEqual(added['Name'], 'computedTiming')
        self.assertEqual(added['Type'], 'ComputedTimingProperties')
        self.assertEqual(added['ExtAttributes'], [{'Name': 'maxChannelCount'}])
        self.assertEqual(unchanged['Name'], 'timing')
        self.assertEqual(unchanged['Type'], 'AnimationEffectTiming')
        self.assertEqual(unchanged['ExtAttributes'], [])

    def test_changed_operation(self):
        changed_interface = self.diff['ANGLEInstancedArrays']
        members = changed_interface['Operations']
        deleted_arguments = [{'Type': 'long', 'Name': 'primcount'}]
        added_arguments = [{'Type': 'unsigned long', 'Name': 'mode'}]
        unchanged_arguments = [{'Type': 'unsigned long', 'Name': 'mode'}]
        for member in members:
            if member.get(DIFF_TAG) == DIFF_TAG_DELETED:
                deleted = member
            elif member.get(DIFF_TAG) == DIFF_TAG_ADDED:
                added = member
            else:
                unchanged = member
        self.assertEqual(deleted['Name'], 'drawElementsInstancedANGLE')
        self.assertEqual(deleted['Type'], 'void')
        self.assertEqual(deleted['ExtAttributes'], [])
        self.assertEqual(deleted['Arguments'], deleted_arguments)
        self.assertEqual(added['Name'], 'drawElementsInstancedANGLE')
        self.assertEqual(added['Type'], 'void')
        self.assertEqual(added['ExtAttributes'], [])
        self.assertEqual(added['Arguments'], added_arguments)
        self.assertEqual(unchanged['Name'], 'drawArraysInstancedANGLE')
        self.assertEqual(unchanged['Type'], 'void')
        self.assertEqual(unchanged['ExtAttributes'], [])
        self.assertEqual(unchanged['Arguments'], unchanged_arguments)


if __name__ == '__main__':
    unittest.main()
