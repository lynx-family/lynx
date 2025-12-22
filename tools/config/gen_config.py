# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

# /usr/bin/env python3
# -*- coding: utf-8 -*-

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


_compile_options: list[Config] = None


def _construct_config_object(key: str, value: dict) -> Config:
    return Config(
        name=key,
        desc=value.get("description", None),
        default_value=value.get("defaultValue", None),
        js_default_value=value.get("jsDefaultValue", "undefined"),
        value_type=value.get("valueType", None),
        js_value_type=value.get("jsValueType", "undefined"),
        since=value.get("since", None),
        deprecated=value.get("deprecated", ""),
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


def parse_config() -> list[Config]:
    with open(CONFIG_YAML_PATH, "r") as f:
        config = yaml.safe_load(f)
    configs: list[Config] = []
    for key, value in config.items():
        if key == "compilerOptions":
            global _compile_options
            _compile_options = [
                _construct_config_object(key, value) for key, value in value.items()
            ]
            continue
        configs.append(_construct_config_object(key, value))
    for config in configs:
        if not config.is_invalid():
            return []
    return configs


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
    with open(template_path, "r") as f:
        lynx_config_tmpl = f.read()

    rendered_content = Template(
        lynx_config_tmpl, trim_blocks=True, lstrip_blocks=True
    ).render(configs=configs, options=options, export=export)
    if str(output_path).endswith(".cc") or str(output_path).endswith(".h"):
        rendered_content = clang_format(rendered_content, file_extension=".h")

    if not output_path.exists():
        with open(output_path, "w") as f:
            f.write(rendered_content)
    else:
        with open(output_path, "r") as f:
            existing_content = f.read()
        if existing_content != rendered_content:
            with open(output_path, "w") as f:
                f.write(rendered_content)
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


def _gen_config_types(configs: list[Config]):
    config_types_tmpl_path = JINJA_TEMPLATES_PATH / "config_types.tmpl"
    config_types_header_path = JS_LIBRARIES_CONFIG_PATH / "types" / "config.d.ts"

    render_code_content(
        config_types_tmpl_path,
        config_types_header_path,
        sort_by_deprecated_and_alphabetical(configs),
    )

    config_types_header_path = OLIVER_CONFIG_PATH / "types" / "config.d.ts"
    if config_types_header_path.exists():
        render_code_content(
            config_types_tmpl_path,
            config_types_header_path,
            sort_by_deprecated_and_alphabetical(configs),
            export=False,
        )


def _gen_compile_options():
    compile_options_header_path = TEMPLATE_CODEC_PATH / "compile_options.h"

    template_content = """{% for option in options %}
    {% if "NONE" not in option.codeGen %}
      {{ option.value_type }} {{ option.member_name }}{{ '{' }}{{ option.default_value }}{{ '}' }};
    {% endif %}
    {% endfor %}
    """

    global _compile_options
    if _compile_options is None:
        print("Compile options not parsed. Run parse_config first.", file=sys.stderr)
        return

    rendered_content = Template(
        template_content, trim_blocks=True, lstrip_blocks=True
    ).render(options=_compile_options)

    with open(compile_options_header_path, "r") as f:
        header_content = f.read()

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
        with open(compile_options_header_path, "w") as f:
            f.write(new_header_content)
    else:
        print(f"No need to update {compile_options_header_path}")


def _gen_compile_options_types():
    compile_options_types_tmpl_path = (
        JINJA_TEMPLATES_PATH / "compiler_options_types.tmpl"
    )
    config_types_header_path = JS_LIBRARIES_CONFIG_PATH / "types" / "compiler-options.d.ts"

    global _compile_options
    render_code_content(
        compile_options_types_tmpl_path,
        config_types_header_path,
        None,
        sort_by_deprecated_and_alphabetical(_compile_options),
    )

    config_types_header_path = OLIVER_CONFIG_PATH / "types" / "compiler-options.d.ts"
    if config_types_header_path.exists():
        render_code_content(
            compile_options_types_tmpl_path,
            config_types_header_path,
            None,
            sort_by_deprecated_and_alphabetical(_compile_options),
            export=False,
        )


def gen_config_doc(configs: list[Config]):
    config_doc_tmpl_path = JINJA_TEMPLATES_PATH / "lynx_config_doc.tmpl"
    config_doc_header_path = LYNX_CONFIG_TOOLS_PATH / "lynx_config_doc.mdx"

    render_code_content(
        config_doc_tmpl_path,
        config_doc_header_path,
        sort_by_deprecated_and_alphabetical(configs),
    )


def _gen_config_keys(configs: list[Config]):
    config_keys_tmpl_path = JINJA_TEMPLATES_PATH / "config_keys.tmpl"
    config_keys_header_path = JS_LIBRARIES_CONFIG_PATH / "config-keys.js"


    global _compile_options
    render_code_content(
        config_keys_tmpl_path,
        config_keys_header_path,
        sort_by_deprecated_and_alphabetical(configs),
        sort_by_deprecated_and_alphabetical(_compile_options),
    )

    config_keys_header_path = OLIVER_CONFIG_PATH / "config-keys.js"
    if config_keys_header_path.exists():
        render_code_content(
            config_keys_tmpl_path,
            config_keys_header_path,
            sort_by_deprecated_and_alphabetical(configs),
            sort_by_deprecated_and_alphabetical(_compile_options),
            export=False,
        )


def gen_lynx_config(configs: list[Config]):
    # gen page config decode
    _gen_page_config_decode(configs)
    # gen lynx config constants
    _gen_lynx_config_constants(configs)
    # gen compile options
    _gen_compile_options()


def gen_types(configs: list[Config]):
    # gen config types
    _gen_config_types(configs)
    # gen compile options types
    _gen_compile_options_types()
    # gen config keys
    _gen_config_keys(configs)


def main():
    arg_parser = argparse.ArgumentParser()
    arg_parser.add_argument("--gen-lynx-config", default=True, action="store_true")
    arg_parser.add_argument("--gen-config-types", action="store_true")
    arg_parser.add_argument("--gen-config-doc", action="store_true")
    args = arg_parser.parse_args()

    configs: list[Config] = parse_config()
    if not configs:
        sys.exit(-1)

    if args.gen_lynx_config:
        # gen lynx config
        gen_lynx_config(configs)
        # gen lynx types npm
        gen_types(configs)

    if args.gen_config_doc:
        # gen config doc
        gen_config_doc(configs)
    sys.exit(0)


if __name__ == "__main__":
    main()
