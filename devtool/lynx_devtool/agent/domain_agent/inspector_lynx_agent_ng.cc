// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_lynx_agent_ng.h"

#include "base/include/log/logging.h"
#include "core/services/recorder/record_payload.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

namespace lynx {
namespace devtool {
namespace {
int ParseLogLevel(const Json::Value& params) {
  return params.isObject() && params["level"].isString()
             ? base::logging::ParseLogLevel(params["level"].asString())
             : -1;
}
}  // namespace

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
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  std::string method = message["method"].asString();
  auto iter = functions_map_.find(method);
  if (iter == functions_map_.end()) {
    SendNotImplementedResponse(sender, message["id"].asInt64(), method);
  } else {
    (this->*(iter->second))(sender, message);
  }
}

void InspectorLynxAgentNG::SetObservePayloadEnabled(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  CDPResponder responder(sender, message["id"].asInt64());
  const auto& params = message["params"];
  if (!params.isObject() || !params["enabled"].isBool()) {
    responder.SendError(CDPErrorCode::InvalidParams,
                        "enabled must be a boolean");
    return;
  }
  tasm::recorder::SetObservationPayloadEnabled(params["enabled"].asBool());
  responder.SendSuccess();
}

void InspectorLynxAgentNG::GetLogLevel(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  Json::Value result;
  result["runtimeLevel"] = base::logging::GetMinLogLevelName();
  result["nativeCompileLevel"] = base::logging::GetCompileMinLogLevelName();
  CDPResponder(sender, message["id"].asInt64()).SendSuccess(result);
}

void InspectorLynxAgentNG::SetLogLevel(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  auto responder =
      std::make_shared<CDPResponder>(sender, message["id"].asInt64());
  const int level = ParseLogLevel(message["params"]);
  if (level < 0) {
    responder->SendError(CDPErrorCode::InvalidParams,
                         "level must be VERBOSE, DEBUG, MONITOR, OBSERVE, "
                         "INFO, WARNING or ERROR");
    return;
  }
  if (!devtool_mediator_ ||
      !devtool_mediator_->LynxSetLogLevel(responder, level)) {
    responder->SendError(CDPErrorCode::InternalError,
                         "The UI thread is not available");
  }
}

// return physical pixel rect of lynx view
void InspectorLynxAgentNG::GetLynxViewRectToWindow(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->LynxGetRectToWindow(sender, message);
}

void InspectorLynxAgentNG::GetProperties(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->LynxGetProperties(sender, message);
}

void InspectorLynxAgentNG::GetData(const std::shared_ptr<MessageSender>& sender,
                                   const Json::Value& message) {
  devtool_mediator_->LynxGetData(sender, message);
}

void InspectorLynxAgentNG::GetComponentId(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->LynxGetComponentId(sender, message);
}

void InspectorLynxAgentNG::GetLynxVersion(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->LynxGetVersion(sender, message);
}

void InspectorLynxAgentNG::TransferData(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->LynxTransferData(sender, message);
}

void InspectorLynxAgentNG::SetTraceMode(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->LynxSetTraceMode(sender, message);
}

void InspectorLynxAgentNG::GetScreenshot(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->GetScreenshot(sender, message);
}

void InspectorLynxAgentNG::GetViewLocationOnScreen(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->LynxGetViewLocationOnScreen(sender, message);
}

void InspectorLynxAgentNG::SendVMEvent(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  devtool_mediator_->LynxSendEventToVM(sender, message);
}

}  // namespace devtool
}  // namespace lynx
