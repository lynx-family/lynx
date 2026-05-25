// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/css_fragment_decorator.h"

#include <utility>

#include "base/include/no_destructor.h"
#include "base/trace/native/trace_event.h"
#include "core/renderer/dom/element_manager.h"

namespace lynx {
namespace tasm {

CSSFragmentDecorator::CSSFragmentDecorator(CSSFragment* intrinsic_style_sheets)
    : CSSFragmentDecorator(intrinsic_style_sheets, nullptr) {}

CSSFragmentDecorator::CSSFragmentDecorator(CSSFragment* intrinsic_style_sheets,
                                           ElementManager* element_manager)
    : intrinsic_style_sheets_(intrinsic_style_sheets),
      element_manager_(element_manager) {
  if (intrinsic_style_sheets) {
    enable_css_lazy_import_ = intrinsic_style_sheets->GetEnableCSSLazyImport();
  }
}

CSSFragmentDecorator::~CSSFragmentDecorator() = default;

template <typename Func>
void CSSFragmentDecorator::ForEachAdoptedFragment(Func&& func,
                                                  bool reverse) const {
  if (element_manager_) {
    element_manager_->ForEachAdoptedStyleSheet(
        [&func](const auto& wrapper) {
          if (wrapper && wrapper->fragment_) {
            return func(*wrapper->fragment_);
          }
          return true;
        },
        reverse);
  }
}

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

const std::vector<std::shared_ptr<CSSFontFaceRule>>&
CSSFragmentDecorator::GetFontFaceRule(const std::string& key) {
  if (!intrinsic_style_sheets_) {
    return GetDefaultFontFaceList();
  }
  return intrinsic_style_sheets_->GetFontFaceRule(key);
}

fml::RefPtr<CSSParseToken> CSSFragmentDecorator::GetSharedCSSStyle(
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

void CSSFragmentDecorator::AddExternalStyle(const std::string& key,
                                            fml::RefPtr<CSSParseToken> value) {
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

bool CSSFragmentDecorator::enable_css_selector() {
  return intrinsic_style_sheets_ &&
         intrinsic_style_sheets_->enable_css_selector();
}

bool CSSFragmentDecorator::enable_css_invalidation() {
  return intrinsic_style_sheets_ &&
         intrinsic_style_sheets_->enable_css_invalidation();
}

template <typename Predicate>
bool CSSFragmentDecorator::HasInAdopted(Predicate pred) {
  bool found = false;
  ForEachAdoptedFragment([&found, &pred](CSSFragment& fragment) {
    if (fragment.enable_css_selector() && pred(fragment)) {
      found = true;
      return false;
    }
    return true;
  });
  return found;
}

void CSSFragmentDecorator::MarkFontFacesResolved(bool resolved) {
  CSSFragment::MarkFontFacesResolved(resolved);
  if (element_manager_) {
    element_manager_->ForEachAdoptedStyleSheet([resolved](const auto& wrapper) {
      if (wrapper) {
        wrapper->MarkFontFacesResolved(resolved);
      }
      return true;
    });
  }
}

bool CSSFragmentDecorator::HasPseudoRules() {
  if (CSSFragment::HasPseudoRules()) {
    return true;
  }
  return HasInAdopted([](CSSFragment& f) {
    return f.rule_set() && !f.rule_set()->pseudo_rules().empty();
  });
}

bool CSSFragmentDecorator::HasAdjacentSiblingRules() {
  if (CSSFragment::HasAdjacentSiblingRules()) {
    return true;
  }
  return HasInAdopted([](CSSFragment& f) {
    return f.rule_set() && f.rule_set()->HasAdjacentSiblingRules();
  });
}

bool CSSFragmentDecorator::HasMediaQueryRules() {
  if (CSSFragment::HasMediaQueryRules()) {
    return true;
  }
  return HasInAdopted([](CSSFragment& f) {
    return f.rule_set() && f.rule_set()->HasMediaQueryRules();
  });
}

CSSKeyframesToken* CSSFragmentDecorator::GetKeyframesRule(
    const base::String& key) {
  CSSKeyframesToken* result = nullptr;
  ForEachAdoptedFragment(
      [&result, &key](CSSFragment& fragment) {
        if (auto* token = fragment.GetKeyframesRule(key)) {
          result = token;
          return false;
        }
        return true;
      },
      true);
  if (result) {
    return result;
  }
  if (intrinsic_style_sheets_) {
    return intrinsic_style_sheets_->GetKeyframesRule(key);
  }
  return nullptr;
}

void CSSFragmentDecorator::ForEachKeyframesMap(
    ForEachKeyframesMapVisitor visitor, void* cb_data) {
  struct Ctx {
    ForEachKeyframesMapVisitor visitor;
    void* cb_data;
  };
  Ctx ctx{visitor, cb_data};
  ForEachAdoptedFragment(
      [&ctx](CSSFragment& fragment) {
        ctx.visitor(fragment.GetKeyframesRuleMap(), ctx.cb_data);
        return true;
      },
      true);
  if (intrinsic_style_sheets_) {
    visitor(intrinsic_style_sheets_->GetKeyframesRuleMap(), cb_data);
  }
}

void CSSFragmentDecorator::ForEachFontFaceMap(ForEachFontFaceMapVisitor visitor,
                                              void* cb_data) {
  struct Ctx {
    ForEachFontFaceMapVisitor visitor;
    void* cb_data;
  };
  Ctx ctx{visitor, cb_data};
  ForEachAdoptedFragment([&ctx](CSSFragment& fragment) {
    ctx.visitor(fragment.GetFontFaceRuleMap(), ctx.cb_data);
    return true;
  });
  if (intrinsic_style_sheets_) {
    visitor(intrinsic_style_sheets_->GetFontFaceRuleMap(), cb_data);
  }
}

void CSSFragmentDecorator::ForEachRuleSet(ForEachRuleSetVisitor visitor,
                                          void* cb_data) {
  if (intrinsic_style_sheets_ && intrinsic_style_sheets_->rule_set()) {
    visitor(intrinsic_style_sheets_->rule_set(), cb_data);
  }
  struct Ctx {
    ForEachRuleSetVisitor visitor;
    void* cb_data;
  };
  Ctx ctx{visitor, cb_data};
  ForEachAdoptedFragment([&ctx](CSSFragment& fragment) {
    if (fragment.enable_css_selector()) {
      if (auto* rs = fragment.rule_set()) {
        ctx.visitor(rs, ctx.cb_data);
      }
    }
    return true;
  });
}

void CSSFragmentDecorator::CollectInvalidationSetsForId(
    css::InvalidationLists& lists, const std::string& id) {
  if (intrinsic_style_sheets_) {
    intrinsic_style_sheets_->CollectInvalidationSetsForId(lists, id);
  }
  ForEachAdoptedFragment([&lists, &id](CSSFragment& fragment) {
    fragment.CollectInvalidationSetsForId(lists, id);
    return true;
  });
}

void CSSFragmentDecorator::CollectInvalidationSetsForClass(
    css::InvalidationLists& lists, const std::string& class_name) {
  if (intrinsic_style_sheets_) {
    intrinsic_style_sheets_->CollectInvalidationSetsForClass(lists, class_name);
  }
  ForEachAdoptedFragment([&lists, &class_name](CSSFragment& fragment) {
    fragment.CollectInvalidationSetsForClass(lists, class_name);
    return true;
  });
}

void CSSFragmentDecorator::CollectInvalidationSetsForPseudoClass(
    css::InvalidationLists& lists, css::LynxCSSSelector::PseudoType pseudo) {
  if (intrinsic_style_sheets_) {
    intrinsic_style_sheets_->CollectInvalidationSetsForPseudoClass(lists,
                                                                   pseudo);
  }
  ForEachAdoptedFragment([&lists, pseudo](CSSFragment& fragment) {
    fragment.CollectInvalidationSetsForPseudoClass(lists, pseudo);
    return true;
  });
}

}  // namespace tasm
}  // namespace lynx
