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
#include "core/base/lynx_trace_categories.h"

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

const StyleMap& CSSParseToken::GetAttributes() {
  if (parser_state_.load(std::memory_order_acquire) ==
      static_cast<int>(ParseState::kParsed)) {
    // If token is already parsed, return the parsed attributes.
    return attributes_;
  }

  StyleMap css_attribute;
  size_t total_pool_capacity = 0;

  // If raw attributes is not empty, process raw attributes.
  if (!raw_attributes_.empty()) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "CSSPatching::ProcessRaw");
    // Get the total pool capacity of the parsed attributes from raw_attributes.
    total_pool_capacity =
        CSSProperty::GetTotalParsedStyleCountFromMap(raw_attributes_);
    css_attribute.set_pool_capacity(total_pool_capacity);
    // Process raw attributes and store them in local variable css_attribute.
    raw_attributes_.foreach ([&](const CSSPropertyID& k, const CSSValue& v) {
      UnitHandler::ProcessCSSValue(k, v, css_attribute, parser_configs_);
    });
  }

  int expected = static_cast<int>(ParseState::kNotParsed);
  // Try to set the parser state to kParsing.
  while (!parser_state_.compare_exchange_strong(
             expected, static_cast<int>(ParseState::kParsing)) &&
         parser_state_.load(std::memory_order_acquire) !=
             static_cast<int>(ParseState::kParsed)) {
    expected = static_cast<int>(ParseState::kNotParsed);
  }

  // If the parser state is already kParsed, return the parsed attributes.
  if (parser_state_.load(std::memory_order_acquire) ==
      static_cast<int>(ParseState::kParsed)) {
    return attributes_;
  }

  attributes_ = std::move(css_attribute);
  // Set the parser state to kParsed.
  parser_state_.store(static_cast<int>(ParseState::kParsed),
                      std::memory_order_release);
  return attributes_;
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
