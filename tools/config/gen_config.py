# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

# /usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import yaml
import re
import sys
from jinja2 import Template
from config_utils import clang_format


class Config:
    _type_known_list = ["PackageInstanceBundleModuleMode", "PackageInstanceDSL"]

    def __init__(
        self,
        name: str,
        desc: str,
        default_value,
        value_type: str,
        sync_to: list[str],
        version_overrides: list[dict],
        author: str,
        code_gen: list[str],
        name_as: dict[str],
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

        self.desc = desc
        self.default_value = default_value
        self.value_type = value_type
        self.sync_to = sync_to
        self.setter_input_type = self.value_type

        if self.value_type == "bool" or self.value_type == "boolean":
            self.value_type = "bool"
            self.default_value = "true" if self.default_value else "false"
        elif self.value_type == "string":
            self.value_type = "std::string"
            self.setter_input_type = "const std::string&"
            if not self.default_value:
                self.default_value = '""'
            else:
                self.default_value = '"' + self.default_value + '"'

        if self.default_value is None:
            self.default_value = ""

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


_binary_decoder_path = os.path.abspath(
    os.path.join(
        os.path.dirname(os.path.abspath(__file__)),
        os.pardir,
        os.pardir,
        "core",
        "template_bundle",
        "template_codec",
        "binary_decoder",
    )
)
_config_yaml_path = os.path.join(_binary_decoder_path, "lynx_config.yml")


def parse_config() -> list[Config]:
    with open(_config_yaml_path, "r") as f:
        config = yaml.safe_load(f)
    configs = []
    for key, value in config.items():
        version_overrides: list[dict] = value.get("versionOverrides")
        configs.append(
            Config(
                key,
                value["description"],
                value["defaultValue"],
                value["valueType"],
                value.get("syncTo"),
                version_overrides,
                value.get("author"),
                value.get("codeGen"),
                value.get("nameAs"),
            )
        )
    return configs


def gen_page_config_decode():
    configs = parse_config()
    config_decode_tmpl_path = os.path.join(
        _binary_decoder_path,
        "lynx_config_decoder.tmpl",
    )
    if not os.path.exists(config_decode_tmpl_path):
        print(f"{config_decode_tmpl_path} not found when gen lynx config decoder")
        sys.exit(1)

    lynx_config_decoder_header_path = os.path.join(
        _binary_decoder_path,
        "lynx_config_decoder.h",
    )
    if os.path.exists(lynx_config_decoder_header_path):
        os.remove(lynx_config_decoder_header_path)
    with open(config_decode_tmpl_path, "r") as f:
        lynx_config_tmpl = f.read()
    with open(lynx_config_decoder_header_path, "w") as f:
        f.write(
            Template(lynx_config_tmpl, trim_blocks=True, lstrip_blocks=True).render(
                configs=configs
            )
        )
    clang_format(lynx_config_decoder_header_path)


def gen_lynx_config():
    configs = parse_config()
    lynx_config_tmpl_path = os.path.join(
        _binary_decoder_path,
        "lynx_config.tmpl",
    )
    if not os.path.exists(lynx_config_tmpl_path):
        print(f"{lynx_config_tmpl_path} not found when gen lynx config")
        sys.exit(1)

    lynx_config_header_path = os.path.join(
        _binary_decoder_path,
        "lynx_config_auto_gen.h",
    )
    if os.path.exists(lynx_config_header_path):
        os.remove(lynx_config_header_path)
    with open(lynx_config_tmpl_path, "r") as f:
        lynx_config_tmpl = f.read()
    with open(lynx_config_header_path, "w") as f:
        f.write(
            Template(lynx_config_tmpl, trim_blocks=True, lstrip_blocks=True).render(
                configs=configs
            )
        )
    clang_format(lynx_config_header_path)


def gen_config():
    # gen page config decode
    gen_page_config_decode()
    # gen lynx config constants
    gen_lynx_config()


if __name__ == "__main__":
    gen_config()
