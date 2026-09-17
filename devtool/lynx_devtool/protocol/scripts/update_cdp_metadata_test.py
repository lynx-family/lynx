#!/usr/bin/env python3
# Copyright 2026 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import tempfile
import unittest
from dataclasses import replace
from pathlib import Path

import update_cdp_metadata


class CdpMetadataScanTest(unittest.TestCase):
    def setUp(self):
        temp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(temp_dir.cleanup)
        root = Path(temp_dir.name).resolve()
        agents = root / "agents"
        agents.mkdir()
        primjs = root / "protocols.cc"
        primjs.write_text(
            'const debug_function_type& GetDebugFunctionMap() {\n'
            '  return {{"Runtime.enable", Enable}};\n}\n',
            encoding="utf-8",
        )
        self.paths = replace(
            update_cdp_metadata.get_paths(),
            source_root=root,
            domain_agent_dir=agents,
            primjs_protocols_source=primjs,
        )

    def test_scan_map_and_direct_dispatch_with_source_and_deduplication(self):
        (self.paths.domain_agent_dir / "sample.cc").write_text(
            'functions_map_["Sample.mapped"] = &SampleAgent::Mapped;\n'
            'void SampleAgent::CallMethod(const Json::Value& message) {\n'
            '  if (method == "Sample.direct") { Direct(); }\n'
            '  else if (\n method == "Sample.mapped"\n) { Mapped(); }\n'
            '  else if (method == "Runtime.forwarded") { Forward(); }\n'
            '}\n'
            'void SampleAgent::HandleNotification() {\n'
            '  if (method == "Sample.notification") { Notify(); }\n'
            '}\n',
            encoding="utf-8",
        )
        (self.paths.domain_agent_dir / "sample_unittest.cc").write_text(
            'void TestAgent::CallMethod() {\n'
            '  if (method == "Sample.testOnly") { Test(); }\n'
            '}\n',
            encoding="utf-8",
        )

        self.assertEqual(
            update_cdp_metadata.scan_local_methods(self.paths),
            [
                update_cdp_metadata.LocalMethod("Runtime", "enable", "protocols.cc"),
                update_cdp_metadata.LocalMethod("Sample", "direct", "agents/sample.cc"),
                update_cdp_metadata.LocalMethod("Sample", "mapped", "agents/sample.cc"),
            ],
        )

    def test_reject_duplicate_map_and_direct_dispatch_in_different_files(self):
        (self.paths.domain_agent_dir / "mapped.cc").write_text(
            'functions_map_["Sample.method"] = &SampleAgent::Method;\n',
            encoding="utf-8",
        )
        (self.paths.domain_agent_dir / "direct.cc").write_text(
            'void SampleAgent::CallMethod() {\n'
            '  if (method == "Sample.method") { Method(); }\n'
            '}\n',
            encoding="utf-8",
        )

        with self.assertRaisesRegex(
            update_cdp_metadata.MetadataError,
            r"duplicate method declaration for Sample\.method",
        ):
            update_cdp_metadata.scan_local_methods(self.paths)


