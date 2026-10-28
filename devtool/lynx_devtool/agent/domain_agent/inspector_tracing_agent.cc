// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_tracing_agent.h"

#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/lynx_devtool/agent/lynx_global_devtool_mediator.h"

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
  functions_map_["Tracing.takeVMSnapshotByUrl"] =
      &InspectorTracingAgent::TakeVMSnapshotByUrl;
}

InspectorTracingAgent::~InspectorTracingAgent() = default;

void InspectorTracingAgent::CallMethod(
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

void InspectorTracingAgent::Start(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  LynxGlobalDevToolMediator::GetInstance().TracingStart(responder, params);
}

void InspectorTracingAgent::End(const std::shared_ptr<CDPResponder>& responder,
                                const Json::Value& params) {
  LynxGlobalDevToolMediator::GetInstance().TracingEnd(responder, params);
}

void InspectorTracingAgent::SetStartupTracingConfig(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  LynxGlobalDevToolMediator::GetInstance().SetStartupTracingConfig(responder,
                                                                   params);
}

void InspectorTracingAgent::GetStartupTracingConfig(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  LynxGlobalDevToolMediator::GetInstance().GetStartupTracingConfig(responder,
                                                                   params);
}

void InspectorTracingAgent::GetStartupTracingFile(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  LynxGlobalDevToolMediator::GetInstance().GetStartupTracingFile(responder,
                                                                 params);
}

void InspectorTracingAgent::TakeVMSnapshotByUrl(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  LynxGlobalDevToolMediator::GetInstance().TakeVMSnapshotByUrl(responder,
                                                               params);
}

}  // namespace devtool
}  // namespace lynx
