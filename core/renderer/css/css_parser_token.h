// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CORE_RENDERER_CSS_CSS_PARSER_TOKEN_H_
#define CORE_RENDERER_CSS_CSS_PARSER_TOKEN_H_

#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/base_export.h"
#include "base/include/vector.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_sheet.h"
#include "core/renderer/css/css_value.h"
#include "core/renderer/css/unit_handler.h"

namespace lynx {
namespace tasm {

class CSSParseToken {
 public:
  CSSParseToken(const CSSParserConfigs& parser_configs)
      : parser_configs_(parser_configs) {}

  virtual ~CSSParseToken() = default;

  auto& sheets() { return sheets_; }
  const std::shared_ptr<CSSSheet>& TargetSheet() const {
    return sheets_.back();
  }

  auto& style_variables() { return style_variables_; }
  const auto& GetStyleVariables() { return style_variables_; }

  void SetAttribute(CSSPropertyID id, const CSSValue& value) {
    attributes_[id] = value;
  }

  virtual void SetAttributes(StyleMap&& attributes) {
    attributes_ = std::move(attributes);
    MarkParsed();
  }

  auto& attributes() { return attributes_; }
  BASE_EXPORT_FOR_DEVTOOL virtual const StyleMap& GetAttributes();

  auto& raw_attributes() { return raw_attributes_; }

  bool IsPseudoStyleToken() const;
  bool IsCascadeSelectorStyleToken() const;
  int GetStyleTokenType() const;

  void MarkAsTouchPseudoToken();
  bool IsTouchPseudoToken() const;

  const CSSParserConfigs& GetCSSParserConfigs() { return parser_configs_; }

  inline void MarkParsed() {
    parser_state_ = static_cast<int>(ParseState::kParsed);
  }

 protected:
  bool is_touch_pseudo_{false};
  base::InlineVector<std::shared_ptr<CSSSheet>, 4> sheets_;

  StyleMap attributes_;
  RawStyleMap raw_attributes_;
  CSSVariableMap style_variables_;
  CSSParserConfigs parser_configs_;

 private:
  enum class ParseState {
    kNotParsed = 0,
    kParsing,
    kParsed,
  };

  std::atomic_int parser_state_{static_cast<int>(ParseState::kNotParsed)};
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_CSS_PARSER_TOKEN_H_
