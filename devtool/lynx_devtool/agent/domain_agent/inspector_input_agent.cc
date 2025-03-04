// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_input_agent.h"

#include <sstream>

#include "devtool/lynx_devtool/base/mouse_event.h"

namespace lynx {
namespace devtool {

InspectorInputAgent::InspectorInputAgent(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_(devtool_mediator) {
  functions_map_["Input.emulateTouchFromMouseEvent"] =
      &InspectorInputAgent::EmulateTouchFromMouseEvent;
}

InspectorInputAgent::~InspectorInputAgent() = default;

void InspectorInputAgent::EmulateTouchFromMouseEvent(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->EmulateTouchFromMouseEvent(sender, message);
}

void InspectorInputAgent::CallMethod(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  std::string method = message["method"].asString();
  Json::Value params = message["params"];

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