class CdpMetadataSinceTest(unittest.TestCase):
    def test_scan_current_lynx_version(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            config = Path(temp_dir) / "config.h"
            config.write_text(
                "#define LYNX_VERSION tasm::V_4_2  // need updated when release lynx\n",
                encoding="utf-8",
            )

            self.assertEqual(
                update_cdp_metadata.scan_current_lynx_version(config), "4.2"
            )

    def test_load_existing_manifest_since(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            manifest = Path(temp_dir) / "cdp_manifest.generated.yaml"
            manifest.write_text(
                "\n".join(
                    [
                        "domains:",
                        "  - name: DOM",
                        "    origin: standard",
                        "    scope: instance",
                        "    methods:",
                        "      - name: getDocument",
                        "        origin: standard",
                        '        since: "3.2"',
                        "        source: dom.cc",
                        "    events:",
                        "      - name: documentUpdated",
                        "        origin: standard",
                        "        since: '3.3'",
                        "        source: dom.cc",
                        "",
                    ]
                ),
                encoding="utf-8",
            )

            existing_manifest = update_cdp_metadata.load_existing_manifest(manifest)

            self.assertIn("DOM", existing_manifest.domains)
            self.assertIn(("DOM", "getDocument"), existing_manifest.methods)
            self.assertIn(("DOM", "documentUpdated"), existing_manifest.events)
            self.assertEqual(
                existing_manifest.method_since[("DOM", "getDocument")], "3.2"
            )
            self.assertEqual(
                existing_manifest.event_since[("DOM", "documentUpdated")], "3.3"
            )

    def test_build_manifest_assigns_since_for_method_entries(self):
        existing_manifest = update_cdp_metadata.ExistingManifest(
            domains={"Component", "DOM"},
            methods={
                ("Component", "uselessUpdate"),
                ("DOM", "getDocument"),
                ("DOM", "getDocumentWithBoxModel"),
            },
            events=set(),
            method_since={
                ("Component", "uselessUpdate"): "3.8",
                ("DOM", "getDocument"): "3.8",
                ("DOM", "getDocumentWithBoxModel"): "3.2",
            },
            event_since={},
        )
        text = update_cdp_metadata.build_manifest(
            paths=None,
            local_methods=[
                update_cdp_metadata.LocalMethod(
                    domain="Component", method="uselessUpdate", source="component.cc"
                ),
                update_cdp_metadata.LocalMethod(
                    domain="DOM", method="getDocument", source="dom.cc"
                ),
                update_cdp_metadata.LocalMethod(
                    domain="DOM", method="getDocumentWithBoxModel", source="dom.cc"
                ),
                update_cdp_metadata.LocalMethod(
                    domain="DOM", method="newMethod", source="dom.cc"
                ),
                update_cdp_metadata.LocalMethod(
                    domain="Lynx", method="getData", source="lynx.cc"
                ),
            ],
            scopes={
                "Component": "global-and-instance",
                "DOM": "instance",
                "Lynx": "instance",
            },
            upstream_methods={"DOM": {"getDocument"}},
            existing_manifest=existing_manifest,
            current_lynx_version="4.2",
        )

        self.assertIn(
            "\n  - name: Component\n"
            "    origin: lynx-extension\n"
            "    scope: global-and-instance\n",
            text,
        )
        self.assertIn(
            "\n  - name: DOM\n    origin: standard\n    scope: instance\n",
            text,
        )
        self.assertIn(
            "\n      - name: getDocument\n        origin: standard\n"
            "        since: '3.8'\n"
            "        source: dom.cc\n",
            text,
        )
        self.assertIn(
            "\n      - name: getDocumentWithBoxModel\n"
            "        origin: lynx-extension\n"
            "        since: '3.2'\n"
            "        source: dom.cc\n",
            text,
        )
        self.assertIn(
            "\n      - name: newMethod\n"
            "        origin: lynx-extension\n"
            "        since: '4.2'\n"
            "        source: dom.cc\n",
            text,
        )
        self.assertIn(
            "\n  - name: Lynx\n"
            "    origin: lynx-extension\n"
            "    scope: instance\n",
            text,
        )

    def test_build_manifest_rejects_existing_entry_without_since(self):
        existing_manifest = update_cdp_metadata.ExistingManifest(
            domains={"DOM"},
            methods={("DOM", "getDocument")},
            events=set(),
            method_since={},
            event_since={},
        )

        with self.assertRaisesRegex(
            update_cdp_metadata.MetadataError,
            r"existing manifest entry DOM\.getDocument is missing mandatory `since`",
        ):
            update_cdp_metadata.build_manifest(
                paths=None,
                local_methods=[
                    update_cdp_metadata.LocalMethod(
                        domain="DOM", method="getDocument", source="dom.cc"
                    )
                ],
                scopes={"DOM": "instance"},
                upstream_methods={"DOM": {"getDocument"}},
                existing_manifest=existing_manifest,
                current_lynx_version="4.2",
            )


if __name__ == "__main__":
    unittest.main()
