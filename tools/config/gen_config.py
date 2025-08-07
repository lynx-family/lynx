# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

# /usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import yaml
import re
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
        codeGen: list[str],
    ):
        self.name = name
        self.upper_camel_case_name = f"{name[0].upper()}{name[1:]}"
        self.snake_case_name = re.sub(
            r"(?<=[a-z])(?=[A-Z])|(?<=[A-Z])(?=[A-Z][a-z])", "_", name
        ).lower()
        self.const_name = f"k{self.upper_camel_case_name}"
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
        self.codeGen = codeGen if codeGen is not None else ["ALL"]


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
_config_yaml_path = os.path.join(_binary_decoder_path, "config.yml")


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
                value["syncTo"],
                version_overrides,
                value.get("author"),
                value.get("codeGen"),
            )
        )
    return configs


def gen_native_members_config(configs: list[Config], page_config_path: str):
    native_config_members_tmpl = """
{% for config in configs %}
{% if "ALL" in config.codeGen or "MEMBER" in config.codeGen %}
  {{ config.value_type }} {{ config.snake_case_name }}_{{ '{' }}{{ config.default_value }}{{ '}' }};
{% endif %}
{% endfor %}
"""
    start_marker = "// BEGIN CONFIG MEMBER GEN"
    end_marker = "// END CONFIG MEMBER GEN"
    gen_code = Template(
        native_config_members_tmpl, trim_blocks=True, lstrip_blocks=True
    ).render(configs=configs)
    replace_comments_with_code(start_marker, end_marker, gen_code, page_config_path)


def gen_native_get_set_func_config(configs: list[Config], page_config_path: str):
    native_config_get_set_func_tmpl = """
{% for config in configs %}
{% if "ALL" in config.codeGen or "MEMBER" in config.codeGen %}
  inline void Set{{ config.upper_camel_case_name }}({{ config.setter_input_type }} {{ config.snake_case_name }}) { {{ config.snake_case_name }}_ = {{ config.snake_case_name }}; }
  inline {{ config.value_type }} Get{{ config.upper_camel_case_name }}() const { return {{ config.snake_case_name }}_; }

{% endif %}
{% endfor %}
"""
    start_marker = "// BEGIN CONFIG GET ADN SET FUNC GEN"
    end_marker = "// END CONFIG GET ADN SET FUNC GEN"
    gen_code = Template(
        native_config_get_set_func_tmpl, trim_blocks=True, lstrip_blocks=True
    ).render(configs=configs)
    replace_comments_with_code(start_marker, end_marker, gen_code, page_config_path)


def replace_comments_with_code(
    start_marker: str, end_marker: str, gen_code: str, page_config_path: str
):
    with open(page_config_path, "r") as f:
        content = f.read()
    pattern = re.compile(
        r"(?<={})(.*?)(?={})".format(start_marker, end_marker),
        re.DOTALL,
    )
    content = re.sub(pattern, gen_code, content)
    with open(page_config_path, "w") as f:
        f.write(content)


def gen_native_config():
    configs = parse_config()
    page_config_path = os.path.join(
        _binary_decoder_path,
        "page_config.h",
    )
    gen_native_members_config(configs, page_config_path)
    gen_native_get_set_func_config(configs, page_config_path)
    clang_format(page_config_path)


def gen_page_config_decode():
    configs = parse_config()
    config_decode_path = os.path.join(
        _binary_decoder_path,
        "lynx_binary_config_decoder.cc",
    )
    config_decode_tmpl = """
{% for config in configs %}
{% if "ALL" in config.codeGen or "DECODE" in config.codeGen %}
  if (doc.HasMember(config::{{ config.const_name }}) && doc[config::{{ config.const_name }}].Is{{ config.doc_type }}()) {
    {% if config.value_type == "TernaryBool" %}
    page_config->Set{{ config.upper_camel_case_name }}(doc[config::{{ config.const_name }}].GetBool() ? TernaryBool::TRUE_VALUE : TernaryBool::FALSE_VALUE);
    {% else %}
    page_config->Set{{ config.upper_camel_case_name }}(doc[config::{{ config.const_name }}].Get{{ config.doc_type }}());
    {% endif %}
  {% if config.version_overrides %}
  {% for version_override in config.version_overrides %}
  } else if (lynx::tasm::Config::IsHigherOrEqual(target_sdk_version_, {{ version_override["minSDKVersion"] }})) {
    page_config->Set{{ config.upper_camel_case_name }}({{ version_override["value"] }});
  }
  {% endfor %}
  {% else %}
  }
  {% endif %}
{% endif %}

{% endfor %}
"""
    start_marker = "// BEGIN CONFIG DECODE GEN"
    end_marker = "// END CONFIG DECODE GEN"
    gen_code = Template(
        config_decode_tmpl, trim_blocks=True, lstrip_blocks=True
    ).render(configs=configs)
    replace_comments_with_code(start_marker, end_marker, gen_code, config_decode_path)
    clang_format(config_decode_path)


def gen_lynx_config():
    configs = parse_config()
    gen_lynx_config_path = os.path.join(
        _binary_decoder_path,
        "auto_gen_lynx_config_constants.h",
    )
    lynx_config_tmpl = """// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

namespace lynx {
namespace tasm {
namespace config {
{% for config in configs %}
  static constexpr const char {{ config.const_name }}[] = "{{ config.name }}";
{% endfor %}
}  // namespace config
}  // namespace tasm
}  // namespace lynx

"""
    if os.path.exists(gen_lynx_config_path):
        os.remove(gen_lynx_config_path)
    with open(gen_lynx_config_path, "w") as f:
        f.write(
            Template(lynx_config_tmpl, trim_blocks=True, lstrip_blocks=True).render(
                configs=configs
            )
        )
    clang_format(gen_lynx_config_path)


def gen_config():
    # gen native config setter and getter
    gen_native_config()
    # gen page config decode
    gen_page_config_decode()
    # gen lynx config constants
    gen_lynx_config()


if __name__ == "__main__":
    gen_config()
