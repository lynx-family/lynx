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
#include "core/renderer/css/computed_css_style.h"
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

  /**
   * @brief Resolves the base computed style for an element.
   * @param parent_style The computed style of the parent element (for
   * inheritance).
   * @param previous_computed_style The previous committed computed style of
   * the owning element.
   * @param resolved_style_map Optional output for the resolved style map.
   * @return A unique_ptr to the fully resolved ComputedCSSStyle.
   */
  std::unique_ptr<starlight::ComputedCSSStyle> ResolveBaseStyle(
      const starlight::ComputedCSSStyle* parent_style,
      const starlight::ComputedCSSStyle* previous_computed_style,
      StyleMap* resolved_style_map = nullptr);

  /**
   * @brief In-place variant of ResolveBaseStyle.
   * @param style The ComputedCSSStyle to resolve into (in-out).
   * @param parent_style The computed style of the parent element (for
   * inheritance).
   * @param previous_computed_style The previous committed computed style.
   * @param resolved_style_map Optional output for the resolved style map.
   */
  void ResolveBaseStyleInPlace(
      starlight::ComputedCSSStyle& style,
      const starlight::ComputedCSSStyle* parent_style,
      const starlight::ComputedCSSStyle* previous_computed_style,
      StyleMap* resolved_style_map = nullptr);

  /**
   * @brief Rebuilds the final computed style from parent style, incorporating
   * sampled animation / transition overrides.
   * @param parent_style The parent's computed style for inheritance.
   * @param previous_style The previous committed computed style.
   * @param sampled_custom_property_overrides Sampled custom property overrides
   * from animations.
   * @param sampled_property_overrides Sampled normal property overrides from
   * animations.
   * @param resolved_style_map Optional output for the resolved style map.
   * @return A unique_ptr to the rebuilt final ComputedCSSStyle.
   */
  std::unique_ptr<starlight::ComputedCSSStyle> RebuildFinalStyleFromParent(
      const starlight::ComputedCSSStyle* parent_style,
      const starlight::ComputedCSSStyle* previous_style,
      const CustomPropertiesMap* sampled_custom_property_overrides,
      const StyleMap* sampled_property_overrides,
      StyleMap* resolved_style_map = nullptr);

  /**
   * @brief Fast-path final style construction when the base style is known
   * and only sampled normal-property overrides need to be applied.
   * @param base_style The already-resolved base ComputedCSSStyle.
   * @param sampled_property_overrides Sampled property overrides.
   * @param resolved_style_map Optional output for the resolved style map.
   * @return A unique_ptr to the final ComputedCSSStyle.
   */
  std::unique_ptr<starlight::ComputedCSSStyle> BuildFinalStyleFromBaseFastPath(
      const starlight::ComputedCSSStyle& base_style,
      const StyleMap* sampled_property_overrides,
      StyleMap* resolved_style_map = nullptr);

  /**
   * @brief Computes the property-level diff between two styles.
   * @param new_style The newly resolved style (mutated to record diffs).
   * @param old_style The previously committed style to compare against.
   * @return true if any property changed.
   */
  bool ComputeStyleDiff(starlight::ComputedCSSStyle& new_style,
                        const starlight::ComputedCSSStyle& old_style);

  void HandleCSSVariables(StyleMap& styles);

  void HandlePseudoElement(CSSFragment* fragment);

  /**
   * @brief Resolves pseudo-element styles for the new styling pipeline.
   */
  void ResolvePseudoElementsForNewPipeline(CSSFragment* fragment);
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

  static void ResolveStyleObjectsBasedOnExistingMap(
      const tasm::StyleMap& old_static_style,
      style::StyleObject** new_static_ptr,
      const tasm::StyleMap* old_dynamic_style,
      style::StyleObject* new_dynamic_object, bool static_dirty,
      bool dynamic_dirty, style::SimpleStyleNode* target);

 private:
  enum class PseudoClassType { kFocus, kHover, kActive };

  Element* element() const;
  ElementManager* manager() const;

  void GetCSSStyleNew(AttributeHolder* node, CSSFragment* style_sheet);

  void GetCSSStyleForFiber(FiberElement* node, CSSFragment* style_sheet);

  void DidCollectMatchedRules(AttributeHolder* holder, StyleMap& result,
                              StyleMap& important_result,
                              CSSVariableMap* changed_css_vars = nullptr,
                              size_t base_reserving_size = 0);

  void MergeHigherPriorityCSSStyle(const StyleMap& matched);
  void MergeHigherPriorityImportantCSSStyle(const StyleMap& matched);

  void SetCSSVariableToNode(const CSSVariableMap& matched);

  void MergeToken(CSSParseToken* token);

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
  static thread_local MatchedVector<const StyleMap*>
      matched_important_style_map;
  static thread_local MatchedVector<const CSSVariableMap*> matched_variable_map;

  struct NewPipelineCollectedStyleInputs {
    StyleMap matched_styles;
    StyleMap inline_styles;
    StyleMap matched_important_styles;
    StyleMap inline_important_styles;
    StyleMap attribute_styles;
  };

  // Step 1: InitStyle - Initialize current computed style with parent computed
  // style, copy css property and custom property
  void InitializeStyleShell(starlight::ComputedCSSStyle& shell_style,
                            const starlight::ComputedCSSStyle* previous_style);
  void InheritParentStyle(starlight::ComputedCSSStyle& computed_style,
                          const starlight::ComputedCSSStyle* parent_style);

  // Step 2: Collect matched rules into the legacy TLS vectors. The new styling
  // pipeline keeps them alive until cascading-affecting properties are fully
  // applied, then clears them.
  void CollectMatchedRules(CSSFragment* style_sheet);

  // Step 3.1: Parse matched_style_map and inline_style to style map. Step 3.2:
  // Parse matched_variable_map and inline style to custom properties in
  // ComputedStyle. Shorthand to longhand also happened in this step. Output:
  // style map with highest priority style and custom_properties in
  // ComputedCSSStyle is updated.
  void AnalyzeMatchedResult(
      starlight::ComputedCSSStyle& new_style, StyleMap& result,
      size_t base_reserving_size,
      const CustomPropertiesMap* custom_property_overrides = nullptr,
      const StyleMap* property_overrides = nullptr);

  /**
   * @brief Collects all static (non-animated) style inputs for resolution.
   * @param new_style The computed style being built.
   * @param inputs Output structure for matched, inline, important, and
   * attribute styles.
   * @param base_reserving_size Capacity hint for the result map.
   */
  void CollectStaticStyleInputs(starlight::ComputedCSSStyle& new_style,
                                NewPipelineCollectedStyleInputs& inputs,
                                size_t base_reserving_size);

  /**
   * @brief Finalizes custom properties by applying overrides and resolving
   * var() references.
   * @param new_style The computed style being built.
   * @param inputs Collected style inputs containing raw custom properties.
   * @param custom_property_overrides Optional sampled custom property
   * overrides.
   * @param property_overrides Optional sampled normal property overrides.
   */
  void FinalizeCustomProperties(
      starlight::ComputedCSSStyle& new_style,
      const NewPipelineCollectedStyleInputs& inputs,
      const CustomPropertiesMap* custom_property_overrides = nullptr,
      const StyleMap* property_overrides = nullptr);

  /**
   * @brief Resolves all collected static style inputs into a single StyleMap.
   * @param new_style The computed style being built.
   * @param inputs Collected style inputs (mutated during resolution).
   * @param result Output resolved style map.
   * @param property_overrides Optional sampled property overrides.
   */
  void ResolveCollectedStyleInputs(
      const starlight::ComputedCSSStyle& new_style,
      NewPipelineCollectedStyleInputs& inputs, StyleMap& result,
      const StyleMap* property_overrides = nullptr);

  /**
   * @brief Collects specified styles from matched CSS rules.
   * @param new_style The computed style being built.
   * @param result Output style map.
   * @param base_reserving_size Capacity hint for the result map.
   */
  void CollectMatchedSpecifiedStyles(
      starlight::ComputedCSSStyle& new_style, StyleMap& result,
      size_t base_reserving_size,
      const MatchedVector<const StyleMap*>& matched_style_maps);

  /**
   * @brief Collects inline specified styles from the element.
   * @param new_style The computed style being built.
   * @param result Output style map.
   */
  void CollectInlineSpecifiedStyles(starlight::ComputedCSSStyle& new_style,
                                    StyleMap& result, bool important);

  /**
   * @brief Collects specified styles from element attributes.
   * @param new_style The computed style being built.
   * @param result Output style map.
   */
  void CollectAttributeSpecifiedStyles(starlight::ComputedCSSStyle& new_style,
                                       StyleMap& result);

  /**
   * @brief Collects custom property declarations from matched CSS rules.
   * @param new_style The computed style being built.
   */
  void CollectMatchedCustomProperties(starlight::ComputedCSSStyle& new_style);

  /**
   * @brief Collects inline custom property declarations from the element.
   * @param new_style The computed style being built.
   */
  void CollectInlineCustomProperties(starlight::ComputedCSSStyle& new_style);

  /**
   * @brief Collects custom property declarations from the AttributeHolder.
   * @param new_style The computed style being built.
   */
  void CollectHolderCustomProperties(starlight::ComputedCSSStyle& new_style);

  /**
   * @brief Resolves CSS variable references in a source StyleMap into a result
   * StyleMap.
   * @param new_style The computed style providing custom property values.
   * @param source Input style map that may contain var() references.
   * @param result Output style map with resolved concrete values.
   */
  void ResolveSpecifiedStyleMap(const starlight::ComputedCSSStyle& new_style,
                                StyleMap& source, StyleMap& result);

  // Step 3.2: Cascading-Affecting Properties, apply direction to computed
  // style, may retrigger AnalyzeMatchedResult if direction is inconsist
  void ApplyCascadingAffectingProperties(
      starlight::ComputedCSSStyle& style, StyleMap& style_map,
      const CustomPropertiesMap* custom_property_overrides = nullptr,
      const StyleMap* property_overrides = nullptr);

  // Step 3.3: High-priority Properties, font-size
  void ApplyHighPriorityProperties(starlight::ComputedCSSStyle& style,
                                   const StyleMap& style_map);

  // Step 3.4: Standard Properties
  void ApplyStandardProperties(starlight::ComputedCSSStyle& style,
                               const StyleMap& style_map);

  void ApplyResolvedStyleMap(
      starlight::ComputedCSSStyle& style, StyleMap& style_map,
      const CustomPropertiesMap* custom_property_overrides = nullptr,
      const StyleMap* property_overrides = nullptr);

  std::unique_ptr<starlight::ComputedCSSStyle> CreateInitialComputedStyle(
      const starlight::ComputedCSSStyle* parent_style,
      const starlight::ComputedCSSStyle* previous_style);
  void PrepareInitialComputedStyle(
      starlight::ComputedCSSStyle& style,
      const starlight::ComputedCSSStyle* parent_style,
      const starlight::ComputedCSSStyle* previous_style);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_STYLE_RESOLVER_H_
