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

// A decorator that lives in each component and takes into account both
// intra-component styles and external classes.
class CSSFragmentDecorator : public CSSFragment {
 public:
  CSSFragmentDecorator(CSSFragment* intrinsic_style_sheets)
      : intrinsic_style_sheets_(intrinsic_style_sheets) {
    if (intrinsic_style_sheets) {
      enable_css_lazy_import_ =
          intrinsic_style_sheets->GetEnableCSSLazyImport();
    }
  }
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

  std::shared_ptr<CSSParseToken> GetSharedCSSStyle(
      const std::string& key) override;
  const CSSKeyframesTokenMap& GetKeyframesRuleMap() override;
  const CSSFontFaceRuleMap& GetFontFaceRuleMap() override;
  CSSKeyframesToken* GetKeyframesRule(const std::string& key) override;
  const std::vector<std::shared_ptr<CSSFontFaceRule>>& GetFontFaceRule(
      const std::string& key) override;

  void AddExternalStyle(const std::string& key,
                        std::shared_ptr<CSSParseToken> value);

  inline bool enable_css_selector() override {
    return intrinsic_style_sheets_ &&
           intrinsic_style_sheets_->enable_css_selector();
  }

  inline bool enable_css_invalidation() override {
    return intrinsic_style_sheets_ &&
           intrinsic_style_sheets_->enable_css_invalidation();
  }

  void CollectInvalidationSetsForId(css::InvalidationLists& lists,
                                    const std::string& id) override {
    if (intrinsic_style_sheets_)
      intrinsic_style_sheets_->CollectInvalidationSetsForId(lists, id);
  }

  void CollectInvalidationSetsForClass(css::InvalidationLists& lists,
                                       const std::string& class_name) override {
    if (intrinsic_style_sheets_)
      intrinsic_style_sheets_->CollectInvalidationSetsForClass(lists,
                                                               class_name);
  }

  void CollectInvalidationSetsForPseudoClass(
      css::InvalidationLists& lists,
      css::LynxCSSSelector::PseudoType pseudo) override {
    if (intrinsic_style_sheets_)
      intrinsic_style_sheets_->CollectInvalidationSetsForPseudoClass(lists,
                                                                     pseudo);
  }

 private:
  CSSFragment* intrinsic_style_sheets_ = nullptr;
  CSSParserTokenMap external_css_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_CSS_CSS_FRAGMENT_DECORATOR_H_
