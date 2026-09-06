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
    const std::shared_ptr<CDPResponder>& responder,
    const Json::Value& message) {
  // Bridge to the legacy mediator path, which still assembles and sends the
  // response through the raw sender. Retrieving the sender releases it from the
  // responder so no duplicate fallback response is emitted.
  LynxGlobalDevToolMediator::GetInstance().MemoryStartTracing(
      responder->RetrieveSender(), message);
}

void InspectorMemoryAgent::StopTracing(
    const std::shared_ptr<CDPResponder>& responder,
    const Json::Value& message) {
  LynxGlobalDevToolMediator::GetInstance().MemoryStopTracing(
      responder->RetrieveSender(), message);
}

void InspectorMemoryAgent::GetAllMemoryUsage(
    const std::shared_ptr<CDPResponder>& responder,
    const Json::Value& message) {
  LynxGlobalDevToolMediator::GetInstance().MemoryGetAllMemoryUsage(
      responder->RetrieveSender(), message);
}

void InspectorMemoryAgent::CallMethod(
    const std::shared_ptr<CDPResponder>& responder,
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
