# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

# /usr/bin/env python3
# -*- coding: utf-8 -*-

import copy
import yaml
import re
import sys
from jinja2 import Template
from config_utils import clang_format, sort_by_deprecated_and_alphabetical
import argparse
from config_def import Config
from config_env import (
    TEMPLATE_CODEC_PATH,
    BINARY_DECODER_PATH,
    CONFIG_YAML_PATH,
    LYNX_CONFIG_TOOLS_PATH,
    JINJA_TEMPLATES_PATH,
    JS_LIBRARIES_CONFIG_PATH,
    OLIVER_CONFIG_PATH,
)
from pathlib import Path


def _construct_config_object(key: str, value: dict) -> Config:
    return Config(
        name=key,
        desc=value.get("description", "NA"),
        default_value=value.get("defaultValue", None),
        js_default_value=value.get("jsDefaultValue", "undefined"),
        value_type=value.get("valueType", None),
        js_value_type=value.get("jsValueType", "undefined"),
        since=value.get("since", ""),
        deprecated=value.get("deprecated", []),
        support_platform=value.get("supportPlatform", ["Android", "iOS", "HarmonyOS"]),
        sync_to=value.get("syncTo", []),
        version_overrides=value.get("versionOverrides", []),
        author=value.get("author", None),
        code_gen=value.get("codeGen", ["ALL"]),
        name_as=value.get("nameAs", {}),
        bind_member_to=value.get("bindMemberTo", ""),
        read_settings=value.get("readSettings", False),
        read_native=value.get("readNative", False),
        export=value.get("export", True),
    )


def parse_config() -> tuple[list[Config], list[Config]]:
    config = yaml.safe_load(CONFIG_YAML_PATH.read_text())
    configs: list[Config] = []
    compiler_options: list[Config] = []
    for key, value in config.items():
        if key == "compilerOptions":
            compiler_options = [
                _construct_config_object(key, value) for key, value in value.items()
            ]
            continue
        configs.append(_construct_config_object(key, value))
    for config in configs:
        if not config.is_invalid():
            return []
    return configs, compiler_options


def _validate_export_version(items: list[Config]) -> list[Config]:
    if not items:
        return items

    def check_version(v_str):
        try:
            return [int(x) for x in v_str.split(".")] >= [3, 2]
        except Exception:
            return True

    items_copy = copy.deepcopy(items)
    for item in items_copy:
        if isinstance(item.since, str) and item.since:
            if not check_version(item.since):
                item.since = "3.2"
        elif isinstance(item.since, list):
            for i, sub in enumerate(item.since):
                for k, v in sub.items():
                    if not check_version(v):
                        item.since[i][k] = "3.2"
        else:
            item.since = "3.2"

        if isinstance(item.deprecated, str) and item.deprecated:
            if not check_version(item.deprecated):
                item.deprecated = "3.2"
        elif isinstance(item.deprecated, list):
            for i, sub in enumerate(item.deprecated):
                for k, v in sub.items():
                    if not check_version(v):
                        item.deprecated[i][k] = "3.2"
    return items_copy


def render_code_content(
    template_path: Path,
    output_path: Path,
    configs: list[Config] = None,
    options: list[Config] = None,
    export: bool = True,
):
    if not template_path.exists():
        print(f"{template_path} not found when gen config")
        sys.exit(1)
    lynx_config_tmpl = template_path.read_text()

    rendered_content = Template(
        lynx_config_tmpl, trim_blocks=True, lstrip_blocks=True
    ).render(configs=configs, options=options, export=export)
    if output_path.suffix == ".cc" or output_path.suffix == ".h":
        rendered_content = clang_format(rendered_content, file_extension=".h")

    if not output_path.exists():
        output_path.write_text(rendered_content)
    else:
        existing_content = output_path.read_text()
        if existing_content != rendered_content:
            output_path.write_text(rendered_content)
        else:
            print(f"No need to update {output_path}")


def _gen_page_config_decode(configs: list[Config]):
    config_decode_tmpl_path = BINARY_DECODER_PATH / "lynx_config_decoder.tmpl"
    lynx_config_decoder_header_path = BINARY_DECODER_PATH / "lynx_config_decoder.h"
    render_code_content(
        config_decode_tmpl_path, lynx_config_decoder_header_path, configs
    )


