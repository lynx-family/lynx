// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/system_info_agent.h"

#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/lynx_devtool/agent/lynx_global_devtool_mediator.h"

namespace lynx {
namespace devtool {

SystemInfoAgent::SystemInfoAgent() {
  functions_map_["SystemInfo.getInfo"] = &SystemInfoAgent::getInfo;
}

SystemInfoAgent::~SystemInfoAgent() = default;

void SystemInfoAgent::getInfo(const std::shared_ptr<CDPResponder>& responder,
                              const Json::Value& message) {
  // Bridge to the legacy mediator path, which still sends the response through
  // the raw sender; RetrieveSender releases it so the responder does not emit a
  // duplicate fallback response.
  LynxGlobalDevToolMediator::GetInstance().SystemInfoGetInfo(
      responder->RetrieveSender(), message);
}

void SystemInfoAgent::CallMethod(const std::shared_ptr<CDPResponder>& responder,
                                 const Json::Value& content) {
  std::string method = content["method"].asString();
  auto iter = functions_map_.find(method);
  if (iter != functions_map_.end()) {
    (this->*(iter->second))(responder, content);
  } else {
    responder->SendErrorResponse(CDPErrorCode::kMethodNotFound,
                                 "'" + method + "' wasn't found");
  }
}

}  // namespace devtool
}  // namespace lynx
