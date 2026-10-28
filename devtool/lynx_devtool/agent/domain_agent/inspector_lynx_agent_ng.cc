// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_lynx_agent_ng.h"

#include <utility>

#include "base/include/log/logging.h"
#include "core/services/recorder/record_payload.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

namespace lynx {
namespace devtool {

InspectorLynxAgentNG::InspectorLynxAgentNG(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_(devtool_mediator) {
  functions_map_["Lynx.setObservePayloadEnabled"] =
      &InspectorLynxAgentNG::SetObservePayloadEnabled;
  functions_map_["Lynx.getLogLevel"] = &InspectorLynxAgentNG::GetLogLevel;
  functions_map_["Lynx.setLogLevel"] = &InspectorLynxAgentNG::SetLogLevel;
  functions_map_["Lynx.getProperties"] = &InspectorLynxAgentNG::GetProperties;
  functions_map_["Lynx.getData"] = &InspectorLynxAgentNG::GetData;
  functions_map_["Lynx.getComponentId"] = &InspectorLynxAgentNG::GetComponentId;
  functions_map_["Lynx.getRectToWindow"] =
      &InspectorLynxAgentNG::GetLynxViewRectToWindow;
  functions_map_["Lynx.getVersion"] = &InspectorLynxAgentNG::GetLynxVersion;
  functions_map_["Lynx.transferData"] = &InspectorLynxAgentNG::TransferData;
  functions_map_["Lynx.setTraceMode"] = &InspectorLynxAgentNG::SetTraceMode;
  functions_map_["Lynx.getScreenshot"] = &InspectorLynxAgentNG::GetScreenshot;
  functions_map_["Lynx.getViewLocationOnScreen"] =
      &InspectorLynxAgentNG::GetViewLocationOnScreen;
  functions_map_["Lynx.sendVMEvent"] = &InspectorLynxAgentNG::SendVMEvent;
}

InspectorLynxAgentNG::~InspectorLynxAgentNG() = default;

void InspectorLynxAgentNG::CallMethod(
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

void InspectorLynxAgentNG::SetObservePayloadEnabled(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  if (!params.isObject() || !params["enabled"].isBool()) {
    responder->SendError(CDPErrorCode::InvalidParams,
                         "enabled must be a boolean");
    return;
  }
  tasm::recorder::SetObservationPayloadEnabled(params["enabled"].asBool());
  responder->SendSuccess();
}

void InspectorLynxAgentNG::GetLogLevel(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value&) {
  Json::Value result;
  result["runtimeLevel"] = base::logging::GetMinLogLevelName();
  result["nativeCompileLevel"] = base::logging::GetCompileMinLogLevelName();
  responder->SendSuccess(std::move(result));
}

void InspectorLynxAgentNG::SetLogLevel(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->LynxSetLogLevel(responder, params);
}

// return physical pixel rect of lynx view
void InspectorLynxAgentNG::GetLynxViewRectToWindow(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->LynxGetRectToWindow(responder, params);
}

void InspectorLynxAgentNG::GetProperties(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->LynxGetProperties(responder, params);
}

void InspectorLynxAgentNG::GetData(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->LynxGetData(responder, params);
}

void InspectorLynxAgentNG::GetComponentId(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->LynxGetComponentId(responder, params);
}

void InspectorLynxAgentNG::GetLynxVersion(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->LynxGetVersion(responder, params);
}

void InspectorLynxAgentNG::TransferData(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->LynxTransferData(responder, params);
}

void InspectorLynxAgentNG::SetTraceMode(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->LynxSetTraceMode(responder, params);
}

void InspectorLynxAgentNG::GetScreenshot(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->GetScreenshot(responder, params);
}

void InspectorLynxAgentNG::GetViewLocationOnScreen(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->LynxGetViewLocationOnScreen(responder, params);
}

void InspectorLynxAgentNG::SendVMEvent(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& params) {
  devtool_mediator_->LynxSendEventToVM(responder, params);
}

}  // namespace devtool
}  // namespace lynx
