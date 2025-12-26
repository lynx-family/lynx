# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

# /usr/bin/env python3
# -*- coding: utf-8 -*-

from pathlib import Path

ROOT_PATH = Path(__file__).resolve().parents[2]
TEMPLATE_CODEC_PATH = ROOT_PATH / "core" / "template_bundle" / "template_codec"
BINARY_DECODER_PATH = TEMPLATE_CODEC_PATH / "binary_decoder"
CONFIG_YAML_PATH = BINARY_DECODER_PATH / "lynx_config.yml"
LYNX_CONFIG_TOOLS_PATH = Path(__file__).resolve().parent
JINJA_TEMPLATES_PATH = LYNX_CONFIG_TOOLS_PATH / "templates"
JS_LIBRARIES_CONFIG_PATH = ROOT_PATH / "js_libraries" / "type-config"
OLIVER_CONFIG_PATH = ROOT_PATH.parent / "oliver" / "type-config"
