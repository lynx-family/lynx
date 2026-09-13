// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_io_agent.h"

#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/lynx_devtool/agent/lynx_global_devtool_mediator.h"

namespace lynx {
namespace devtool {

InspectorIOAgent::InspectorIOAgent() {
  functions_map_["IO.read"] = &InspectorIOAgent::Read;
  functions_map_["IO.close"] = &InspectorIOAgent::Close;
}

InspectorIOAgent::~InspectorIOAgent() = default;

void InspectorIOAgent::CallMethod(
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

void InspectorIOAgent::Read(const std::shared_ptr<CDPResponder>& responder,
                            const Json::Value& params) {
  LynxGlobalDevToolMediator::GetInstance().IORead(responder, params);
}

void InspectorIOAgent::Close(const std::shared_ptr<CDPResponder>& responder,
                             const Json::Value& params) {
  LynxGlobalDevToolMediator::GetInstance().IOClose(responder, params);
}

}  // namespace devtool
}  // namespace lynx
