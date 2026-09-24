// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_ui_tree_agent.h"

#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

namespace lynx {
namespace devtool {

InspectorUITreeAgent::InspectorUITreeAgent(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_(devtool_mediator) {
  functions_map_["UITree.enable"] = &InspectorUITreeAgent::Enable;
  functions_map_["UITree.disable"] = &InspectorUITreeAgent::Disable;
  functions_map_["UITree.getLynxUITree"] = &InspectorUITreeAgent::GetLynxUITree;
  legacy_functions_map_["UITree.getUIInfoForNode"] =
      &InspectorUITreeAgent::GetUIInfoForNode;
  legacy_functions_map_["UITree.setUIStyle"] =
      &InspectorUITreeAgent::SetUIStyle;
}

void InspectorUITreeAgent::Enable(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->UITree_Enable(responder, params);
}

void InspectorUITreeAgent::Disable(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->UITree_Disable(responder, params);
}

void InspectorUITreeAgent::GetLynxUITree(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->GetLynxUITree(responder, params);
}

void InspectorUITreeAgent::GetUIInfoForNode(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->GetUIInfoForNode(sender, message);
}

void InspectorUITreeAgent::SetUIStyle(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->SetUIStyle(sender, message);
}

void InspectorUITreeAgent::CallMethod(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  std::string method = message["method"].asString();
  auto iter = legacy_functions_map_.find(method);
  if (iter == legacy_functions_map_.end()) {
    SendNotImplementedResponse(sender, message["id"].asInt64(), method);
  } else {
    (this->*(iter->second))(sender, message);
  }
}

void InspectorUITreeAgent::CallMethod(
    const std::shared_ptr<CDPResponder>& responder,
    const Json::Value& message) {
  const std::string method = message["method"].asString();
  const auto iter = functions_map_.find(method);
  if (iter == functions_map_.end()) {
    // Fall back only while patch2 commands still use MessageSender.
    CallMethod(responder->RetrieveSender(), message);
    return;
  }
  (this->*(iter->second))(responder, message["params"]);
}

}  // namespace devtool
}  // namespace lynx
