// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_ELEMENT_ELEMENT_HELPER_H_
#define DEVTOOL_LYNX_DEVTOOL_ELEMENT_ELEMENT_HELPER_H_

#include <memory>
#include <set>

#include "core/inspector/style_sheet.h"
#include "core/renderer/dom/element.h"
#include "devtool/lynx_devtool/base/screen_metadata.h"
#include "devtool/lynx_devtool/element/element_inspector.h"
#include "third_party/jsoncpp/include/json/json.h"

using lynx::tasm::Element;

namespace lynx {
namespace devtool {

extern const char* kLynxLocalUrl;
extern const char* kLynxSecurityOrigin;
extern const char* kLynxMimeType;

extern const char* kPaddingCurlyBrackets;

class ElementHelper {
 public:
  static Element* GetPreviousNode(Element* ptr);

  static Json::Value GetDocumentBodyFromNode(Element* ptr);
  static void SetJsonValueOfNode(Element* ptr, Json::Value& value);
  static Json::Value GetMatchedStylesForNode(Element* ptr);
  static Json::Value GetKeyframesRulesForNode(Element* ptr);
  static std::pair<bool, Json::Value> GetKeyframesRule(const std::string& name,
                                                       Element* ptr);
  static void FillKeyFramesRule(
      Element* ptr,
      const std::unordered_multimap<std::string, CSSPropertyDetail>&
          css_property,
      Json::Value& content, std::set<std::string>& animation_name_set,
      const std::string& key);
  static void FillKeyFramesRuleByStyleSheet(
      Element* ptr, const InspectorStyleSheet& style_sheet,
      Json::Value& content, std::set<std::string>& animation_name_set);
  static Json::Value GetInlineStyleOfNode(Element* ptr);
  static Json::Value GetBackGroundColorsOfNode(Element* ptr);
  static Json::Value GetMatchedCSSRulesOfNode(Element* ptr);
  static void ApplyCascadeStyles(Element* ptr, Json::Value& result,
                                 const std::string& rule);
  static void ApplyPseudoCascadeStyles(Element* ptr, Json::Value& result,
                                       const std::string& rule);
  static std::string GetPseudoChildNameForStyle(
      const std::string& rule, const std::string& pseudo_child);
  static void ApplyPseudoChildStyle(Element* ptr, Json::Value& result,
                                    const std::string& rule);
  static Json::Value GetInheritedCSSRulesOfNode(Element* ptr);

  static Json::Value GetAttributesImpl(Element* ptr);
  static Json::Value GetAttributesAsTextOfNode(Element* ptr,
                                               const std::string& name);
  static Json::Value GetStyleSheetAsText(
      const InspectorStyleSheet& style_sheet);
  static Json::Value GetStyleSheetAsTextOfNode(
      Element* ptr, const std::string& style_sheet_id, const Range& range);
  static Json::Value GetStyleSheetText(Element* ptr,
                                       const std::string& style_sheet_id);

  static void SetInlineStyleTexts(Element* ptr, const std::string& text,
                                  const Range& range);
  static void SetInlineStyleSheet(Element* ptr,
                                  const InspectorStyleSheet& style_sheet);
  static void SetSelectorStyleTexts(Element* root, Element* ptr,
                                    const std::string& text,
                                    const Range& range);
  static bool GetElementPtrMatchingForCascadedStyleSheet(
      std::vector<lynx::tasm::Element*>& res, lynx::tasm::Element* root,
      const std::string& name, const std::string& style_sheet_name);
  static void GetElementPtrMatchingStyleSheet(
      std::vector<Element*>& res, Element* root,
      const std::string& style_sheet_name);
  static void SetStyleTexts(Element* root, Element* ptr,
                            const std::string& text, const Range& range);
  static void SetAttributes(Element* ptr, const std::string& name,
                            const std::string& text);
  static void RemoveAttributes(Element* ptr, const std::string& name);
  static void SetOuterHTML(Element* manager, int indexId, std::string html);

  static std::vector<Json::Value> SetAttributesAsText(Element* ptr,
                                                      std::string name,
                                                      std::string text);
  static std::string GetElementContent(Element* ptr, int num);
  static std::string GetStyleNodeText(Element* ptr);
  static Json::Value GetStyleSheetHeader(Element* ptr);
  static Json::Value CreateStyleSheet(Element* ptr);
  static InspectorStyleSheet GetInlineStyleTexts(Element* ptr);
  static Json::Value AddRule(Element* ptr, const std::string& style_sheet_id,
                             const std::string& rule_text, const Range& range);
  static int QuerySelector(Element* ptr, const std::string& selector);
  static Json::Value QuerySelectorAll(Element* ptr,
                                      const std::string& selector);
  static std::string GetProperties(Element* ptr);
  static std::string GetData(Element* ptr);
  static std::string GetComponentId(Element* ptr);
  static void PerformSearchFromNode(Element* ptr, std::string& query,
                                    std::vector<int>& results);
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_ELEMENT_ELEMENT_HELPER_H_
