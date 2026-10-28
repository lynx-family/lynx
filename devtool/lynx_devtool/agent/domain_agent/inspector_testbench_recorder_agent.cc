// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_testbench_recorder_agent.h"

#include "core/services/recorder/recorder_controller.h"
#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/lynx_devtool/agent/lynx_global_devtool_mediator.h"

namespace lynx {
namespace devtool {

InspectorTestBenchRecorderAgent::InspectorTestBenchRecorderAgent() {
  functions_map_["Recording.start"] = &InspectorTestBenchRecorderAgent::Start;
  functions_map_["Recording.end"] = &InspectorTestBenchRecorderAgent::End;
}

void InspectorTestBenchRecorderAgent::CallMethod(
    const std::shared_ptr<CDPResponder>& responder,
    const Json::Value& message) {
  std::string method = message["method"].asString();
  auto iter = functions_map_.find(method);
  if (iter == functions_map_.end() ||
      !lynx::tasm::recorder::RecorderController::Enable()) {
    responder->SendError(CDPErrorCode::MethodNotFound,
                         "Not implemented: " + method);
  } else {
    (this->*(iter->second))(responder, message["params"]);
  }
}

void InspectorTestBenchRecorderAgent::Start(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  LynxGlobalDevToolMediator::GetInstance().RecordingStart(responder, params);
}

void InspectorTestBenchRecorderAgent::End(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  LynxGlobalDevToolMediator::GetInstance().RecordingEnd(responder, params);
}

}  // namespace devtool
}  // namespace lynx
