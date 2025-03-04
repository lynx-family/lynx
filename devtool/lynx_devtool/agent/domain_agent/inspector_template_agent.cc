// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_template_agent.h"

#include "core/runtime/vm/lepus/json_parser.h"

namespace lynx {
namespace devtool {
InspectorTemplateAgent::InspectorTemplateAgent(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_(devtool_mediator) {
  functions_map_["Template.templateData"] =
      &InspectorTemplateAgent::GetTemplateData;
  functions_map_["Template.templateConfigInfo"] =
      &InspectorTemplateAgent::GetTemplateConfigInfo;
  functions_map_["Template.templateApi"] =
      &InspectorTemplateAgent::GetTemplateApiInfo;
  functions_map_["Template.getTemplateJs"] =
      &InspectorTemplateAgent::GetTemplateJsInfo;
}

InspectorTemplateAgent::~InspectorTemplateAgent() = default;

void InspectorTemplateAgent::GetTemplateData(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->TemplateGetTemplateData(sender, message);
}

void InspectorTemplateAgent::GetTemplateConfigInfo(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value result(Json::ValueType::objectValue);

  // ConfigInfo has benn removed
  response["result"] = "";
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorTemplateAgent::GetTemplateApiInfo(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->TemplateGetTemplateApiInfo(sender, message);
}

void InspectorTemplateAgent::GetTemplateJsInfo(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->TemplateGetTemplateJsInfo(sender, message);
}

void InspectorTemplateAgent::CallMethod(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& content) {
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
