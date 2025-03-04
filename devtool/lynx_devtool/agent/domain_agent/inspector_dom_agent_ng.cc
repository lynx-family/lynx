// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_dom_agent_ng.h"

namespace lynx {
namespace devtool {

InspectorDOMAgentNG::InspectorDOMAgentNG(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_(devtool_mediator) {
  functions_map_["DOM.enable"] = &InspectorDOMAgentNG::Enable;
  functions_map_["DOM.disable"] = &InspectorDOMAgentNG::Disable;
  functions_map_["DOM.enableDomTree"] = &InspectorDOMAgentNG::EnableDomTree;
  functions_map_["DOM.disableDomTree"] = &InspectorDOMAgentNG::DisableDomTree;
  functions_map_["DOM.getDocument"] = &InspectorDOMAgentNG::GetDocument;
  functions_map_["DOM.getDocumentWithBoxModel"] =
      &InspectorDOMAgentNG::GetDocumentWithBoxModel;
  functions_map_["DOM.requestChildNodes"] =
      &InspectorDOMAgentNG::RequestChildNodes;
  functions_map_["DOM.getBoxModel"] = &InspectorDOMAgentNG::GetBoxModel;
  functions_map_["DOM.setAttributesAsText"] =
      &InspectorDOMAgentNG::SetAttributesAsText;
  functions_map_["DOM.markUndoableState"] =
      &InspectorDOMAgentNG::MarkUndoableState;
  functions_map_["DOM.getNodeForLocation"] =
      &InspectorDOMAgentNG::GetNodeForLocation;
  functions_map_["DOM.pushNodesByBackendIdsToFrontend"] =
      &InspectorDOMAgentNG::PushNodesByBackendIdsToFrontend;
  functions_map_["DOM.removeNode"] = &InspectorDOMAgentNG::RemoveNode;
  functions_map_["DOM.moveTo"] = &InspectorDOMAgentNG::MoveTo;
  functions_map_["DOM.copyTo"] = &InspectorDOMAgentNG::CopyTo;
  functions_map_["DOM.getOuterHTML"] = &InspectorDOMAgentNG::GetOuterHTML;
  functions_map_["DOM.setOuterHTML"] = &InspectorDOMAgentNG::SetOuterHTML;
  functions_map_["DOM.setInspectedNode"] =
      &InspectorDOMAgentNG::SetInspectedNode;
  functions_map_["DOM.querySelector"] = &InspectorDOMAgentNG::QuerySelector;
  functions_map_["DOM.querySelectorAll"] =
      &InspectorDOMAgentNG::QuerySelectorAll;
  functions_map_["DOM.innerText"] = &InspectorDOMAgentNG::InnerText;
  functions_map_["DOM.getAttributes"] = &InspectorDOMAgentNG::GetAttributes;
  functions_map_["DOM.performSearch"] = &InspectorDOMAgentNG::PerformSearch;
  functions_map_["DOM.getSearchResults"] =
      &InspectorDOMAgentNG::GetSearchResults;
  functions_map_["DOM.discardSearchResults"] =
      &InspectorDOMAgentNG::DiscardSearchResults;
  functions_map_["DOM.scrollIntoViewIfNeeded"] =
      &InspectorDOMAgentNG::ScrollIntoViewIfNeeded;
  functions_map_["DOM.getOriginalNodeIndex"] =
      &InspectorDOMAgentNG::GetOriginalNodeIndex;
}

InspectorDOMAgentNG::~InspectorDOMAgentNG() = default;

void InspectorDOMAgentNG::QuerySelector(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->QuerySelector(sender, message);
}

void InspectorDOMAgentNG::GetAttributes(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->GetAttributes(sender, message);
}

void InspectorDOMAgentNG::InnerText(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->InnerText(sender, message);
}

void InspectorDOMAgentNG::QuerySelectorAll(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->QuerySelectorAll(sender, message);
}

void InspectorDOMAgentNG::Enable(const std::shared_ptr<MessageSender>& sender,
                                 const Json::Value& message) {
  devtool_mediator_->DOM_Enable(sender, message);
}

void InspectorDOMAgentNG::Disable(const std::shared_ptr<MessageSender>& sender,
                                  const Json::Value& message) {
  devtool_mediator_->DOM_Disable(sender, message);
}

void InspectorDOMAgentNG::EnableDomTree(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->DOMEnableDomTree(sender, message);
}

void InspectorDOMAgentNG::DisableDomTree(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->DOMDisableDomTree(sender, message);
}

void InspectorDOMAgentNG::GetDocument(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->GetDocument(sender, message);
}

void InspectorDOMAgentNG::GetDocumentWithBoxModel(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->GetDocumentWithBoxModel(sender, message);
}

void InspectorDOMAgentNG::RequestChildNodes(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->RequestChildNodes(sender, message);
}

void InspectorDOMAgentNG::GetBoxModel(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->DOM_GetBoxModel(sender, message);
}

void InspectorDOMAgentNG::SetAttributesAsText(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->SetAttributesAsText(sender, message);
}

void InspectorDOMAgentNG::MarkUndoableState(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->MarkUndoableState(sender, message);
}

void InspectorDOMAgentNG::GetNodeForLocation(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->GetNodeForLocation(sender, message);
}

void InspectorDOMAgentNG::PushNodesByBackendIdsToFrontend(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->PushNodesByBackendIdsToFrontend(sender, message);
}

void InspectorDOMAgentNG::RemoveNode(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->RemoveNode(sender, message);
}

void InspectorDOMAgentNG::MoveTo(const std::shared_ptr<MessageSender>& sender,
                                 const Json::Value& message) {
  devtool_mediator_->MoveTo(sender, message);
}

void InspectorDOMAgentNG::CopyTo(const std::shared_ptr<MessageSender>& sender,
                                 const Json::Value& message) {
  devtool_mediator_->CopyTo(sender, message);
}

void InspectorDOMAgentNG::GetOuterHTML(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->GetOuterHTML(sender, message);
}

void InspectorDOMAgentNG::SetOuterHTML(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->SetOuterHTML(sender, message);
}

void InspectorDOMAgentNG::SetInspectedNode(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->SetInspectedNode(sender, message);
}

void InspectorDOMAgentNG::PerformSearch(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->PerformSearch(sender, message);
}

void InspectorDOMAgentNG::GetSearchResults(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->GetSearchResults(sender, message);
}

void InspectorDOMAgentNG::DiscardSearchResults(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->DiscardSearchResults(sender, message);
}

void InspectorDOMAgentNG::ScrollIntoViewIfNeeded(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->ScrollIntoViewIfNeeded(sender, message);
}

void InspectorDOMAgentNG::GetOriginalNodeIndex(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->GetOriginalNodeIndex(sender, message);
}

void InspectorDOMAgentNG::CallMethod(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  std::string method = message["method"].asString();
  auto iter = functions_map_.find(method);
  if (iter == functions_map_.end()) {
    Json::Value res;
    res["error"] = Json::ValueType::objectValue;
    res["error"]["code"] = kInspectorErrorCode;
    res["error"]["message"] = "Not implemented: " + method;
    res["id"] = message["id"].asInt64();
    sender->SendMessage("CDP", res);
  } else {
    (this->*(iter->second))(sender, message);
  }
}
}  // namespace devtool
}  // namespace lynx
