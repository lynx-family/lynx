// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_testbench_replay_agent.h"

#include "core/services/replay/replay_controller.h"
#include "devtool/lynx_devtool/agent/lynx_global_devtool_mediator.h"

namespace lynx {
namespace devtool {

InspectorTestBenchReplayAgent::InspectorTestBenchReplayAgent() {
  functions_map_["Replay.start"] = &InspectorTestBenchReplayAgent::Start;
  functions_map_["Replay.end"] = &InspectorTestBenchReplayAgent::End;
}

InspectorTestBenchReplayAgent::~InspectorTestBenchReplayAgent() = default;

void InspectorTestBenchReplayAgent::CallMethod(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  std::string method = message["method"].asString();
  auto iter = functions_map_.find(method);
  if (iter == functions_map_.end() ||
      !lynx::tasm::replay::ReplayController::Enable()) {
    Json::Value res;
    res["error"]["code"] = kInspectorErrorCode;
    res["error"]["message"] = "Not implemented: " + method;
    res["id"] = message["id"].asInt64();
    sender->SendMessage("CDP", res);
  } else {
    (this->*(iter->second))(sender, message);
  }
}

void InspectorTestBenchReplayAgent::Start(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  LynxGlobalDevToolMediator::GetInstance().ReplayStart(sender, message);
}

void InspectorTestBenchReplayAgent::End(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  LynxGlobalDevToolMediator::GetInstance().ReplayEnd(sender, message);
}

}  // namespace devtool
}  // namespace lynx
