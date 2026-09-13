// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_input_agent.h"

#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

namespace lynx {
namespace devtool {

InspectorInputAgent::InspectorInputAgent(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_(devtool_mediator) {
  functions_map_["Input.emulateTouchFromMouseEvent"] =
      &InspectorInputAgent::EmulateTouchFromMouseEvent;
  functions_map_["Input.insertText"] = &InspectorInputAgent::InsertText;
  functions_map_["Input.synthesizeTapGesture"] =
      &InspectorInputAgent::SynthesizeTapGesture;
}

InspectorInputAgent::~InspectorInputAgent() = default;

void InspectorInputAgent::EmulateTouchFromMouseEvent(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->EmulateTouchFromMouseEvent(responder, params);
}

void InspectorInputAgent::InsertText(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->InsertText(responder, params);
}

void InspectorInputAgent::SynthesizeTapGesture(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->SynthesizeTapGesture(responder, params);
}

void InspectorInputAgent::CallMethod(
    const std::shared_ptr<CDPResponder>& responder,
    const Json::Value& message) {
  std::string method = message["method"].asString();
  auto iter = functions_map_.find(method);
  if (iter == functions_map_.end()) {
    responder->SendError(CDPErrorCode::MethodNotFound,
                         "'" + method + "' wasn't found");
  } else {
    (this->*(iter->second))(responder, message["params"]);
  }
}
}  // namespace devtool
}  // namespace lynx
