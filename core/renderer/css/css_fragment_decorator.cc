// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/css_fragment_decorator.h"

#include <utility>

#include "base/include/no_destructor.h"
#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"

namespace lynx {
namespace tasm {

CSSFragmentDecorator::~CSSFragmentDecorator() = default;

const CSSParserTokenMap& CSSFragmentDecorator::pseudo_map() {
  if (!intrinsic_style_sheets_) {
    static base::NoDestructor<CSSParserTokenMap> fake_pseudo{};
    return *fake_pseudo;
  }
  return intrinsic_style_sheets_->pseudo_map();
}

const CSSParserTokenMap& CSSFragmentDecorator::child_pseudo_map() {
  if (!intrinsic_style_sheets_) {
    static base::NoDestructor<CSSParserTokenMap> fake_child_pseudo{};
    return *fake_child_pseudo;
  }
  return intrinsic_style_sheets_->child_pseudo_map();
}

const PseudoNotStyle& CSSFragmentDecorator::pseudo_not_style() {
  if (!intrinsic_style_sheets_) {
    static base::NoDestructor<PseudoNotStyle> fake_pseudo_not_style{};
    return *fake_pseudo_not_style;
  }
  return intrinsic_style_sheets_->pseudo_not_style();
}

const CSSParserTokenMap& CSSFragmentDecorator::css() {
  // TODO(wangyifei.20010605): only for unittest, don't use it please, I will
  // remove it later.
  if (intrinsic_style_sheets_ &&
      intrinsic_style_sheets_->enable_css_selector()) {
    for (auto& ext_css : external_css_) {
      intrinsic_style_sheets_->rule_set()->AddToRuleSet(ext_css.first,
                                                        ext_css.second);
    }
  }
  if (!external_css_.empty()) {
    return external_css_;
  } else if (intrinsic_style_sheets_) {
    return intrinsic_style_sheets_->css();
  }
  return external_css_;
}

const CSSParserTokenMap& CSSFragmentDecorator::cascade_map() {
  if (!intrinsic_style_sheets_) {
    static base::NoDestructor<CSSParserTokenMap> fake_cascade{};
    return *fake_cascade;
  }
  return intrinsic_style_sheets_->cascade_map();
}

const CSSKeyframesTokenMap& CSSFragmentDecorator::GetKeyframesRuleMap() {
  if (!intrinsic_style_sheets_) {
    static base::NoDestructor<CSSKeyframesTokenMap> fake_keyframes{};
    return *fake_keyframes;
  }
  return intrinsic_style_sheets_->GetKeyframesRuleMap();
}

css::RuleSet* CSSFragmentDecorator::rule_set() {
  if (!intrinsic_style_sheets_) {
    return nullptr;
  }
  return intrinsic_style_sheets_->rule_set();
}

const CSSFontFaceRuleMap& CSSFragmentDecorator::GetFontFaceRuleMap() {
  if (!intrinsic_style_sheets_) {
    static base::NoDestructor<CSSFontFaceRuleMap> fake_fontfaces{};
    return *fake_fontfaces;
  }
  return intrinsic_style_sheets_->GetFontFaceRuleMap();
}

bool CSSFragmentDecorator::HasCSSStyle() {
  if (has_css_style_.has_value()) {
    return has_css_style_.value_or(false);
  }
  if (enable_css_lazy_import_) {
    if (!external_css_.empty() ||
        (intrinsic_style_sheets_ && intrinsic_style_sheets_->HasCSSStyle())) {
      has_css_style_ = true;
      return true;
    }
    has_css_style_ = false;
    return false;
  } else {
    has_css_style_ = !css().empty();
    return !css().empty();
  }
}

CSSParseToken* CSSFragmentDecorator::GetCSSStyle(const std::string& key) {
  if (intrinsic_style_sheets_ &&
      intrinsic_style_sheets_->enable_css_selector()) {
    for (auto& ext_css : external_css_) {
      intrinsic_style_sheets_->rule_set()->AddToRuleSet(ext_css.first,
                                                        ext_css.second);
    }
  }
  if (!external_css_.empty()) {
    auto it = external_css_.find(key);
    if (it != external_css_.end()) {
      return it->second.get();
    }
  }
  if (intrinsic_style_sheets_) {
    return intrinsic_style_sheets_->GetCSSStyle(key);
  }
  return nullptr;
}

CSSKeyframesToken* CSSFragmentDecorator::GetKeyframesRule(
    const std::string& key) {
  if (!intrinsic_style_sheets_) {
    return nullptr;
  }
  return intrinsic_style_sheets_->GetKeyframesRule(key);
}

const std::vector<std::shared_ptr<CSSFontFaceRule>>&
CSSFragmentDecorator::GetFontFaceRule(const std::string& key) {
  if (!intrinsic_style_sheets_) {
    return GetDefaultFontFaceList();
  }
  return intrinsic_style_sheets_->GetFontFaceRule(key);
}

std::shared_ptr<CSSParseToken> CSSFragmentDecorator::GetSharedCSSStyle(
    const std::string& key) {
  if (intrinsic_style_sheets_ &&
      intrinsic_style_sheets_->enable_css_selector()) {
    for (auto& ext_css : external_css_) {
      intrinsic_style_sheets_->rule_set()->AddToRuleSet(ext_css.first,
                                                        ext_css.second);
    }
  }
  if (!external_css_.empty()) {
    auto it = external_css_.find(key);
    if (it != external_css_.end()) {
      return it->second;
    }
  }
  if (intrinsic_style_sheets_) {
    return intrinsic_style_sheets_->GetSharedCSSStyle(key);
  }
  return nullptr;
}

void CSSFragmentDecorator::AddExternalStyle(
    const std::string& key, std::shared_ptr<CSSParseToken> value) {
  // A new independent attribute map is needed for each component instance, as
  // multiple external tokens may merge to become the new token.
  if (external_css_.find(key) == external_css_.end()) {
    external_css_[key] = std::move(value);
    return;
  }

  auto& target = external_css_[key];
  for (auto it : value->GetAttributes()) {
    target->SetAttribute(it.first, it.second);
  }
  // Resolve raw_attributes and mark target token is already parsed.
  target->GetAttributes();
}

bool CSSFragmentDecorator::HasPseudoNotStyle() {
  if (intrinsic_style_sheets_) {
    return intrinsic_style_sheets_->HasPseudoNotStyle();
  }
  return false;
}

void CSSFragmentDecorator::InitPseudoNotStyle() {
  if (intrinsic_style_sheets_) {
    intrinsic_style_sheets_->InitPseudoNotStyle();
  }
}

#define GET_PARSER_TOKEN_STYLE(name)                         \
  CSSParseToken* CSSFragmentDecorator::Get##name##Style(     \
      const std::string& key) {                              \
    if (intrinsic_style_sheets_) {                           \
      return intrinsic_style_sheets_->Get##name##Style(key); \
    }                                                        \
    return nullptr;                                          \
  }

GET_PARSER_TOKEN_STYLE(Pseudo)
GET_PARSER_TOKEN_STYLE(Cascade)
GET_PARSER_TOKEN_STYLE(Id)
GET_PARSER_TOKEN_STYLE(Tag)
GET_PARSER_TOKEN_STYLE(Universal)
#undef GET_PARSER_TOKEN_STYLE

}  // namespace tasm
}  // namespace lynx
