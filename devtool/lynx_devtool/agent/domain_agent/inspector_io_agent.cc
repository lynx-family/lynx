// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_io_agent.h"

#include "devtool/lynx_devtool/agent/lynx_global_devtool_mediator.h"

namespace lynx {
namespace devtool {

InspectorIOAgent::InspectorIOAgent() {
  functions_map_["IO.read"] = &InspectorIOAgent::Read;
  functions_map_["IO.close"] = &InspectorIOAgent::Close;
}

InspectorIOAgent::~InspectorIOAgent() = default;

void InspectorIOAgent::CallMethod(const std::shared_ptr<MessageSender>& sender,
                                  const Json::Value& message) {
  std::string method = message["method"].asString();
  auto iter = functions_map_.find(method);
  if (iter == functions_map_.end()) {
    Json::Value res;
    res["error"] = Json::ValueType::objectValue;
    res["error"]["code"] = kInspectorErrorCode;
    res["error"]["message"] = "Not implemented: " + method;
    res["id"] = message["id"].asInt64();
    sender->SendMessage("CDP", res);
  } else {
    (this->*(iter->second))(sender, message);
  }
}

void InspectorIOAgent::Read(const std::shared_ptr<MessageSender>& sender,
                            const Json::Value& message) {
  LynxGlobalDevToolMediator::GetInstance().IORead(sender, message);
}

void InspectorIOAgent::Close(const std::shared_ptr<MessageSender>& sender,
                             const Json::Value& message) {
  LynxGlobalDevToolMediator::GetInstance().IOClose(sender, message);
}

}  // namespace devtool
}  // namespace lynx
