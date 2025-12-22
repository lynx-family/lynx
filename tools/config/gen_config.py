# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

# /usr/bin/env python3
# -*- coding: utf-8 -*-

import glob
import os
import yaml
import re
import sys
from jinja2 import Template
from config_utils import clang_format, sort_by_deprecated_and_alphabetical
import argparse

_accounts_set = None
_accounts_mapping_path = os.path.normpath(
    os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        os.path.pardir,
        os.path.pardir,
        os.path.pardir,
        "accounts-mapping.yml",
    )
)


class Config:
    _type_known_list = [
        "lepus::Value",
        "std::unordered_set<CSSPropertyID>",
        "base::Version",
    ]

    def __init__(
        self,
        name: str,
        desc: str,
        default_value: str,
        js_default_value: str,
        value_type: str,
        js_value_type: str,
        since: str,
        deprecated: str,
        support_platform: str,
        sync_to: list[str],
        version_overrides: list[dict],
        author: str,
        code_gen: list[str],
        name_as: dict[str],
        bind_member_to: str,
        read_settings: bool,
        read_native: bool,
    ):
        self.name = name
        self.upper_camel_case_name = f"{name[0].upper()}{name[1:]}"
        self.setter_func_name = f"Set{self.upper_camel_case_name}"
        self.getter_func_name = f"Get{self.upper_camel_case_name}"
        self.snake_case_name = re.sub(
            r"(?<=[a-z])(?=[A-Z])|(?<=[A-Z])(?=[A-Z][a-z])", "_", name
        ).lower()
        self.const_name = f"k{self.upper_camel_case_name}"
        if name_as is not None:
            self.snake_case_name = (
                name_as.get("member") if name_as.get("member") else self.snake_case_name
            )
            self.setter_func_name = (
                name_as.get("setter")
                if name_as.get("setter")
                else self.setter_func_name
            )
            self.getter_func_name = (
                name_as.get("getter")
                if name_as.get("getter")
                else self.getter_func_name
            )
            self.const_name = (
                name_as.get("const") if name_as.get("const") else self.const_name
            )

        self.member_name = (
            bind_member_to if bind_member_to else f"{self.snake_case_name}_"
        )
        self.desc = desc
        self.default_value = default_value
        self.value_type = value_type
        self.sync_to = sync_to
        self.setter_input_type = self.value_type
        self.js_default_value = js_default_value
        self.js_value_type = js_value_type
        self.since = since
        self.deprecated = deprecated
        self.support_platform = ", ".join(support_platform)

        if self.value_type == "bool" or self.value_type == "boolean":
            self.value_type = "bool"
            self.js_value_type = "boolean"
            self.js_default_value = self.default_value
        elif self.value_type == "string":
            self.value_type = "std::string"
            self.setter_input_type = "const std::string&"
            self.js_value_type = "string"
            if not self.default_value:
                self.default_value = '""'
                self.js_default_value = '""'
            else:
                self.default_value = '"' + self.default_value + '"'
                self.js_default_value = self.default_value
        elif self.value_type == "TernaryBool":
            self.js_value_type = "boolean"
        elif not self.js_value_type:
            self.js_value_type = "undefined"

        if self.default_value is None:
            self.default_value = ""
            self.js_default_value = "undefined"

        self.doc_type = None
        if self.value_type == "bool" or self.value_type == "TernaryBool":
            self.doc_type = "Bool"
        elif self.value_type == "std::string":
            self.doc_type = "String"
        elif self.value_type == "int32_t":
            self.doc_type = "Int"
        elif self.value_type == "int64_t":
            self.doc_type = "Int64"
        elif self.value_type == "double":
            self.doc_type = "Double"
        elif self.value_type == "uint8_t":
            self.doc_type = "Int"
        elif self.value_type == "uint32_t":
            self.doc_type = "Uint"
        elif self.value_type == "uint64_t":
            self.doc_type = "Uint64"
        elif self.value_type not in self._type_known_list:
            print(f"Document unsupported type: {self.value_type}")

        self.version_overrides = version_overrides
        self.author = author
        self.codeGen = code_gen if code_gen is not None else ["ALL"]
        self.read_settings = read_settings
        self.read_native = read_native

    def is_invalid(self):
        if self.deprecated:
            return True
        if not (self.desc and isinstance(self.desc, str)):
            print(
                f"Config {self.name} config description field '{self.desc}' is invalid, please ensure it is not empty and configured as a string.",
                file=sys.stderr,
            )
            return False
        if not isinstance(self.default_value, str):
            print(
                f"Config {self.name} defaultValue field '{self.default_value}' is invalid, please configured as a string.",
                file=sys.stderr,
            )
            return False
        if not (self.value_type and isinstance(self.value_type, str)):
            print(
                f"Config {self.name} valueType field '{self.value_type}' is invalid, please ensure it is not empty and configured as a string.",
                file=sys.stderr,
            )
            return False
        if not (self.since and isinstance(self.since, str)):
            print(
                f"Config {self.name} since field '{self.since}' is invalid, please ensure it is not empty and configured as a string.",
                file=sys.stderr,
            )
            return False
        if not (self.author and isinstance(self.author, str) and self._check_author()):
            print(
                f"Config {self.name} author field '{self.author}' is invalid, please ensure it is not empty and configured as a string.",
                file=sys.stderr,
            )
            return False
        return True

    def _check_author(self) -> bool:
        global _accounts_set
        if _accounts_set is None:
            _accounts_set = set()
            if not os.path.exists(_accounts_mapping_path):
                print(
                    f"please ensure {_accounts_mapping_path} file exists.",
                    file=sys.stderr,
                )
            else:
                with open(_accounts_mapping_path, "r") as f:
                    accounts_mapping = yaml.safe_load(f)
                    for account in accounts_mapping.get("mappings"):
                        _accounts_set.add(account.get("external_username"))
        if not _accounts_set or self.author in _accounts_set:
            return True
        else:
            print(
                f"Config {self.name} author field '{self.author}' is invalid, please ensure it is in the {_accounts_mapping_path} file.",
                file=sys.stderr,
            )
            return False


