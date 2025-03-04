// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_ELEMENT_ELEMENT_INSPECTOR_H_
#define DEVTOOL_LYNX_DEVTOOL_ELEMENT_ELEMENT_INSPECTOR_H_

#include <tuple>

#include "core/base/utils/any.h"
#include "core/renderer/dom/css_patching.h"
#include "core/renderer/dom/element.h"
#include "devtool/lynx_devtool/agent/inspector_util.h"

using lynx::tasm::Element;
using lynx::tasm::LayoutNode;

namespace lynx {
namespace tasm {

class CSSFragment;

}
}  // namespace lynx

namespace lynx {
namespace devtool {

class ElementInspector {
 public:
  static int NodeId(Element* element) {
    CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", -1);
    return element->impl_id();
  }
  static int NodeType(Element* element) {
    CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", -1);
    auto* inspector_attribute = element->inspector_attribute();
    CHECK_NULL_AND_LOG_RETURN_VALUE(inspector_attribute,
                                    "inspector_attribute is null", -1);
    return inspector_attribute->node_type_;
  }
  static std::string LocalName(Element* element) {
    CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", "");
    auto* inspector_attribute = element->inspector_attribute();
    CHECK_NULL_AND_LOG_RETURN_VALUE(inspector_attribute,
                                    "inspector_attribute is null", "");
    return inspector_attribute->local_name_;
  }
  static std::string NodeName(Element* element) {
    CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", "");
    auto* inspector_attribute = element->inspector_attribute();
    CHECK_NULL_AND_LOG_RETURN_VALUE(inspector_attribute,
                                    "inspector_attribute is null", "");
    return inspector_attribute->node_name_;
  }
  static std::string NodeValue(Element* element) {
    CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", "");
    auto* inspector_attribute = element->inspector_attribute();
    CHECK_NULL_AND_LOG_RETURN_VALUE(inspector_attribute,
                                    "inspector_attribute is null", "");
    return inspector_attribute->node_value_;
  }
  static std::string SelectorId(Element* element) {
    CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", "");
    auto* inspector_attribute = element->inspector_attribute();
    CHECK_NULL_AND_LOG_RETURN_VALUE(inspector_attribute,
                                    "inspector_attribute is null", "");
    return inspector_attribute->selector_id_;
  }
  static std::string SelectorTag(Element* element) {
    CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", "");
    return element->GetTag().str();
  }
  static std::vector<std::string> ClassOrder(Element* element) {
    CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", {});
    auto* inspector_attribute = element->inspector_attribute();
    CHECK_NULL_AND_LOG_RETURN_VALUE(inspector_attribute,
                                    "inspector_attribute is null", {});
    return inspector_attribute->class_order_;
  }
  static lynx::devtool::InspectorElementType Type(Element* element) {
    CHECK_NULL_AND_LOG_RETURN_VALUE(
        element, "element is null",
        lynx::devtool::InspectorElementType::DOCUMENT);
    auto* inspector_attribute = element->inspector_attribute();
    CHECK_NULL_AND_LOG_RETURN_VALUE(
        inspector_attribute, "inspector_attribute is null",
        lynx::devtool::InspectorElementType::DOCUMENT);
    return inspector_attribute->type_;
  }
  static std::vector<std::string>& AttrOrder(Element* element) {
    return element->inspector_attribute()->attr_order_;
  }
  static std::unordered_map<std::string, std::string>& AttrMap(
      Element* element) {
    return element->inspector_attribute()->attr_map_;
  }
  static std::vector<std::string>& DataOrder(Element* element) {
    return element->inspector_attribute()->data_order_;
  }
  static std::unordered_map<std::string, std::string>& DataMap(
      Element* element) {
    return element->inspector_attribute()->data_map_;
  }
  static std::vector<std::string>& EventOrder(Element* element) {
    return element->inspector_attribute()->event_order_;
  }
  static std::unordered_map<std::string, std::string>& EventMap(
      Element* element) {
    return element->inspector_attribute()->event_map_;
  }
  static Element* StyleRoot(Element* element) {
    CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", nullptr);
    auto* inspector_attribute = element->inspector_attribute();
    CHECK_NULL_AND_LOG_RETURN_VALUE(inspector_attribute,
                                    "inspector_attribute is null", nullptr);
    return inspector_attribute->style_root_;
  }
  static lynx::devtool::InspectorStyleSheet& GetInlineStyleSheet(
      Element* element) {
    return element->inspector_attribute()->inline_style_sheet_;
  }
  static std::vector<lynx::devtool::InspectorCSSRule>& GetCssRules(
      Element* element) {
    return element->inspector_attribute()->css_rules_;
  }
  static std::unordered_multimap<std::string,
                                 lynx::devtool::InspectorStyleSheet>&
  GetStyleSheetMap(Element* element) {
    return element->inspector_attribute()->style_sheet_map_;
  }
  static const std::unordered_map<
      std::string, std::vector<lynx::devtool::InspectorKeyframe>>&
  GetAnimationMap(Element* element) {
    return element->inspector_attribute()->animation_map_;
  }
  static void SetInlineStyleSheet(
      Element* element, const lynx::devtool::InspectorStyleSheet& style) {
    CHECK_NULL_AND_LOG_RETURN(element, "element is null");
    auto* inspector_attribute = element->inspector_attribute();
    CHECK_NULL_AND_LOG_RETURN(inspector_attribute,
                              "inspector_attribute is null");
    inspector_attribute->inline_style_sheet_ = style;
  }
  static void SetClassOrder(Element* element,
                            const std::vector<std::string>& class_order) {
    CHECK_NULL_AND_LOG_RETURN(element, "element is null");
    auto* inspector_attribute = element->inspector_attribute();
    CHECK_NULL_AND_LOG_RETURN(inspector_attribute,
                              "inspector_attribute is null");
    inspector_attribute->class_order_ = class_order;
  }
  static void SetSelectorId(Element* element, const std::string& selector_id) {
    CHECK_NULL_AND_LOG_RETURN(element, "element is null");
    auto* inspector_attribute = element->inspector_attribute();
    CHECK_NULL_AND_LOG_RETURN(inspector_attribute,
                              "inspector_attribute is null");
    inspector_attribute->selector_id_ = selector_id;
  }
  static void SetAttrOrder(Element* element,
                           const std::vector<std::string>& attr_order) {
    CHECK_NULL_AND_LOG_RETURN(element, "element is null");
    auto* inspector_attribute = element->inspector_attribute();
    CHECK_NULL_AND_LOG_RETURN(inspector_attribute,
                              "inspector_attribute is null");
    inspector_attribute->attr_order_ = attr_order;
  }
  static void SetAttrMap(
      Element* element,
      std::unordered_map<std::string, std::string>& attr_map) {
    CHECK_NULL_AND_LOG_RETURN(element, "element is null");
    auto* inspector_attribute = element->inspector_attribute();
    CHECK_NULL_AND_LOG_RETURN(inspector_attribute,
                              "inspector_attribute is null");
    inspector_attribute->attr_map_ = attr_map;
  }
  static void SetDataOrder(Element* element,
                           const std::vector<std::string>& data_order) {
    CHECK_NULL_AND_LOG_RETURN(element, "element is null");
    auto* inspector_attribute = element->inspector_attribute();
    CHECK_NULL_AND_LOG_RETURN(inspector_attribute,
                              "inspector_attribute is null");
    inspector_attribute->data_order_ = data_order;
  }
  static void SetDataMap(
      Element* element,
      std::unordered_map<std::string, std::string>& data_map) {
    CHECK_NULL_AND_LOG_RETURN(element, "element is null");
    auto* inspector_attribute = element->inspector_attribute();
    CHECK_NULL_AND_LOG_RETURN(inspector_attribute,
                              "inspector_attribute is null");
    inspector_attribute->data_map_ = data_map;
  }
  static void SetEventOrder(Element* element,
                            const std::vector<std::string>& event_order) {
    CHECK_NULL_AND_LOG_RETURN(element, "element is null");
    auto* inspector_attribute = element->inspector_attribute();
    CHECK_NULL_AND_LOG_RETURN(inspector_attribute,
                              "inspector_attribute is null");
    inspector_attribute->event_order_ = event_order;
  }
  static void SetEventMap(
      Element* element,
      std::unordered_map<std::string, std::string>& event_map) {
    CHECK_NULL_AND_LOG_RETURN(element, "element is null");
    auto* inspector_attribute = element->inspector_attribute();
    CHECK_NULL_AND_LOG_RETURN(inspector_attribute,
                              "inspector_attribute is null");
    inspector_attribute->event_map_ = event_map;
  }
  static Element* DocElement(Element* element) {
    CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", nullptr);
    auto* inspector_attribute = element->inspector_attribute();
    CHECK_NULL_AND_LOG_RETURN_VALUE(inspector_attribute,
                                    "inspector_attribute is null", nullptr);
    return inspector_attribute->doc_.get();
  }
  static Element* StyleValueElement(Element* element) {
    CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", nullptr);
    auto* inspector_attribute = element->inspector_attribute();
    CHECK_NULL_AND_LOG_RETURN_VALUE(inspector_attribute,
                                    "inspector_attribute is null", nullptr);
    return inspector_attribute->style_value_.get();
  }
  static void SetDocElement(const lynx::base::any& data);
  static void SetStyleValueElement(const lynx::base::any& data);
  static bool IsNeedEraseId(Element* element) {
    CHECK_NULL_AND_LOG_RETURN_VALUE(element, "element is null", false);
    auto* inspector_attribute = element->inspector_attribute();
    CHECK_NULL_AND_LOG_RETURN_VALUE(inspector_attribute,
                                    "inspector_attribute is null", false);
    return inspector_attribute->needs_erase_id_;
  }
  static void SetIsNeedEraseId(Element* element, bool needs_erase_id) {
    CHECK_NULL_AND_LOG_RETURN(element, "element is null");
    auto* inspector_attribute = element->inspector_attribute();
    CHECK_NULL_AND_LOG_RETURN(inspector_attribute,
                              "inspector_attribute is null");
    inspector_attribute->needs_erase_id_ = needs_erase_id;
  }

