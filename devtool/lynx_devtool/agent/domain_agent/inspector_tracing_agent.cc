// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_tracing_agent.h"

#include <fstream>

#include "core/base/lynx_trace_categories.h"
#include "core/runtime/profile/runtime_profiler_manager.h"
#include "devtool/lynx_devtool/agent/lynx_global_devtool_mediator.h"
#include "devtool/lynx_devtool/base/file_stream.h"

namespace lynx {
namespace devtool {

InspectorTracingAgent::InspectorTracingAgent() {
  functions_map_["Tracing.start"] = &InspectorTracingAgent::Start;
  functions_map_["Tracing.end"] = &InspectorTracingAgent::End;
  functions_map_["Tracing.setStartupTracingConfig"] =
      &InspectorTracingAgent::SetStartupTracingConfig;
  functions_map_["Tracing.getStartupTracingConfig"] =
      &InspectorTracingAgent::GetStartupTracingConfig;
  functions_map_["Tracing.getStartupTracingFile"] =
      &InspectorTracingAgent::GetStartupTracingFile;
}

InspectorTracingAgent::~InspectorTracingAgent() = default;

void InspectorTracingAgent::CallMethod(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  std::string method = message["method"].asString();
  auto iter = functions_map_.find(method);
  if (iter == functions_map_.end()) {
    Json::Value res;
    res["error"]["code"] = kInspectorErrorCode;
    res["error"]["message"] = "Not implemented: " + method;
    res["id"] = message["id"].asInt64();
    sender->SendMessage("CDP", res);
  } else {
    (this->*(iter->second))(sender, message);
  }
}

void InspectorTracingAgent::Start(const std::shared_ptr<MessageSender>& sender,
                                  const Json::Value& message) {
  LynxGlobalDevToolMediator::GetInstance().TracingStart(sender, message);
}

void InspectorTracingAgent::End(const std::shared_ptr<MessageSender>& sender,
                                const Json::Value& message) {
  LynxGlobalDevToolMediator::GetInstance().TracingEnd(sender, message);
}

void InspectorTracingAgent::SetStartupTracingConfig(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  LynxGlobalDevToolMediator::GetInstance().SetStartupTracingConfig(sender,
                                                                   message);
}

void InspectorTracingAgent::GetStartupTracingConfig(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  LynxGlobalDevToolMediator::GetInstance().GetStartupTracingFile(sender,
                                                                 message);
}

void InspectorTracingAgent::GetStartupTracingFile(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  LynxGlobalDevToolMediator::GetInstance().GetStartupTracingFile(sender,
                                                                 message);
}

}  // namespace devtool
}  // namespace lynx
