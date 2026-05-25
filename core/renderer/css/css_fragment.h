// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_CSS_FRAGMENT_H_
#define CORE_RENDERER_CSS_CSS_FRAGMENT_H_

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/include/value/base_string.h"
#include "base/include/vector.h"
#include "core/renderer/css/css_font_face_token.h"
#include "core/renderer/css/css_keyframes_token.h"
#include "core/renderer/css/css_parser_token.h"
#include "core/renderer/css/ng/invalidation/invalidation_set.h"
#include "core/renderer/css/ng/style/rule_set.h"

namespace lynx {
namespace tasm {

struct PseudoNotContent {
  CSSSheet::SheetType scope_type;
  std::string selector_key;
  std::string scope;
};

using PseudoClassStyleMap = std::unordered_map<std::string, PseudoNotContent>;

using CSSParserTokenMap =
    std::unordered_map<std::string, fml::RefPtr<CSSParseToken>>;

using CSSKeyframesTokenMap =
    base::LinearFlatMap<base::String, fml::RefPtr<CSSKeyframesToken>>;

struct PseudoNotStyle {
  PseudoClassStyleMap pseudo_not_for_tag;
  PseudoClassStyleMap pseudo_not_for_class;
  PseudoClassStyleMap pseudo_not_for_id;
  std::unordered_map<int, PseudoClassStyleMap> pseudo_not_global_map;
};

// TODO(songshourui.null): rename this class to StyleSheet.
class CSSFragment {
 public:
  CSSFragment() = default;

  CSSFragment(CSSKeyframesTokenMap keyframes, CSSFontFaceRuleMap fontfaces)
      : keyframes_(std::move(keyframes)), fontfaces_(std::move(fontfaces)){};

  virtual ~CSSFragment() = default;

  virtual const CSSParserTokenMap& pseudo_map() = 0;
  virtual const CSSParserTokenMap& child_pseudo_map() = 0;
  virtual const CSSParserTokenMap& cascade_map() = 0;
  virtual const CSSParserTokenMap& css() = 0;
  virtual css::RuleSet* rule_set() = 0;
  virtual const PseudoNotStyle& pseudo_not_style() = 0;

  virtual CSSParseToken* GetCSSStyle(const std::string& key) = 0;
  virtual CSSParseToken* GetPseudoStyle(const std::string& key) = 0;
  virtual CSSParseToken* GetCascadeStyle(const std::string& key) = 0;
  virtual CSSParseToken* GetIdStyle(const std::string& key) = 0;
  virtual CSSParseToken* GetTagStyle(const std::string& key) = 0;
  virtual CSSParseToken* GetUniversalStyle(const std::string& key) = 0;

  virtual bool HasPseudoNotStyle() = 0;
  virtual void InitPseudoNotStyle() = 0;
  virtual bool HasIdSelector() { return true; }

  virtual bool enable_css_selector() = 0;
  virtual bool enable_css_invalidation() = 0;

  virtual void CollectInvalidationSetsForId(css::InvalidationLists& lists,
                                            const std::string& id) = 0;
  virtual void CollectInvalidationSetsForClass(
      css::InvalidationLists& lists, const std::string& class_name) = 0;
  virtual void CollectInvalidationSetsForPseudoClass(
      css::InvalidationLists& lists,
      css::LynxCSSSelector::PseudoType pseudo) = 0;

  virtual fml::RefPtr<CSSParseToken> GetSharedCSSStyle(
      const std::string& key) = 0;

  virtual const CSSKeyframesTokenMap& GetKeyframesRuleMap();

  virtual const CSSFontFaceRuleMap& GetFontFaceRuleMap();

  virtual CSSKeyframesToken* GetKeyframesRule(const base::String& key);
  virtual const std::vector<std::shared_ptr<CSSFontFaceRule>>& GetFontFaceRule(
      const std::string& key);

  virtual bool HasCSSStyle() = 0;

