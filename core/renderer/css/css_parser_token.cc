// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/renderer/css/css_parser_token.h"

#include <cstddef>

#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/renderer/trace/renderer_trace_event_def.h"

namespace lynx {
namespace tasm {

bool CSSParseToken::IsPseudoStyleToken() const {
  const auto& target_sheet_ptr = TargetSheet();
  if (target_sheet_ptr) {
    return target_sheet_ptr->GetType() > CSSSheet::NAME_SELECT &&
           target_sheet_ptr->GetType() != CSSSheet::ALL_SELECT;
  } else {
    return false;
  }
}

bool CSSParseToken::IsCascadeSelectorStyleToken() const {
  return sheets_.size() > 1;
}

namespace {
inline void ParseRawStyleMap(const RawStyleMap& raw, StyleMap& out,
                             const CSSParserConfigs& configs) {
  if (raw.empty()) {
    return;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, CSS_PATCH_PROCESS_RAW);
  size_t capacity = CSSProperty::GetTotalParsedStyleCountFromMap(raw);
  out.reserve(capacity);
  raw.for_each([&](const CSSPropertyID& k, const CSSValue& v) {
    UnitHandler::ProcessCSSValue(k, v, out, configs);
  });
}
}  // namespace

const StyleMap& CSSParseToken::GetAttributes() {
  if (parser_state_.load(std::memory_order_acquire) == ParseState::kParsed) {
    return attributes_;
  }

  StyleMap css_attribute;
  StyleMap css_important_attribute;

  ParseRawStyleMap(raw_attributes_, css_attribute, parser_configs_);
  ParseRawStyleMap(raw_important_attributes_, css_important_attribute,
                   parser_configs_);

  int expected = ParseState::kNotParsed;
  while (
      !parser_state_.compare_exchange_strong(expected, ParseState::kParsing) &&
      parser_state_.load(std::memory_order_acquire) != ParseState::kParsed) {
    expected = ParseState::kNotParsed;
  }

  if (parser_state_.load(std::memory_order_acquire) == ParseState::kParsed) {
    return attributes_;
  }

  attributes_ = std::move(css_attribute);
  important_attributes_ = std::move(css_important_attribute);
  parser_state_.store(ParseState::kParsed, std::memory_order_release);
  return attributes_;
}

const StyleMap& CSSParseToken::GetImportantAttributes() {
  if (parser_state_.load(std::memory_order_acquire) != ParseState::kParsed) {
    GetAttributes();
  }
  return important_attributes_;
}

int CSSParseToken::GetStyleTokenType() const {
  if (sheets_.empty()) {
    return 0;
  }
  const auto& target_sheet_ptr = TargetSheet();
  return target_sheet_ptr ? target_sheet_ptr->GetType() : 0;
}

void CSSParseToken::MarkAsTouchPseudoToken() { is_touch_pseudo_ = true; }

bool CSSParseToken::IsTouchPseudoToken() const { return is_touch_pseudo_; }

}  // namespace tasm
}  // namespace lynx