def _gen_lynx_config_constants(configs: list[Config]):
    lynx_config_header_tmpl_path = BINARY_DECODER_PATH / "lynx_config_header.tmpl"
    lynx_config_header_path = BINARY_DECODER_PATH / "lynx_config_auto_gen.h"
    render_code_content(lynx_config_header_tmpl_path, lynx_config_header_path, configs)

    lynx_config_cc_tmpl_path = BINARY_DECODER_PATH / "lynx_config_cc.tmpl"
    lynx_config_header_path = BINARY_DECODER_PATH / "lynx_config_auto_gen.cc"
    render_code_content(lynx_config_cc_tmpl_path, lynx_config_header_path, configs)

    config_const_tmpl_path = BINARY_DECODER_PATH / "lynx_config_constant.tmpl"
    lynx_config_const_header_path = (
        BINARY_DECODER_PATH / "lynx_config_constant_auto_gen.h"
    )
    render_code_content(config_const_tmpl_path, lynx_config_const_header_path, configs)


def _gen_config_types(configs: list[Config], export_configs: list[Config]):
    config_types_tmpl_path = JINJA_TEMPLATES_PATH / "config_types.tmpl"
    config_types_header_path = JS_LIBRARIES_CONFIG_PATH / "types" / "config.d.ts"

    render_code_content(
        config_types_tmpl_path,
        config_types_header_path,
        sort_by_deprecated_and_alphabetical(export_configs),
        export=True,
    )

    config_types_header_path = OLIVER_CONFIG_PATH / "types" / "config.d.ts"
    if config_types_header_path.exists():
        render_code_content(
            config_types_tmpl_path,
            config_types_header_path,
            sort_by_deprecated_and_alphabetical(configs),
            export=False,
        )


def _gen_compile_options(options: list[Config]):
    compile_options_header_path = TEMPLATE_CODEC_PATH / "compile_options.h"

    template_content = """{% for option in options %}
    {% if "NONE" not in option.codeGen %}
      {{ option.value_type }} {{ option.member_name }}{{ '{' }}{{ option.default_value }}{{ '}' }};
    {% endif %}
    {% endfor %}
    """

    rendered_content = Template(
        template_content, trim_blocks=True, lstrip_blocks=True
    ).render(options=options)

    header_content = compile_options_header_path.read_text()

    pattern = r"(\s*// Compile options auto generated start\s*)(.*?)(^\s*// Compile options auto generated end\s*)"
    replacement = r"\1" + rendered_content + r"\3"
    new_header_content, num_replacements = re.subn(
        pattern, replacement, header_content, flags=re.DOTALL | re.MULTILINE
    )

    if num_replacements == 0:
        print(f"Markers not found in {compile_options_header_path}", file=sys.stderr)
        return

    new_header_content = clang_format(new_header_content, file_extension=".h")
    if new_header_content != header_content:
        compile_options_header_path.write_text(new_header_content)
    else:
        print(f"No need to update {compile_options_header_path}")


def _gen_compile_options_types(options: list[Config], export_options: list[Config]):
    compile_options_types_tmpl_path = (
        JINJA_TEMPLATES_PATH / "compiler_options_types.tmpl"
    )
    config_types_header_path = (
        JS_LIBRARIES_CONFIG_PATH / "types" / "compiler-options.d.ts"
    )

    global _compile_options
    render_code_content(
        compile_options_types_tmpl_path,
        config_types_header_path,
        None,
        sort_by_deprecated_and_alphabetical(export_options),
    )

    config_types_header_path = OLIVER_CONFIG_PATH / "types" / "compiler-options.d.ts"
    if config_types_header_path.exists():
        render_code_content(
            compile_options_types_tmpl_path,
            config_types_header_path,
            None,
            sort_by_deprecated_and_alphabetical(options),
            export=False,
        )


def _prepare_config_doc(configs: list[Config]):
    platform_doc_map = {
        "Android": "<AndroidOnly /> ",
        "iOS": "<IOSOnly /> ",
        "HarmonyOS": "<HarmonyOnly /> ",
    }
    platform_badge_map = {
        "Android": "android",
        "iOS": "ios",
        "HarmonyOS": "harmony",
    }

    def _get_badge_doc(data):
        if isinstance(data, list):
            doc_parts = []
            for item in data:
                for platform, badge_name in platform_badge_map.items():
                    if platform in item:
                        version = item[platform]
                        doc_parts.append(
                            f'<PlatformBadge platform="{badge_name}" version="{version}" />'
                        )
                        break
            return "".join(doc_parts)
        elif isinstance(data, str):
            return f"<VersionBadge v={{{data}}}/>"
        return ""

    for config in configs:
        config.support_platform_doc = "".join(
            platform_doc_map.get(platform, "") for platform in config.support_platform
        )
        config.since_doc = _get_badge_doc(config.since)
        config.deprecated_doc = _get_badge_doc(config.deprecated)