  static bool HasDataModel(Element* element);

  static void InitForInspector(const lynx::base::any& data);
  static void InitTypeForInspector(Element* element);
  static void InitInlineStyleSheetForInspector(Element* element);
  static void InitIdForInspector(Element* element);
  static void InitClassForInspector(Element* element);
  static void InitAttrForInspector(Element* element);
  static void InitDataSetForInspector(Element* element);
  static void InitEventMapForInspector(Element* element);
  static void InitPlugForInspector(const lynx::base::any& data);

  static void InitDocumentElement(Element* element);
  static void InitComponentElement(Element* element);
  static void InitStyleValueElement(const lynx::base::any& data);
  static void InitNormalElement(Element* element);

  static lynx::devtool::InspectorStyleSheet InitStyleSheet(
      Element* element, int start_line, std::string name,
      std::unordered_map<std::string, std::string> styles,
      uint64_t position = 0);

  static Element* GetParentComponentElementFromDataModel(Element* element);
  static Element* GetChildElementForComponentRemoveView(Element* element);

  static void Flush(Element* element);
  static void InitStyleRoot(const lynx::base::any& data);
  static void SetStyleRoot(const lynx::base::any& data);

  static std::unordered_map<std::string, std::string> GetCssByStyleMap(
      Element* element, const lynx::tasm::StyleMap& style_map);
  static std::unordered_map<std::string, std::string> GetCssVariableByMap(
      const lynx::tasm::CSSVariableMap& style_variables);
  static std::unordered_map<std::string, std::string> GetCSSByName(
      Element* element, std::string name);
  static std::unordered_map<std::string, std::string> GetCSSByParseToken(
      Element* element, lynx::tasm::CSSParseToken* token);
  static std::vector<lynx::devtool::InspectorStyleSheet> GetMatchedStyleSheet(
      Element* element);
  static lynx::devtool::LynxDoubleMapString GetAnimationByName(
      Element* element, std::string name);
  static lynx::devtool::InspectorStyleSheet GetStyleSheetByName(
      Element* element, const std::string& name);
  static std::vector<lynx::devtool::InspectorKeyframe>
  GetAnimationKeyframeByName(Element* element, const std::string& name);

