// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_STYLE_RESOLVER_H_
#define CORE_RENDERER_DOM_STYLE_RESOLVER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/include/value/base_value.h"
#include "base/include/vector.h"
#include "core/base/lynx_export.h"
#include "core/renderer/css/css_fragment.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_selector_constants.h"
#include "core/renderer/css/css_variable_handler.h"
#include "core/renderer/css/dynamic_css_styles_manager.h"
#include "core/renderer/dom/attribute_holder.h"
#include "core/runtime/lepus/bindings/style/shared_css_fragment_wrapper.h"

namespace lynx {
namespace style {
class SimpleStyleNode;
class StyleObject;
}  // namespace style
namespace tasm {
class FiberElement;
class ElementManager;

class StyleResolver {
 public:
  constexpr const static size_t kDefaultMatchedSize = 16;

  template <class T>
  using MatchedVector = base::InlineVector<T, kDefaultMatchedSize>;

  LYNX_EXPORT_FOR_DEVTOOL static MatchedVector<css::MatchedRule>
  GetCSSMatchedRule(
      AttributeHolder* node, CSSFragment* style_sheet,
      const std::vector<fml::RefPtr<tasm::SharedCSSFragmentWrapper>>*
          adopted_sheets = nullptr);

  void ResolveStyle(StyleMap& result, CSSFragment* fragment,
                    CSSVariableMap* changed_css_vars = nullptr);
  void HandleCSSVariables(StyleMap& styles);

  void HandlePseudoElement(CSSFragment* fragment);
  /**
   * @brief Resolves differences between an old and new list of style objects,
   *        applying changes to a target node.
   * @param old_ptr Pointer to the array of old style objects.
   * @param new_ptr Pointer to the array of new style objects.
   * @param target The SimpleStyleNode to which style changes are applied.
   *
   * This function iterates through both old and new style object lists to
   * determine which styles were added, removed, or remained unchanged.
   * It calls helper functions to handle the application or removal of styles
   * from the target node.
   */
  static void ResolveStyleObjects(style::StyleObject** old_ptr,
                                  style::StyleObject** new_ptr,
                                  style::SimpleStyleNode* target);

  static void ResolveStyleObjectsBasedOnExistingMap(
      const tasm::StyleMap& old_dcl_style, style::StyleObject** new_ptr,
      style::SimpleStyleNode* target);

 private:
  enum class PseudoClassType { kFocus, kHover, kActive };

  Element* element() const;
  ElementManager* manager() const;

  void GetCSSStyleNew(AttributeHolder* node, CSSFragment* style_sheet);

  void GetCSSStyleForFiber(FiberElement* node, CSSFragment* style_sheet);

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

  void ParsePlaceHolderTokens(PseudoPlaceHolderStyles& result,
                              const StyleMap& map);

  using InlineTokenVector = base::InlineVector<CSSParseToken*, 16>;

  PseudoPlaceHolderStyles ParsePlaceHolderTokens(
      const InlineTokenVector& tokens);

  InlineTokenVector ParsePseudoCSSTokens(AttributeHolder* node,
                                         const char* selector);

  void ResolvePseudoElement(PseudoState state, CSSFragment* fragment,
                            FiberElement* fiber_element,
                            const char* pseudo_selector);

  void ParsePseudoCSSTokensForFiber(FiberElement* element,
                                    CSSFragment* fragment, const char* selector,
                                    StyleMap& map);

  static thread_local MatchedVector<const StyleMap*> matched_style_map;
  static thread_local MatchedVector<const CSSVariableMap*> matched_variable_map;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_STYLE_RESOLVER_H_
