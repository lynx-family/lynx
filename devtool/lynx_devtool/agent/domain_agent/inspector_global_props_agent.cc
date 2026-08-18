// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_global_props_agent.h"

namespace lynx {
namespace devtool {

InspectorGlobalPropsAgent::InspectorGlobalPropsAgent(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_(devtool_mediator) {
  functions_map_["GlobalProps.enable"] = &InspectorGlobalPropsAgent::Enable;
  functions_map_["GlobalProps.disable"] = &InspectorGlobalPropsAgent::Disable;
  functions_map_["GlobalProps.get"] = &InspectorGlobalPropsAgent::Get;
  functions_map_["GlobalProps.replace"] = &InspectorGlobalPropsAgent::Replace;
}

void InspectorGlobalPropsAgent::CallMethod(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  const std::string method = message["method"].asString();
  auto iter = functions_map_.find(method);
  if (iter == functions_map_.end()) {
    SendNotImplementedResponse(sender, message["id"].asInt64(), method);
    return;
  }
  (this->*(iter->second))(sender, message);
}

void InspectorGlobalPropsAgent::Enable(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->GlobalPropsEnable(sender, message);
}

void InspectorGlobalPropsAgent::Disable(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->GlobalPropsDisable(sender, message);
}

void InspectorGlobalPropsAgent::Get(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->GlobalPropsGet(sender, message);
}

void InspectorGlobalPropsAgent::Replace(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->GlobalPropsReplace(sender, message);
}

}  // namespace devtool
}  // namespace lynx
