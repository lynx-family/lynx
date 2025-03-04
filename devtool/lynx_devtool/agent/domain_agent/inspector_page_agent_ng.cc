// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_page_agent_ng.h"

#include "devtool/lynx_devtool/element/element_helper.h"

namespace lynx {
namespace devtool {

#define BANNER ""

InspectorPageAgentNG::InspectorPageAgentNG(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_(devtool_mediator) {
  functions_map_["Page.enable"] = &InspectorPageAgentNG::Enable;
  functions_map_["Page.canEmulate"] = &InspectorPageAgentNG::CanEmulate;
  functions_map_["Page.canScreencast"] = &InspectorPageAgentNG::CanScreencast;
  functions_map_["Page.getResourceTree"] =
      &InspectorPageAgentNG::GetResourceTree;
  functions_map_["Page.getResourceContent"] =
      &InspectorPageAgentNG::GetResourceContent;
  functions_map_["Page.startScreencast"] =
      &InspectorPageAgentNG::StartScreencast;
  functions_map_["Page.stopScreencast"] = &InspectorPageAgentNG::StopScreencast;
  functions_map_["Page.screencastFrameAck"] =
      &InspectorPageAgentNG::ScreencastFrameAck;
  functions_map_["Page.reload"] = &InspectorPageAgentNG::Reload;
  functions_map_["Page.navigate"] = &InspectorPageAgentNG::Navigate;
}

InspectorPageAgentNG::~InspectorPageAgentNG() = default;

void InspectorPageAgentNG::Enable(const std::shared_ptr<MessageSender>& sender,
                                  const Json::Value& message) {
  devtool_mediator_->PageEnable(sender, message);
}

void InspectorPageAgentNG::CanScreencast(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->PageCanScreencast(sender, message);
}

void InspectorPageAgentNG::CanEmulate(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->PageCanEmulate(sender, message);
}

void InspectorPageAgentNG::GetResourceTree(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->PageGetResourceTree(sender, message);
}

void InspectorPageAgentNG::GetResourceContent(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->PageGetResourceContent(sender, message);
}

void InspectorPageAgentNG::StartScreencast(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->StartScreencast(sender, message);
}

void InspectorPageAgentNG::StopScreencast(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->StopScreencast(sender, message);
}

void InspectorPageAgentNG::ScreencastFrameAck(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->ScreencastFrameAck(sender, message);
}

void InspectorPageAgentNG::Reload(const std::shared_ptr<MessageSender>& sender,
                                  const Json::Value& message) {
  devtool_mediator_->PageReload(sender, message);
}

void InspectorPageAgentNG::Navigate(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->PageNavigate(sender, message);
}

void InspectorPageAgentNG::CallMethod(
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
