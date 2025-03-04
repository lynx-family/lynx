// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_INSPECTOR_TASM_EXECUTOR_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_INSPECTOR_TASM_EXECUTOR_H_
#include <memory>
#include <set>
#include <unordered_map>

#include "core/inspector/style_sheet.h"
#include "core/renderer/template_assembler.h"
#include "devtool/base_devtool/native/public/devtool_status.h"
#include "devtool/base_devtool/native/public/message_sender.h"
#include "devtool/lynx_devtool/agent/agent_defines.h"
#include "devtool/lynx_devtool/agent/devtool_platform_facade.h"
#include "third_party/jsoncpp/include/json/json.h"

namespace lynx {
namespace tasm {
class Element;
class LayoutNode;
}  // namespace tasm
}  // namespace lynx

namespace lynx {
namespace devtool {

class LynxDevToolMediator;

class InspectorTasmExecutor
    : public std::enable_shared_from_this<InspectorTasmExecutor> {
 public:
  InspectorTasmExecutor(
      const std::shared_ptr<LynxDevToolMediator>& devtool_mediator);
  InspectorTasmExecutor(
      const std::shared_ptr<LynxDevToolMediator>& devtool_mediator,
      const std::shared_ptr<tasm::TemplateAssembler> tasm);

  void SetDevToolPlatformFacade(
      const std::shared_ptr<DevToolPlatformFacade>& devtool_platform_facade);

 public:
  // event
  enum class DomCdpEvent {
    DOCUMENT_UPDATED,
    CHILD_NODE_REMOVED,
    ATTRIBUTE_MODIFIED,
    ATTRIBUTE_REMOVED
  };
  enum class CssCdpEvent {
    STYLE_SHEET_ADDED,
    STYLE_SHEET_REMOVED,
    STYLE_SHEET_CHANGED
  };
  void SendDOMEventMsg(const DomCdpEvent& event_name, int nodeId,
                       const std::string& name, int parentNodeId);
  void SendCSSEventMsg(const CssCdpEvent& event_name,
                       const std::string& style_sheet_id,
                       lynx::tasm::Element* ptr);

 public:
  void OnDocumentUpdated();
  void OnElementNodeAdded(lynx::tasm::Element* ptr);
  void OnElementNodeRemoved(lynx::tasm::Element* ptr);
  void OnCharacterDataModified(lynx::tasm::Element* ptr);
  void OnElementDataModelSet(lynx::tasm::Element* ptr);
  void OnElementManagerWillDestroy();
  void OnSetNativeProps(lynx::tasm::Element* ptr, const std::string& name,
                        const std::string& value, bool is_style);
  void OnCSSStyleSheetAdded(lynx::tasm::Element* ptr);

 public:
  // dom related
  void DiffID(lynx::tasm::Element* ptr);
  void DiffAttr(lynx::tasm::Element* ptr);
  void DiffClass(lynx::tasm::Element* ptr);
  void DiffStyle(lynx::tasm::Element* ptr);

 public:
  // dom domain
  DECLARE_DEVTOOL_METHOD(QuerySelector)
  DECLARE_DEVTOOL_METHOD(GetAttributes)
  DECLARE_DEVTOOL_METHOD(InnerText)
  DECLARE_DEVTOOL_METHOD(QuerySelectorAll)
  DECLARE_DEVTOOL_METHOD(DOM_Enable)
  DECLARE_DEVTOOL_METHOD(DOM_Disable)
  DECLARE_DEVTOOL_METHOD(GetDocument)
  DECLARE_DEVTOOL_METHOD(GetDocumentWithBoxModel)
  DECLARE_DEVTOOL_METHOD(RequestChildNodes)
  DECLARE_DEVTOOL_METHOD(DOM_GetBoxModel)
  DECLARE_DEVTOOL_METHOD(DOMEnableDomTree);
  DECLARE_DEVTOOL_METHOD(DOMDisableDomTree);
  DECLARE_DEVTOOL_METHOD(SetAttributesAsText)
  DECLARE_DEVTOOL_METHOD(MarkUndoableState)
  DECLARE_DEVTOOL_METHOD(PushNodesByBackendIdsToFrontend)
  DECLARE_DEVTOOL_METHOD(RemoveNode)
  DECLARE_DEVTOOL_METHOD(MoveTo)
  DECLARE_DEVTOOL_METHOD(CopyTo)
  DECLARE_DEVTOOL_METHOD(GetOuterHTML)
  DECLARE_DEVTOOL_METHOD(SetOuterHTML)
  DECLARE_DEVTOOL_METHOD(SetInspectedNode)
  DECLARE_DEVTOOL_METHOD(PerformSearch)
  DECLARE_DEVTOOL_METHOD(GetSearchResults)
  DECLARE_DEVTOOL_METHOD(DiscardSearchResults)
  DECLARE_DEVTOOL_METHOD(GetOriginalNodeIndex)

  // css domain
  DECLARE_DEVTOOL_METHOD(CSS_Enable)
  DECLARE_DEVTOOL_METHOD(CSS_Disable)
  DECLARE_DEVTOOL_METHOD(GetMatchedStylesForNode)
  DECLARE_DEVTOOL_METHOD(GetComputedStyleForNode)
  DECLARE_DEVTOOL_METHOD(GetInlineStylesForNode)
  DECLARE_DEVTOOL_METHOD(SetStyleTexts)
  DECLARE_DEVTOOL_METHOD(GetStyleSheetText)
  DECLARE_DEVTOOL_METHOD(GetBackgroundColors)
  DECLARE_DEVTOOL_METHOD(SetStyleSheetText)
  DECLARE_DEVTOOL_METHOD(CreateStyleSheet)
  DECLARE_DEVTOOL_METHOD(AddRule)
  DECLARE_DEVTOOL_METHOD(StartRuleUsageTracking)
  DECLARE_DEVTOOL_METHOD(UpdateRuleUsageTracking)
  DECLARE_DEVTOOL_METHOD(StopRuleUsageTracking)

  // overlay
  DECLARE_DEVTOOL_METHOD(HighlightNode)
  DECLARE_DEVTOOL_METHOD(HideHighlight)

  // layer tree
  DECLARE_DEVTOOL_METHOD(LayerTreeEnable)
  DECLARE_DEVTOOL_METHOD(LayerTreeDisable)
  DECLARE_DEVTOOL_METHOD(LayerPainted)
  DECLARE_DEVTOOL_METHOD(CompositingReasons)

  // page domain
  DECLARE_DEVTOOL_METHOD(PageGetResourceContent)

  // Lynx domain
  DECLARE_DEVTOOL_METHOD(LynxGetProperties)
  DECLARE_DEVTOOL_METHOD(LynxGetData)
  DECLARE_DEVTOOL_METHOD(LynxGetComponentId)
  DECLARE_DEVTOOL_METHOD(TemplateGetTemplateApiInfo)

  // DOM ScrollIntoViewIfNeeded
  DECLARE_DEVTOOL_METHOD(ScrollIntoViewIfNeeded)

 public:
  void LayerTreeDidChange(
      const std::shared_ptr<lynx::devtool::MessageSender>& sender);
  std::vector<double> GetBoxModel(tasm::Element* element);

  Json::Value GetLayerContentFromElement(
      const std::shared_ptr<lynx::devtool::MessageSender>& sender,
      lynx::tasm::Element* element);

  Json::Value GetLayoutInfoFromElement(
      const std::shared_ptr<lynx::devtool::MessageSender>& sender,
      lynx::tasm::Element* element);

  Json::Value BuildLayerTreeFromElement(
      const std::shared_ptr<lynx::devtool::MessageSender>& sender,
      lynx::tasm::Element* root_element);

 public:
  lynx::tasm::Element* GetElementById(int node_id);
  lynx::tasm::Element* GetElementRoot();
  Json::Value GetUsageItem(const std::string& stylesheet_id,
                           const std::string& content,
                           const std::string& selector);
  void CollectDomTreeCssUsage(Json::Value& rule_usage_array,
                              const std::string& stylesheet_id,
                              const std::string& content);
  std::string GetLayoutTree(tasm::Element* element);
  void SendLayoutTree();
  Json::Value GetBoxModelOfNode(tasm::Element* ptr, double screen_scale_factor,
                                std::string mode, tasm::Element* root);
  Json::Value GetDocumentBodyFromNodeWithBoxModel(tasm::Element* ptr);
  Json::Value GetComputedStyleOfNode(tasm::Element* ptr);
  void RestoreOriginNodeInlineStyle();
  const std::map<DevToolFunction, std::function<void(const lynx::base::any&)>>&
  GetFunctionForElementMap();

 private:
  bool dom_use_compression_;
  int dom_compression_threshold_;
  size_t origin_node_id_ = 0;

  bool rule_usage_tracking_;
  bool layer_tree_enabled_ = false;
  lynx::tasm::Element* element_root_;
  std::weak_ptr<tasm::TemplateAssembler> tasm_;
  std::unordered_map<int32_t, lynx::tasm::LayoutNode*> layout_nodes_;
  std::weak_ptr<LynxDevToolMediator> devtool_mediator_wp_;
  std::unordered_map<uint64_t, std::vector<int>> search_results_;
  lynx::devtool::InspectorStyleSheet origin_inline_style_;
  std::shared_ptr<DevToolPlatformFacade> devtool_platform_facade_;

  std::set<std::string> css_used_selector_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_INSPECTOR_TASM_EXECUTOR_H_
