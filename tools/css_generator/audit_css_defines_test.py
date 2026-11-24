#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import json
import os
import tempfile
import unittest

import audit_css_defines as auditor


def _write_file(path, content):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as handle:
        handle.write(content)


class AuditCssDefinesTest(unittest.TestCase):
    def test_repository_audit_runs_cleanly(self):
        report = auditor.audit_css_defines()
        self.assertGreater(report["total_properties"], 200)
        self.assertEqual(report["missing_handler_references"], [])

    def test_detects_engine_mismatches(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            css_dir = os.path.join(temp_dir, "css_defines")
            os.makedirs(css_dir, exist_ok=True)
            _write_file(
                os.path.join(css_dir, "1-align-self.json"),
                json.dumps(
                    {
                        "name": "align-self",
                        "id": 1,
                        "type": "enum",
                        "default_value": "stretch",
                        "consumption_status": "layout-only",
                        "is_shorthand": True,
                        "values": [
                            {"value": "auto", "version": "1.0"},
                            {"value": "stretch", "version": "1.0"},
                        ],
                        "formal_syntax": "stretch",
                    }
                ),
            )
            _write_file(
                os.path.join(css_dir, "2-perspective.json"),
                json.dumps(
                    {
                        "name": "perspective",
                        "id": 2,
                        "type": "length",
                        "default_value": "none",
                        "consumption_status": "skip",
                        "is_shorthand": False,
                        "values": [{"value": "px", "version": "1.0"}],
                        "desc": "desc",
                        "formal_syntax": "<length>",
                        "compat_data": {},
                    }
                ),
            )

            css_property_cc = os.path.join(temp_dir, "css_property.cc")
            _write_file(
                css_property_cc,
                "const std::unordered_set<CSSPropertyID> shorthandCSSProperties{kPropertyIDPadding};",
            )
            computed_style_h = os.path.join(temp_dir, "computed_css_style.h")
            _write_file(
                computed_style_h,
                "#define FOREACH_PLATFORM_PROPERTY(V) \\\n  V(AlignSelf)\n\n",
            )
            default_computed_h = os.path.join(temp_dir, "default_computed_style.h")
            _write_file(default_computed_h, "")
            default_layout_h = os.path.join(temp_dir, "default_layout_style.h")
            _write_file(
                default_layout_h,
                "static constexpr FlexAlignType SL_DEFAULT_ALIGN_SELF = FlexAlignType::kAuto;",
            )
            config_h = os.path.join(temp_dir, "config.h")
            _write_file(config_h, "#define DEFAULT_FONT_SIZE_DP 14\n")
            parser_dir = os.path.join(temp_dir, "parser")
            os.makedirs(parser_dir, exist_ok=True)

            report = auditor.audit_css_defines(
                css_defines_dir=css_dir,
                css_property_cc=css_property_cc,
                computed_style_h=computed_style_h,
                default_computed_h=default_computed_h,
                default_layout_h=default_layout_h,
                config_h=config_h,
                parser_dir=parser_dir,
            )

            self.assertEqual(len(report["shorthand_mismatches"]), 1)
            self.assertEqual(len(report["platform_layout_conflicts"]), 1)
            self.assertEqual(len(report["default_value_mismatches"]), 1)
            self.assertEqual(len(report["enum_syntax_mismatches"]), 1)
            self.assertEqual(len(report["non_enum_with_values"]), 1)
            self.assertEqual(report["missing_metadata"]["desc"], ["1-align-self.json"])


if __name__ == "__main__":
    unittest.main()
