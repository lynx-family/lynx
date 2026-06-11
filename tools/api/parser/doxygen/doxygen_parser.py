# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

# /usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
from doxmlparser import index as index_parser
from doxmlparser import compound as compound_parser
from env_setup import ANDROID_API_PATH, IOS_API_PATH
from api_writer import write_api_metadata
from .doxygen_config import DoxygenConfig
from .doxmlparser_etree_compat import patch_doxmlparser_etree
from . import compounddef_parse

patch_doxmlparser_etree(index_parser, compound_parser)


class DoxygenParser:
    def __init__(self, platform):
        self.platform = platform
        self.api_path = os.path.normpath(
            os.path.join(os.path.dirname(__file__), os.pardir, os.pardir)
        )
        self.api_file = os.path.join(
            ANDROID_API_PATH if platform == "android" else IOS_API_PATH,
            f"lynx_{platform}.api",
        )

    def generate_xml(self) -> bool:
        doxygen_config = DoxygenConfig(
            self.platform, enable_generate_xml=True, enable_generate_html=False
        )
        return doxygen_config.execute(self.api_path, self.platform)

    def generate_html(self) -> bool:
        doxygen_config = DoxygenConfig(
            self.platform, enable_generate_xml=False, enable_generate_html=True
        )
        return doxygen_config.execute(self.api_path, self.platform)

    def parse(self):
        if not self.generate_xml():
            print("generate xml failed")
            return None
        xml_path = os.path.join(self.api_path, self.platform, "xml", "index.xml")
        root_object = index_parser.parse(xml_path, silence=True, print_warnings=False)

        object_list = [
            obj
            for compound in root_object.get_compound()
            for obj in self._parse_compound(compound)
        ]
        return object_list

    def dump(self) -> bool:
        try:
            object_list = self.parse()
        except Exception as e:
            print(f"parse {self.platform} api metadata failed: {e}", file=sys.stderr)
            print(f"Kept existing API metadata file: {self.api_file}", file=sys.stderr)
            return False
        if object_list is None:
            print(f"Kept existing API metadata file: {self.api_file}", file=sys.stderr)
            return False
        return write_api_metadata(self.api_file, object_list, self.platform)

    def _parse_compound(self, compound):
        xml_path = os.path.join(
            self.api_path, self.platform, "xml", f"{compound.get_refid()}.xml"
        )
        root_object = compound_parser.parse(xml_path, silence=True, print_warnings=False)
        for compounddef in root_object.get_compounddef():
            if compounddef.get_prot() not in [
                compound_parser.DoxProtectionKind.PUBLIC,
                None,
            ]:
                # We only care about public api.
                continue
            object = compounddef_parse.parse(compounddef)
            if object:
                yield object


if __name__ == "__main__":
    # For testing
    sys.path.append(
        os.path.join(os.path.dirname(__file__), os.path.pardir, os.path.pardir)
    )
    parser = DoxygenParser("ios")
    parser.dump()
