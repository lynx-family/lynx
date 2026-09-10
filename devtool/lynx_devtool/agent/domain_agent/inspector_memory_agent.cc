// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_memory_agent.h"

#include "core/runtime/lepus/json_parser.h"
#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/lynx_devtool/agent/lynx_global_devtool_mediator.h"

namespace lynx {
namespace devtool {
InspectorMemoryAgent::InspectorMemoryAgent() {
  functions_map_["Memory.startTracing"] = &InspectorMemoryAgent::StartTracing;
  functions_map_["Memory.stopTracing"] = &InspectorMemoryAgent::StopTracing;
  functions_map_["Memory.getAllMemoryUsage"] =
      &InspectorMemoryAgent::GetAllMemoryUsage;
}

InspectorMemoryAgent::~InspectorMemoryAgent() = default;

void InspectorMemoryAgent::StartTracing(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  LynxGlobalDevToolMediator::GetInstance().MemoryStartTracing(responder,
                                                              params);
}

void InspectorMemoryAgent::StopTracing(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  LynxGlobalDevToolMediator::GetInstance().MemoryStopTracing(responder, params);
}

void InspectorMemoryAgent::GetAllMemoryUsage(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  LynxGlobalDevToolMediator::GetInstance().MemoryGetAllMemoryUsage(responder,
                                                                   params);
}

void InspectorMemoryAgent::CallMethod(
    const std::shared_ptr<CDPResponder>& responder,
    const Json::Value& content) {
  std::string method = content["method"].asString();
  auto iter = functions_map_.find(method);
  if (iter != functions_map_.end()) {
    (this->*(iter->second))(responder, content["params"]);
  } else {
    responder->SendError(CDPErrorCode::MethodNotFound,
                         "'" + method + "' wasn't found");
  }
}

}  // namespace devtool
}  // namespace lynx
