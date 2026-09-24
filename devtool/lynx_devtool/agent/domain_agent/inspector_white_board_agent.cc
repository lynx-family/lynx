// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_white_board_agent.h"

#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

namespace lynx {
namespace devtool {

InspectorWhiteBoardAgent::InspectorWhiteBoardAgent(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_(devtool_mediator) {
  functions_map_["WhiteBoard.enable"] = &InspectorWhiteBoardAgent::Enable;
  functions_map_["WhiteBoard.disable"] = &InspectorWhiteBoardAgent::Disable;
  functions_map_["WhiteBoard.setSharedData"] =
      &InspectorWhiteBoardAgent::SetSharedData;
  legacy_functions_map_["WhiteBoard.getSharedData"] =
      &InspectorWhiteBoardAgent::GetSharedData;
  legacy_functions_map_["WhiteBoard.removeSharedData"] =
      &InspectorWhiteBoardAgent::RemoveSharedData;
  legacy_functions_map_["WhiteBoard.clear"] = &InspectorWhiteBoardAgent::Clear;
}

void InspectorWhiteBoardAgent::CallMethod(
    const std::shared_ptr<CDPResponder>& responder,
    const Json::Value& message) {
  std::string method = message["method"].asString();
  auto it = functions_map_.find(method);
  if (it == functions_map_.end()) {
    // Fall back only while patch2 commands still use MessageSender.
    CallMethod(responder->RetrieveSender(), message);
  } else {
    (this->*(it->second))(responder, message["params"]);
  }
}

void InspectorWhiteBoardAgent::CallMethod(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  std::string method = message["method"].asString();
  auto it = legacy_functions_map_.find(method);
  if (it == legacy_functions_map_.end()) {
    SendNotImplementedResponse(sender, message["id"].asInt64(), method);
  } else {
    (this->*(it->second))(sender, message);
  }
}

void InspectorWhiteBoardAgent::Enable(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->WhiteBoardEnable(responder, params);
}

void InspectorWhiteBoardAgent::Disable(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->WhiteBoardDisable(responder, params);
}

void InspectorWhiteBoardAgent::SetSharedData(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->WhiteBoardSetSharedData(responder, params);
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
