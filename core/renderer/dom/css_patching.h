// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_CSS_PATCHING_H_
#define CORE_RENDERER_DOM_CSS_PATCHING_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "base/include/base_export.h"
#include "base/include/vector.h"
#include "core/renderer/css/css_fragment.h"
#include "core/renderer/css/css_selector_constants.h"
#include "core/renderer/css/css_variable_handler.h"
#include "core/renderer/css/dynamic_css_styles_manager.h"
#include "core/renderer/dom/attribute_holder.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {
class FiberElement;
class RadonElement;
class ElementManager;

class CSSPatching {
 public:
  constexpr const static size_t kDefaultMatchedSize = 16;

  template <class T>
  using MatchedVector = base::InlineVector<T, kDefaultMatchedSize>;

  BASE_EXPORT_FOR_DEVTOOL static MatchedVector<css::MatchedRule>
  GetCSSMatchedRule(AttributeHolder* node, CSSFragment* style_sheet);

  CSSPatching(Element* element, ElementManager* manager);
  ~CSSPatching() = default;

  void ResolveStyle(StyleMap& result, CSSFragment* fragment,
                    CSSVariableMap* changed_css_vars = nullptr);
  void HandleCSSVariables(StyleMap& styles);

  void HandlePseudoElement(CSSFragment* fragment);

  void ResolvePseudoSelectors();
  void ResolvePlaceHolder();

  void SetEnableFiberArch(bool enable);
  void SetElementManager(ElementManager* manager) { manager_ = manager; }

 private:
  enum class PseudoClassType { kFocus, kHover, kActive };

  void GetCSSStyleNew(AttributeHolder* node, CSSFragment* style_sheet);

  void GetCSSStyleForFiber(FiberElement* node, CSSFragment* style_sheet);

  void GetCSSStyleCompatible(Element* element, CSSFragment* style_sheet);

  void DidCollectMatchedRules(AttributeHolder* holder, StyleMap& result,
                              CSSVariableMap* changed_css_vars,
                              size_t base_reserving_size = 0);

  void MergeHigherPriorityCSSStyle(const StyleMap& matched);

  void SetCSSVariableToNode(const CSSVariableMap& matched);

  void GetCSSByRule(CSSSheet::SheetType type, CSSFragment* style_sheet,
                    AttributeHolder* node, const std::string& rule);

  void ApplyCascadeStyles(CSSFragment* style_sheet, AttributeHolder* node,
                          const std::string& rule);

  void ApplyCascadeStylesForFiber(CSSFragment* style_sheet, FiberElement* node,
                                  const std::string& rule);

  void MergeHigherCascadeStyles(const std::string& current_selector,
                                const std::string& parent_selector,
                                AttributeHolder* node,
                                CSSFragment* style_sheet);
  void MergeHigherCascadeStylesForFiber(const std::string& current_selector,
                                        const std::string& parent_selector,
                                        AttributeHolder* node,
                                        CSSFragment* style_sheet);

  void PreSetGlobalPseudoNotCSS(
      CSSSheet::SheetType type, const std::string& rule,
      const std::unordered_map<int, PseudoClassStyleMap>&
          pseudo_not_global_array,
      CSSFragment* style_sheet, AttributeHolder* node);

  void ApplyPseudoNotCSSStyle(AttributeHolder* node,
                              const PseudoClassStyleMap& pseudo_not_map,
                              CSSFragment* style_sheet,
                              const std::string& selector);

  void ApplyPseudoClassChildSelectorStyle(Element* current_node,
                                          CSSFragment* style_sheet,
                                          const std::string& selector_key);

  void GetPseudoClassStyle(PseudoClassType pseudo_type,
                           CSSFragment* style_sheet, AttributeHolder* node);

  const tasm::CSSParserConfigs& GetCSSParserConfigs();

  RadonElement* CreatePseudoNode(int style_type);

  void UpdateContentNode(const StyleMap& attrs, RadonElement* element);

  void ParsePlaceHolderTokens(PseudoPlaceHolderStyles& result,
                              const StyleMap& map);

  using InlineTokenVector = base::InlineVector<CSSParseToken*, 16>;

  PseudoPlaceHolderStyles ParsePlaceHolderTokens(
      const InlineTokenVector& tokens);

  InlineTokenVector ParsePseudoCSSTokens(AttributeHolder* node,
                                         const char* selector);

  void UpdateSelectionPseudo(const InlineTokenVector& token_list,
                             RadonElement* self);

  void GenerateContentData(const lepus::Value& value,
                           const AttributeHolder* vnode,
                           RadonElement* shadow_node);

  void ResolvePseudoElement(PseudoState state, CSSFragment* fragment,
                            FiberElement* fiber_element,
                            const char* pseudo_selector);

  void ParsePseudoCSSTokensForFiber(FiberElement* element,
                                    CSSFragment* fragment, const char* selector,
                                    StyleMap& map);

  Element* element_;

  // TODO(songshourui.null): Remove `ElementManager` later, ensuring that
  // `CSSPatching` does not depend on `ElementManager`.
  ElementManager* manager_;

  CSSVariableHandler css_var_handler_;

  static thread_local MatchedVector<const StyleMap*> matched_style_map;
  static thread_local MatchedVector<const CSSVariableMap*> matched_variable_map;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_CSS_PATCHING_H_
