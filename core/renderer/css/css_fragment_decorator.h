// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_CSS_CSS_FRAGMENT_DECORATOR_H_
#define CORE_RENDERER_CSS_CSS_FRAGMENT_DECORATOR_H_

#include <memory>
#include <string>
#include <vector>

#include "core/renderer/css/css_fragment.h"

namespace lynx {
namespace tasm {

class ElementManager;

// A decorator that lives in each component and takes into account both
// intra-component styles, external classes, and adopted stylesheets.
class CSSFragmentDecorator : public CSSFragment {
 public:
  explicit CSSFragmentDecorator(CSSFragment* intrinsic_style_sheets);
  CSSFragmentDecorator(CSSFragment* intrinsic_style_sheets,
                       ElementManager* element_manager);
  ~CSSFragmentDecorator() override;

  const CSSParserTokenMap& css() override;
  const CSSParserTokenMap& pseudo_map() override;
  const CSSParserTokenMap& child_pseudo_map() override;
  const CSSParserTokenMap& cascade_map() override;
  css::RuleSet* rule_set() override;
  const PseudoNotStyle& pseudo_not_style() override;

  bool HasCSSStyle() override;
  CSSParseToken* GetCSSStyle(const std::string& key) override;
  CSSParseToken* GetPseudoStyle(const std::string& key) override;
  CSSParseToken* GetCascadeStyle(const std::string& key) override;
  CSSParseToken* GetIdStyle(const std::string& key) override;
  CSSParseToken* GetTagStyle(const std::string& key) override;
  CSSParseToken* GetUniversalStyle(const std::string& key) override;

  bool HasPseudoNotStyle() override;
  void InitPseudoNotStyle() override;

  fml::RefPtr<CSSParseToken> GetSharedCSSStyle(const std::string& key) override;
  const CSSKeyframesTokenMap& GetKeyframesRuleMap() override;
  const CSSFontFaceRuleMap& GetFontFaceRuleMap() override;
  CSSKeyframesToken* GetKeyframesRule(const base::String& key) override;
  const std::vector<std::shared_ptr<CSSFontFaceRule>>& GetFontFaceRule(
      const std::string& key) override;

  void AddExternalStyle(const std::string& key,
                        fml::RefPtr<CSSParseToken> value);

  bool enable_css_selector() override;
  bool enable_css_invalidation() override;

  bool HasPseudoRules() override;
  bool HasAdjacentSiblingRules() override;

  void MarkFontFacesResolved(bool resolved) override;

  void ForEachKeyframesMap(ForEachKeyframesMapVisitor visitor,
                           void* cb_data) override;
  void ForEachFontFaceMap(ForEachFontFaceMapVisitor visitor,
                          void* cb_data) override;
  void ForEachRuleSet(ForEachRuleSetVisitor visitor, void* cb_data) override;

  void CollectInvalidationSetsForId(css::InvalidationLists& lists,
                                    const std::string& id) override;
  void CollectInvalidationSetsForClass(css::InvalidationLists& lists,
                                       const std::string& class_name) override;
  void CollectInvalidationSetsForPseudoClass(
      css::InvalidationLists& lists,
      css::LynxCSSSelector::PseudoType pseudo) override;

  bool IntrinsicStyleSheetHasTouchPseudoToken() {
    if (intrinsic_style_sheets_) {
      return intrinsic_style_sheets_->HasTouchPseudoToken();
    }
    return HasTouchPseudoToken();
  }

 private:
  template <typename Func>
  void ForEachAdoptedFragment(Func&& func, bool reverse = false) const;

  template <typename Predicate>
  bool HasInAdopted(Predicate pred);

  CSSFragment* intrinsic_style_sheets_ = nullptr;
  ElementManager* element_manager_ = nullptr;
  CSSParserTokenMap external_css_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_CSS_FRAGMENT_DECORATOR_H_