  // Check if this stylesheet (including adopted sheets for decorators)
  // contains pseudo rules.
  virtual bool HasPseudoRules() {
    auto* rs = rule_set();
    return rs && !rs->pseudo_rules().empty();
  }

  // Check if this stylesheet (including adopted sheets for decorators)
  // contains adjacent-sibling rules.
  virtual bool HasAdjacentSiblingRules() {
    auto* rs = rule_set();
    return rs && rs->HasAdjacentSiblingRules();
  }

  // Check if this stylesheet (including adopted sheets for decorators)
  // contains condition rules with parsed media queries.
  virtual bool HasMediaQueryRules() {
    auto* rs = rule_set();
    return rs && rs->HasMediaQueryRules();
  }

  bool HasPseudoStyle() { return !pseudo_map().empty(); }

  bool HasCascadeStyle() { return !cascade_map().empty(); }

  // Iterate all keyframes maps that contribute to this stylesheet.
  // For CSSFragmentDecorator, this includes adopted stylesheets.
  using ForEachKeyframesMapVisitor = void (*)(const CSSKeyframesTokenMap&,
                                              void*);
  virtual void ForEachKeyframesMap(ForEachKeyframesMapVisitor visitor,
                                   void* cb_data) {
    visitor(GetKeyframesRuleMap(), cb_data);
  }

  // Iterate all font-face maps that contribute to this stylesheet.
  // For CSSFragmentDecorator, this includes adopted stylesheets.
  using ForEachFontFaceMapVisitor = void (*)(const CSSFontFaceRuleMap&, void*);
  virtual void ForEachFontFaceMap(ForEachFontFaceMapVisitor visitor,
                                  void* cb_data) {
    visitor(GetFontFaceRuleMap(), cb_data);
  }

  // Iterate all rule sets that contribute to this stylesheet.
  // For CSSFragmentDecorator, this includes adopted stylesheets.
  using ForEachRuleSetVisitor = void (*)(css::RuleSet*, void*);
  virtual void ForEachRuleSet(ForEachRuleSetVisitor visitor, void* cb_data) {
    if (auto* rs = rule_set()) {
      visitor(rs, cb_data);
    }
  }

  bool HasFontFacesResolved() const { return has_font_faces_resolved_; }

  virtual void MarkFontFacesResolved(bool resolved) {
    has_font_faces_resolved_ = resolved;
  }

  void MarkHasTouchPseudoToken() { has_touch_pseudo_token_ = true; }
  bool HasTouchPseudoToken() const { return has_touch_pseudo_token_; }
  const std::vector<std::shared_ptr<CSSFontFaceRule>>& GetDefaultFontFaceList();

  void SetKeyFramesRuleMap(CSSKeyframesTokenMap map) {
    keyframes_ = std::move(map);
  }
  void SetFontFaceRuleMap(CSSFontFaceRuleMap map) {
    fontfaces_ = std::move(map);
  }

  bool GetEnableCSSLazyImport() { return enable_css_lazy_import_; }

  void SetEnableCSSLazyImport(bool enable) { enable_css_lazy_import_ = enable; }

  static void CollectIdChangedInvalidation(CSSFragment*,
                                           css::InvalidationLists&,
                                           const std::string&,
                                           const std::string&);

  static void CollectClassChangedInvalidation(CSSFragment*,
                                              css::InvalidationLists&,
                                              const ClassList&,
                                              const ClassList&);

  static void CollectPseudoChangedInvalidation(CSSFragment*,
                                               css::InvalidationLists&,
                                               PseudoState, PseudoState);

 protected:
  bool has_touch_pseudo_token_{false};
  // FIXME(linxs): it's better to flush related fontface or keyframe only when
  // any element has font-family or animation indicated the font faces has been
  // resolved or not
  bool has_font_faces_resolved_{false};

  // enableCSSLazyImport's default value is false now.
  bool enable_css_lazy_import_ = false;

  std::optional<bool> has_css_style_;

  CSSKeyframesTokenMap keyframes_;
  CSSFontFaceRuleMap fontfaces_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_CSS_FRAGMENT_H_
