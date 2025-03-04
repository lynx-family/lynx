// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_lynx_agent_ng.h"

#include "devtool/lynx_devtool/element/element_helper.h"

namespace lynx {
namespace devtool {

InspectorLynxAgentNG::InspectorLynxAgentNG(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_(devtool_mediator) {
  functions_map_["Lynx.getProperties"] = &InspectorLynxAgentNG::GetProperties;
  functions_map_["Lynx.getData"] = &InspectorLynxAgentNG::GetData;
  functions_map_["Lynx.getComponentId"] = &InspectorLynxAgentNG::GetComponentId;
  functions_map_["Lynx.getRectToWindow"] =
      &InspectorLynxAgentNG::GetLynxViewRectToWindow;
  functions_map_["Lynx.getVersion"] = &InspectorLynxAgentNG::GetLynxVersion;
  functions_map_["Lynx.transferData"] = &InspectorLynxAgentNG::TransferData;
  functions_map_["Lynx.setTraceMode"] = &InspectorLynxAgentNG::SetTraceMode;
  functions_map_["Lynx.getScreenshot"] = &InspectorLynxAgentNG::GetScreenshot;
  functions_map_["Lynx.getViewLocationOnScreen"] =
      &InspectorLynxAgentNG::GetViewLocationOnScreen;
  functions_map_["Lynx.sendVMEvent"] = &InspectorLynxAgentNG::SendVMEvent;
}

InspectorLynxAgentNG::~InspectorLynxAgentNG() = default;

void InspectorLynxAgentNG::CallMethod(
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

// return physical pixel rect of lynx view
void InspectorLynxAgentNG::GetLynxViewRectToWindow(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->LynxGetRectToWindow(sender, message);
}

void InspectorLynxAgentNG::GetProperties(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->LynxGetProperties(sender, message);
}

void InspectorLynxAgentNG::GetData(const std::shared_ptr<MessageSender>& sender,
                                   const Json::Value& message) {
  devtool_mediator_->LynxGetData(sender, message);
}

void InspectorLynxAgentNG::GetComponentId(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->LynxGetComponentId(sender, message);
}

void InspectorLynxAgentNG::GetLynxVersion(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->LynxGetVersion(sender, message);
}

void InspectorLynxAgentNG::TransferData(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->LynxTransferData(sender, message);
}

void InspectorLynxAgentNG::SetTraceMode(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->LynxSetTraceMode(sender, message);
}

void InspectorLynxAgentNG::GetScreenshot(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->GetScreenshot(sender, message);
}

void InspectorLynxAgentNG::GetViewLocationOnScreen(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->LynxGetViewLocationOnScreen(sender, message);
}

void InspectorLynxAgentNG::SendVMEvent(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->LynxSendEventToVM(sender, message);
}

}  // namespace devtool
}  // namespace lynx