_compile_options: list[Config] = None
_template_codec_path = os.path.abspath(
    os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        os.pardir,
        os.pardir,
        "core",
        "template_bundle",
        "template_codec",
    )
)
_binary_decoder_path = os.path.join(_template_codec_path, "binary_decoder")
_config_yaml_path = os.path.join(_binary_decoder_path, "lynx_config.yml")


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
    )


def parse_config() -> list[Config]:
    with open(_config_yaml_path, "r") as f:
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
    template_path: str,
    output_path: str,
    configs: list[Config],
    options: list[Config] = None,
):
    if not os.path.exists(template_path):
        print(f"{template_path} not found when gen config")
        sys.exit(1)
    with open(template_path, "r") as f:
        lynx_config_tmpl = f.read()

    rendered_content = Template(
        lynx_config_tmpl, trim_blocks=True, lstrip_blocks=True
    ).render(configs=configs, options=options)
    if output_path.endswith(".cc") or output_path.endswith(".h"):
        rendered_content = clang_format(rendered_content, file_extension=".h")

    if not os.path.exists(output_path):
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


def gen_page_config_decode(configs: list[Config]):
    config_decode_tmpl_path = os.path.join(
        _binary_decoder_path,
        "lynx_config_decoder.tmpl",
    )

    lynx_config_decoder_header_path = os.path.join(
        _binary_decoder_path,
        "lynx_config_decoder.h",
    )
    render_code_content(
        config_decode_tmpl_path, lynx_config_decoder_header_path, configs
    )


def gen_lynx_config(configs: list[Config]):
    lynx_config_header_tmpl_path = os.path.join(
        _binary_decoder_path,
        "lynx_config_header.tmpl",
    )

    lynx_config_header_path = os.path.join(
        _binary_decoder_path,
        "lynx_config_auto_gen.h",
    )
    render_code_content(lynx_config_header_tmpl_path, lynx_config_header_path, configs)

    lynx_config_cc_tmpl_path = os.path.join(
        _binary_decoder_path,
        "lynx_config_cc.tmpl",
    )

    lynx_config_header_path = os.path.join(
        _binary_decoder_path,
        "lynx_config_auto_gen.cc",
    )
    render_code_content(lynx_config_cc_tmpl_path, lynx_config_header_path, configs)

    config_const_tmpl_path = os.path.join(
        _binary_decoder_path,
        "lynx_config_constant.tmpl",
    )
    lynx_config_const_header_path = os.path.join(
        _binary_decoder_path,
        "lynx_config_constant_auto_gen.h",
    )
    render_code_content(config_const_tmpl_path, lynx_config_const_header_path, configs)


def gen_config_types(configs: list[Config]):
    config_types_tmpl_path = os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        "config_types.tmpl",
    )

    config_types_header_path = os.path.normpath(
        os.path.join(
            os.path.dirname(os.path.abspath(__file__)),
            os.path.pardir,
            os.path.pardir,
            "js_libraries",
            "type-config",
            "types",
            "config.d.ts",
        )
    )

    render_code_content(
        config_types_tmpl_path,
        config_types_header_path,
        sort_by_deprecated_and_alphabetical(configs),
    )


def gen_compile_options():
    compile_options_header_path = os.path.join(
        _template_codec_path, "compile_options.h"
    )

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


def gen_compile_options_types():
    compile_options_types_tmpl_path = os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        "compiler_options_types.tmpl",
    )

    config_types_header_path = os.path.normpath(
        os.path.join(
            os.path.dirname(os.path.abspath(__file__)),
            os.path.pardir,
            os.path.pardir,
            "js_libraries",
            "type-config",
            "types",
            "compiler-options.d.ts",
        )
    )

    global _compile_options
    render_code_content(
        compile_options_types_tmpl_path,
        config_types_header_path,
        sort_by_deprecated_and_alphabetical(_compile_options),
    )


def gen_config_doc(configs: list[Config]):
    config_doc_tmpl_path = os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        "lynx_config_doc.tmpl",
    )
    config_doc_header_path = os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        "lynx_config_doc.mdx",
    )

    render_code_content(
        config_doc_tmpl_path,
        config_doc_header_path,
        sort_by_deprecated_and_alphabetical(configs),
    )


def gen_config_keys(configs: list[Config]):
    config_keys_tmpl_path = os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        "config_keys.tmpl",
    )

    config_keys_header_path = os.path.normpath(
        os.path.join(
            os.path.dirname(os.path.abspath(__file__)),
            os.path.pardir,
            os.path.pardir,
            "js_libraries",
            "type-config",
            "config-keys.js",
        )
    )

    global _compile_options
    render_code_content(
        config_keys_tmpl_path,
        config_keys_header_path,
        sort_by_deprecated_and_alphabetical(configs),
        sort_by_deprecated_and_alphabetical(_compile_options),
    )


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
        # gen page config decode
        gen_page_config_decode(configs)
        # gen lynx config constants
        gen_lynx_config(configs)
        # gen compile options
        gen_compile_options()
        # gen config types
        gen_config_types(configs)
        # gen compile options types
        gen_compile_options_types()
        # gen config keys
        gen_config_keys(configs)
    if args.gen_config_doc:
        # gen config doc
        gen_config_doc(configs)
    sys.exit(0)


if __name__ == "__main__":
    main()
