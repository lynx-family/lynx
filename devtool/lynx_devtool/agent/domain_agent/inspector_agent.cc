// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_agent.h"

#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

namespace lynx {
namespace devtool {

InspectorAgent::InspectorAgent(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_(devtool_mediator) {
  functions_map_["Inspector.enable"] = &InspectorAgent::Enable;
  functions_map_["Inspector.detached"] = &InspectorAgent::Detached;
}

void InspectorAgent::Enable(const std::shared_ptr<CDPResponder>& responder,
                            const Json::Value&) {
  responder->SendSuccess();
}

void InspectorAgent::Detached(const std::shared_ptr<CDPResponder>& responder,
                              const Json::Value&) {
  Json::Value event(Json::objectValue);
  event["method"] = "Inspector.detached";
  event["params"]["reason"] = "";
  devtool_mediator_->SendCDPEvent(event);
  responder->SendSuccess();
}

void InspectorAgent::CallMethod(const std::shared_ptr<CDPResponder>& responder,
                                const Json::Value& message) {
  const std::string method = message["method"].asString();
  auto iter = functions_map_.find(method);
  if (iter == functions_map_.end()) {
    responder->SendError(CDPErrorCode::MethodNotFound,
                         "'" + method + "' wasn't found");
    return;
  }
  (this->*(iter->second))(responder, message["params"]);
}
}  // namespace devtool
}  // namespace lynx
