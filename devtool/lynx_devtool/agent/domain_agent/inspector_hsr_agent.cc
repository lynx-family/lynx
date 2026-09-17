// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_hsr_agent.h"

#include "devtool/base_devtool/native/public/cdp_responder.h"

namespace lynx {
namespace devtool {
InspectorHSRAgent::InspectorHSRAgent() = default;

void InspectorHSRAgent::CallMethod(
    const std::shared_ptr<CDPResponder>& responder,
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
