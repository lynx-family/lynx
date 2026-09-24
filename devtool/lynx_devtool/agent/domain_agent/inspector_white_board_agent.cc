// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_white_board_agent.h"

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
  functions_map_["WhiteBoard.getSharedData"] =
      &InspectorWhiteBoardAgent::GetSharedData;
  functions_map_["WhiteBoard.removeSharedData"] =
      &InspectorWhiteBoardAgent::RemoveSharedData;
  functions_map_["WhiteBoard.clear"] = &InspectorWhiteBoardAgent::Clear;
}

void InspectorWhiteBoardAgent::CallMethod(
    const std::shared_ptr<CDPResponder>& responder,
    const Json::Value& message) {
  std::string method = message["method"].asString();
  auto it = functions_map_.find(method);
  if (it == functions_map_.end()) {
    responder->SendError(CDPErrorCode::MethodNotFound,
                         "'" + method + "' wasn't found");
  } else {
    (this->*(it->second))(responder, message["params"]);
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
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->WhiteBoardGetSharedData(responder, params);
}

void InspectorWhiteBoardAgent::RemoveSharedData(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->WhiteBoardRemoveSharedData(responder, params);
}

void InspectorWhiteBoardAgent::Clear(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->WhiteBoardClear(responder, params);
}

}  // namespace devtool
}  // namespace lynx
