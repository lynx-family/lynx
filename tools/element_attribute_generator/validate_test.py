#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import copy
import unittest

import validate


def source_model(*names):
    return {
        "schema_version": 1,
        "source": "test.d.ts",
        "attributes": list(names),
    }


def registry(*names):
    return {
        "schema_version": 1,
        "id_type": "uint16",
        "reserved_ranges": [
            {"start": 0, "end": 999, "owner": "css_property"},
            {
                "start": 1000,
                "end": 1023,
                "owner": "element_construction_metadata",
            },
        ],
        "attributes": [
            {"id": 1024 + index, "name": name, "state": "active"}
            for index, name in enumerate(names)
        ],
    }


class RegistryValidationTest(unittest.TestCase):
    def test_accepts_valid_registry(self):
        validate.validate_registry(registry("name", "src"), source_model("name", "src"))

    def test_rejects_duplicate_id(self):
        value = registry("name", "src")
        value["attributes"][1]["id"] = 1024
        with self.assertRaisesRegex(validate.ValidationError, "strictly increasing"):
            validate.validate_registry(value, source_model("name", "src"))

    def test_rejects_typing_and_registry_drift(self):
        with self.assertRaisesRegex(validate.ValidationError, "missing=.*src"):
            validate.validate_registry(registry("name"), source_model("name", "src"))
        with self.assertRaisesRegex(validate.ValidationError, "extra=.*src"):
            validate.validate_registry(registry("name", "src"), source_model("name"))

    def test_rejects_id_rename_against_baseline(self):
        baseline = registry("name")
        current = copy.deepcopy(baseline)
        current["attributes"][0]["name"] = "src"
        with self.assertRaisesRegex(validate.ValidationError, "changed name"):
            validate.validate_history(current, baseline)

    def test_rejects_tombstone_reactivation(self):
        baseline = registry("name")
        baseline["attributes"][0]["state"] = "tombstone"
        with self.assertRaisesRegex(validate.ValidationError, "became active"):
            validate.validate_history(registry("name"), baseline)

    def test_rejects_new_id_below_previous_maximum(self):
        baseline = registry("name", "src")
        baseline["attributes"][1]["id"] = 1026
        current = copy.deepcopy(baseline)
        current["attributes"].insert(
            1, {"id": 1025, "name": "flatten", "state": "active"}
        )
        with self.assertRaisesRegex(validate.ValidationError, "previous max ID"):
            validate.validate_history(current, baseline)

    def test_rejects_removed_id(self):
        baseline = registry("name", "src")
        with self.assertRaisesRegex(validate.ValidationError, "was removed"):
            validate.validate_history(registry("name"), baseline)


if __name__ == "__main__":
    unittest.main()
