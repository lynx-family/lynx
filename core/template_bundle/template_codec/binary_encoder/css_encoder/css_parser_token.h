// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_CSS_ENCODER_CSS_PARSER_TOKEN_H_
#define CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_CSS_ENCODER_CSS_PARSER_TOKEN_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/base_export.h"
#include "core/renderer/css/css_parser_token.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_sheet.h"
#include "core/renderer/css/css_value.h"
#include "core/renderer/css/unit_handler.h"
#include "core/runtime/vm/lepus/json_parser.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/template_bundle/template_codec/compile_options.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace encoder {

// TODO(songshourui.null): Subsequently, the encoder::CSSParseToken will be
// renamed to CSSStyleRuleForEncoder. It will no longer inherit from
// tasm::CSSParseToken but will instead hold an instance of tasm::CSSParseToken.
class CSSParseToken : public tasm::CSSParseToken {
 public:
  static void SplitRules(const std::string& str, const std::string& pattern,
                         std::vector<std::string>& des);

  CSSParseToken(const rapidjson::Value& style, std::string& rule,
                const std::string& path,
                const rapidjson::Value& style_variables,
                const tasm::CompileOptions& compile_options);
  CSSParseToken(const lepus::Value& style, std::string& rule,
                const std::string& path, const lepus::Value& style_variables,
                const tasm::CompileOptions& compile_options);
  CSSParseToken(const tasm::CSSParserConfigs& parser_configs)
      : tasm::CSSParseToken(parser_configs) {}

  virtual const tasm::StyleMap& GetAttributes() override;

  virtual void SetAttributes(tasm::StyleMap&& attributes) override {
    attributes_ = std::move(attributes);
  }

  virtual ~CSSParseToken() override {}

  bool IsGlobalPseudoStyleToken() const;

 private:
  void ParseAttributes(const rapidjson::Value& value);
  void ParseStyleVariables(const rapidjson::Value& value);
  void SplitSelector(std::string& select);
  void HandlePseudoSelector(std::string& select);

  std::shared_ptr<tasm::CSSSheet> CreatSheet(
      const std::string& name, std::shared_ptr<tasm::CSSSheet> parent_sheet);

  std::string path_;
  const tasm::CompileOptions compile_options_;
};
}  // namespace encoder
}  // namespace lynx

#endif  // CORE_TEMPLATE_BUNDLE_TEMPLATE_CODEC_BINARY_ENCODER_CSS_ENCODER_CSS_PARSER_TOKEN_H_