def gen_config_doc(
    configs: list[Config], internal_path: Path = None, external_path: Path = None
):
    _prepare_config_doc(configs)
    export_configs = _validate_export_version(configs)
    _prepare_config_doc(export_configs)
    config_doc_tmpl_path = JINJA_TEMPLATES_PATH / "lynx_config_doc.tmpl"
    if internal_path and internal_path.exists():
        internal_doc_path = (
            internal_path
            / "internal_docs"
            / "en"
            / "api"
            / "lynx-config"
            / "config-reference.mdx"
        )
    else:
        print(
            f"Internal path {internal_path} not found. Use default path {LYNX_CONFIG_TOOLS_PATH}"
        )
        internal_doc_path = LYNX_CONFIG_TOOLS_PATH / "config-reference.mdx"
    if external_path and external_path.exists():
        external_doc_path = (
            external_path
            / "docs"
            / "en"
            / "api"
            / "lynx-config"
            / "config-reference.mdx"
        )
    else:
        print(
            f"External path {external_path} not found. Use default path {LYNX_CONFIG_TOOLS_PATH / 'config-reference.mdx'}"
        )
        external_doc_path = LYNX_CONFIG_TOOLS_PATH / "config-reference.mdx"

    render_code_content(
        config_doc_tmpl_path,
        internal_doc_path,
        sort_by_deprecated_and_alphabetical(configs),
        None,
        export=False,
    )

    render_code_content(
        config_doc_tmpl_path,
        external_doc_path,
        sort_by_deprecated_and_alphabetical(export_configs),
        None,
        export=True,
    )


def _gen_config_keys(
    configs: list[Config],
    export_configs: list[Config],
    options: list[Config],
    export_options: list[Config],
):
    config_keys_tmpl_path = JINJA_TEMPLATES_PATH / "config_keys.tmpl"
    config_keys_header_path = JS_LIBRARIES_CONFIG_PATH / "config-keys.js"

    render_code_content(
        config_keys_tmpl_path,
        config_keys_header_path,
        sort_by_deprecated_and_alphabetical(export_configs),
        sort_by_deprecated_and_alphabetical(export_options),
    )

    config_keys_header_path = OLIVER_CONFIG_PATH / "config-keys.js"
    if config_keys_header_path.exists():
        render_code_content(
            config_keys_tmpl_path,
            config_keys_header_path,
            sort_by_deprecated_and_alphabetical(configs),
            sort_by_deprecated_and_alphabetical(options),
            export=False,
        )


def gen_lynx_config(configs: list[Config], options: list[Config]):
    # gen page config decode
    _gen_page_config_decode(configs)
    # gen lynx config constants
    _gen_lynx_config_constants(configs)
    # gen compile options
    _gen_compile_options(options)


def gen_types(configs: list[Config], options: list[Config]):
    export_configs = _validate_export_version(configs)
    export_options = _validate_export_version(options)
    # gen config types
    _gen_config_types(configs, export_configs)
    # gen compile options types
    _gen_compile_options_types(options, export_options)
    # gen config keys
    _gen_config_keys(configs, export_configs, options, export_options)


def main():
    arg_parser = argparse.ArgumentParser()
    arg_parser.add_argument("--gen-lynx-config", default=True, action="store_true")
    arg_parser.add_argument("--gen-config-types", action="store_true")

    subparsers = arg_parser.add_subparsers(dest="command")
    doc_parser = subparsers.add_parser("gen-config-doc")
    doc_parser.add_argument("--internal-path", type=Path)
    doc_parser.add_argument("--external-path", type=Path)

    args = arg_parser.parse_args()

    configs, options = parse_config()
    if not configs:
        sys.exit(-1)

    if args.gen_lynx_config:
        # gen lynx config
        gen_lynx_config(configs, options)
        # gen lynx types npm
        gen_types(configs, options)

    if args.command == "gen-config-doc":
        # gen config doc
        gen_config_doc(configs + options, args.internal_path, args.external_path)
    sys.exit(0)


if __name__ == "__main__":
    main()
