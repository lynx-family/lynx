// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_testbench_recorder_agent.h"

#include <fstream>

#include "core/services/recorder/recorder_controller.h"
#include "devtool/base_devtool/native/public/message_sender.h"
#include "devtool/lynx_devtool/agent/lynx_global_devtool_mediator.h"

namespace lynx {
namespace devtool {

InspectorTestBenchRecorderAgent::InspectorTestBenchRecorderAgent() {
  functions_map_["Recording.start"] = &InspectorTestBenchRecorderAgent::Start;
  functions_map_["Recording.end"] = &InspectorTestBenchRecorderAgent::End;
}

void InspectorTestBenchRecorderAgent::CallMethod(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  std::string method = message["method"].asString();
  auto iter = functions_map_.find(method);
  if (iter == functions_map_.end() ||
      !lynx::tasm::recorder::RecorderController::Enable()) {
    Json::Value res;
    res["error"]["code"] = kInspectorErrorCode;
    res["error"]["message"] = "Not implemented: " + method;
    res["id"] = message["id"].asInt64();
    sender->SendMessage("CDP", res);
  } else {
    (this->*(iter->second))(sender, message);
  }
}

void InspectorTestBenchRecorderAgent::Start(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  LynxGlobalDevToolMediator::GetInstance().RecordingStart(sender, message);
}

void InspectorTestBenchRecorderAgent::End(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  LynxGlobalDevToolMediator::GetInstance().RecordingEnd(sender, message);
}

}  // namespace devtool
}  // namespace lynx
