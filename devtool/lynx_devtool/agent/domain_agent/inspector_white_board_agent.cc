// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_white_board_agent.h"

namespace lynx {
namespace devtool {

InspectorWhiteBoardAgent::InspectorWhiteBoardAgent(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_(devtool_mediator) {
  functions_map_["WhiteBoard.enable"] = &InspectorWhiteBoardAgent::Enable;
  functions_map_["WhiteBoard.disable"] = &InspectorWhiteBoardAgent::Disable;
  functions_map_["WhiteBoard.setSharedData"] =
      &InspectorWhiteBoardAgent::SetSharedData;
  functions_map_["WhiteBoard.getSharedData"] =
      &InspectorWhiteBoardAgent::GetSharedData;
  functions_map_["WhiteBoard.removeSharedData"] =
      &InspectorWhiteBoardAgent::RemoveSharedData;
  functions_map_["WhiteBoard.clear"] = &InspectorWhiteBoardAgent::Clear;
}

void InspectorWhiteBoardAgent::CallMethod(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  std::string method = message["method"].asString();
  auto it = functions_map_.find(method);
  if (it == functions_map_.end()) {
    SendNotImplementedResponse(sender, message["id"].asInt64(), method);
  } else {
    (this->*(it->second))(sender, message);
  }
}

void InspectorWhiteBoardAgent::Enable(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->WhiteBoardEnable(sender, message);
}

void InspectorWhiteBoardAgent::Disable(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->WhiteBoardDisable(sender, message);
}

void InspectorWhiteBoardAgent::SetSharedData(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->WhiteBoardSetSharedData(sender, message);
}

void InspectorWhiteBoardAgent::GetSharedData(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->WhiteBoardGetSharedData(sender, message);
}

void InspectorWhiteBoardAgent::RemoveSharedData(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->WhiteBoardRemoveSharedData(sender, message);
}

void InspectorWhiteBoardAgent::Clear(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->WhiteBoardClear(sender, message);
}

}  // namespace devtool
}  // namespace lynx
