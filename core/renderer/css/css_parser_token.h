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
#include <utility>
#include <vector>

#include "base/include/fml/memory/ref_counted.h"
#include "base/include/vector.h"
#include "core/base/lynx_export.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_sheet.h"
#include "core/renderer/css/css_value.h"
#include "core/renderer/css/unit_handler.h"

namespace lynx {
namespace tasm {

class CSSParseToken : public fml::RefCountedThreadSafeStorage {
 public:
  CSSParseToken(const CSSParserConfigs& parser_configs)
      : parser_configs_(parser_configs) {}

  virtual ~CSSParseToken() = default;

  void ReleaseSelf() const override { delete this; }

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
  LYNX_EXPORT_FOR_DEVTOOL virtual const StyleMap& GetAttributes();

  auto& raw_attributes() { return raw_attributes_; }

  bool IsPseudoStyleToken() const;
  bool IsCascadeSelectorStyleToken() const;
  int GetStyleTokenType() const;

  void MarkAsTouchPseudoToken();
  bool IsTouchPseudoToken() const;

  const CSSParserConfigs& GetCSSParserConfigs() { return parser_configs_; }

  inline void MarkParsed() { parser_state_ = ParseState::kParsed; }

 protected:
  bool is_touch_pseudo_{false};
  base::InlineVector<std::shared_ptr<CSSSheet>, 4> sheets_;

  StyleMap attributes_;
  RawStyleMap raw_attributes_;
  CSSVariableMap style_variables_;
  CSSParserConfigs parser_configs_;

 private:
  enum ParseState : int {
    kNotParsed = 0,
    kParsing,
    kParsed,
  };

  std::atomic_int parser_state_{ParseState::kNotParsed};
};
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_CSS_PARSER_TOKEN_H_
