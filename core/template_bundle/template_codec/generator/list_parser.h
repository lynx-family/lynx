// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_GENERATOR_LIST_PARSER_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_GENERATOR_LIST_PARSER_H_

#include <string>
#include <unordered_map>

#include "core/template_bundle/template_codec/generator/template_parser.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace tasm {

class ListParser {
 public:
  enum class ListComponentType : size_t {
    DEFAULT = 0,
    HEADER = 1,
    FOOTER = 2,
    LIST_ROW = 3
  };
  using ListNodeType = std::array<std::string, 3>;
  ListParser(TemplateParser &parser, int element_id)
      : parser_{parser}, eid_{element_id} {}
  std::string GenList(const rapidjson::Value &child);

 private:
  TemplateParser &parser_;
  const int eid_;

  ListParser::ListNodeType Generate(const rapidjson::Value &child,
                                    ListComponentType list_component_type,
                                    size_t depth);

  ListParser::ListNodeType GenerateList(const rapidjson::Value &child,
                                        ListComponentType list_component_type,
                                        size_t depth);

  ListParser::ListNodeType GenerateIf(const rapidjson::Value &child,
                                      ListComponentType list_component_type,
                                      size_t depth);

  ListParser::ListNodeType GenerateRepeat(const rapidjson::Value &child,
                                          ListComponentType list_component_type,
                                          size_t depth);

  ListParser::ListNodeType GenerateNode(const rapidjson::Value &child,
                                        ListComponentType list_component_type,
                                        size_t depth);

  ListParser::ListNodeType GenerateNodeBlock(
      const rapidjson::Value &child, ListComponentType list_component_type,
      size_t depth);

  ListParser::ListNodeType GenerateNodeHeader(
      const rapidjson::Value &child, ListComponentType list_component_type,
      size_t depth);

  ListParser::ListNodeType GenerateNodeFooter(
      const rapidjson::Value &child, ListComponentType list_component_type,
      size_t depth);

  ListParser::ListNodeType GenerateNodeListRow(
      const rapidjson::Value &child, ListComponentType list_component_type,
      size_t depth);

  ListParser::ListNodeType GenerateComponent(
      const rapidjson::Value &child, ListComponentType list_component_type,
      size_t depth);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_GENERATOR_LIST_PARSER_H_
