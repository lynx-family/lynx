// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_template_agent.h"

#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

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

void InspectorTemplateAgent::GetTemplateData(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->TemplateGetTemplateData(responder, params);
}

void InspectorTemplateAgent::GetTemplateConfigInfo(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  // ConfigInfo has been removed; the response result is intentionally empty.
  responder->SendSuccess(Json::Value(""));
}

void InspectorTemplateAgent::GetTemplateApiInfo(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->TemplateGetTemplateApiInfo(responder, params);
}

void InspectorTemplateAgent::GetTemplateJsInfo(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->TemplateGetTemplateJsInfo(responder, params);
}

void InspectorTemplateAgent::CallMethod(
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

}  // namespace devtool
}  // namespace lynx
