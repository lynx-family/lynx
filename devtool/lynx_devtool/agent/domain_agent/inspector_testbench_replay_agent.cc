// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_testbench_replay_agent.h"

#include "core/services/replay/replay_controller.h"
#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/lynx_devtool/agent/lynx_global_devtool_mediator.h"

namespace lynx {
namespace devtool {

InspectorTestBenchReplayAgent::InspectorTestBenchReplayAgent() {
  functions_map_["Replay.start"] = &InspectorTestBenchReplayAgent::Start;
  functions_map_["Replay.end"] = &InspectorTestBenchReplayAgent::End;
}

InspectorTestBenchReplayAgent::~InspectorTestBenchReplayAgent() = default;

void InspectorTestBenchReplayAgent::CallMethod(
    const std::shared_ptr<CDPResponder>& responder,
    const Json::Value& message) {
  std::string method = message["method"].asString();
  auto iter = functions_map_.find(method);
  if (iter == functions_map_.end() ||
      !lynx::tasm::replay::ReplayController::Enable()) {
    responder->SendError(CDPErrorCode::MethodNotFound,
                         "Not implemented: " + method);
  } else {
    (this->*(iter->second))(responder, message["params"]);
  }
}

void InspectorTestBenchReplayAgent::Start(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  LynxGlobalDevToolMediator::GetInstance().ReplayStart(responder, params);
}

void InspectorTestBenchReplayAgent::End(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  LynxGlobalDevToolMediator::GetInstance().ReplayEnd(responder, params);
}

}  // namespace devtool
}  // namespace lynx