  static std::string GetVirtualSlotName(Element* slot_plug);
  static std::string GetComponentName(Element* element);
  static Element* GetElementByID(Element* element, int id);

  /**
   * Helper function to get Element's corresponding CSSFragment. If element is
   * not component/page, return nullptr.
   * @param element component's element
   */
  static Element* GetCSSStyleComponentElement(Element* element);

  static std::string GetComponentProperties(Element* element);
  static std::string GetComponentData(Element* element);
  static std::string GetComponentId(Element* element);

  static std::unordered_map<std::string, std::string>
  GetInlineStylesFromAttributeHolder(Element* element);
  static std::string GetSelectorIDFromAttributeHolder(Element* element);
  static std::vector<std::string> GetClassOrderFromAttributeHolder(
      Element* element);

  static lynx::devtool::LynxAttributePair GetAttrFromAttributeHolder(
      Element* element);

  static lynx::devtool::LynxAttributePair GetDataSetFromAttributeHolder(
      Element* element);
  static lynx::devtool::LynxAttributePair GetEventMapFromAttributeHolder(
      Element* element);

  static void SetPropsAccordingToStyleSheet(
      Element* element, const lynx::devtool::InspectorStyleSheet& style_sheet);
  static void SetPropsForCascadedStyleSheet(Element* element,
                                            const std::string& rule);
  static void AdjustStyleSheet(Element* element);
  static void DeleteStyleFromInlineStyleSheet(Element* element,
                                              const std::string& name);
  static void UpdateStyleToInlineStyleSheet(Element* element,
                                            const std::string& name,
                                            const std::string& value);
  static void DeleteStyle(Element* element, const std::string& name);
  static void UpdateStyle(Element* element, const std::string& name,
                          const std::string& value);
  static void DeleteAttr(Element* element, const std::string& name);
  static void UpdateAttr(Element* element, const std::string& name,
                         const std::string& value);
  static void DeleteClasses(Element* element);
  static void UpdateClasses(Element* element,
                            const std::vector<std::string> classes);
  static void SetStyleSheetByName(
      Element* element, const std::string& name,
      const lynx::devtool::InspectorStyleSheet& style_sheet);
  static bool IsStyleRootHasCascadeStyle(Element* element);
  static bool IsEnableCSSSelector(Element* element);
  static bool IsEnableCSSInheritance(Element* element);

  static std::unordered_map<std::string, std::string> GetDefaultCss();
  static std::vector<double> GetOverlayNGBoxModel(Element* element);
  static std::vector<float> GetRectToWindow(Element* element);

  static std::vector<Element*> SelectElementAll(Element* element,
                                                const std::string& selector);
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_ELEMENT_ELEMENT_INSPECTOR_H_
