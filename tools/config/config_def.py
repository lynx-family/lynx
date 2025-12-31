# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

# /usr/bin/env python3
# -*- coding: utf-8 -*-

import yaml
import re
import sys
from pathlib import Path

_accounts_set = None
_accounts_mapping_path = Path(__file__).resolve().parents[3] / "accounts-mapping.yml"


class Config:
    _type_known_list = [
        "lepus::Value",
        "std::unordered_set<CSSPropertyID>",
        "base::Version",
        "FeOption",
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
        export: bool,
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
        self.support_platform = support_platform

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
        self.export = export

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
        if not (
            self.since and (isinstance(self.since, str) or isinstance(self.since, list))
        ):
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
        if not self._check_support_platform():
            return False
        if self.deprecated and not (
            isinstance(self.deprecated, str) or isinstance(self.deprecated, list)
        ):
            print(
                f"Config {self.name} deprecated field '{self.deprecated}' is invalid, please ensure it is not empty and configured as a string.",
                file=sys.stderr,
            )
            return False
        return True

    def _check_author(self) -> bool:
        global _accounts_set
        if _accounts_set is None:
            _accounts_set = set()
            if not _accounts_mapping_path.exists():
                print(
                    f"please ensure {_accounts_mapping_path} file exists.",
                    file=sys.stderr,
                )
            else:
                accounts_mapping = yaml.safe_load(_accounts_mapping_path.read_text())
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

    def _check_support_platform(self) -> bool:
        _support_platform_set = {"iOS", "Android", "HarmonyOS"}
        if not self.support_platform:
            return True
        for platform in self.support_platform:
            if platform not in _support_platform_set:
                print(
                    f"Config {self.name} supportPlatform field '{self.support_platform}' is invalid, please ensure it is in {list(_support_platform_set)}.",
                    file=sys.stderr,
                )
                return False
        return True
