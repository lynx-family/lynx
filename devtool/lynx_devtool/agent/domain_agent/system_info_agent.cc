// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/system_info_agent.h"

#include "devtool/lynx_devtool/agent/lynx_global_devtool_mediator.h"

namespace lynx {
namespace devtool {

SystemInfoAgent::SystemInfoAgent() {
  functions_map_["SystemInfo.getInfo"] = &SystemInfoAgent::getInfo;
}

SystemInfoAgent::~SystemInfoAgent() = default;

void SystemInfoAgent::getInfo(const std::shared_ptr<MessageSender>& sender,
                              const Json::Value& message) {
  LynxGlobalDevToolMediator::GetInstance().SystemInfoGetInfo(sender, message);
}

void SystemInfoAgent::CallMethod(const std::shared_ptr<MessageSender>& sender,
                                 const Json::Value& content) {
  std::string method = content["method"].asString();
  auto iter = functions_map_.find(method);
  if (iter != functions_map_.end()) {
    (this->*(iter->second))(sender, content);
  } else {
    Json::Value res;
    res["error"] = Json::ValueType::objectValue;
    res["error"]["code"] = kInspectorErrorCode;
    res["error"]["message"] = "Not implemented: " + method;
    res["id"] = content["id"].asInt64();
    sender->SendMessage("CDP", res);
  }
}

}  // namespace devtool
}  // namespace lynx
