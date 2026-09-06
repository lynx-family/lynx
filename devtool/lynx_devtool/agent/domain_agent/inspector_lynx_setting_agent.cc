// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_lynx_setting_agent.h"

#include <utility>

#include "devtool/base_devtool/native/public/cdp_responder.h"
#include "devtool/lynx_devtool/agent/global_devtool_platform_facade.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator_base.h"

namespace lynx {
namespace devtool {

InspectorLynxSettingAgent::InspectorLynxSettingAgent() {
  functions_map_["LynxSetting.getValues"] =
      &InspectorLynxSettingAgent::GetValues;
  functions_map_["LynxSetting.getLayeredValues"] =
      &InspectorLynxSettingAgent::GetLayeredValues;
  functions_map_["LynxSetting.getValue"] = &InspectorLynxSettingAgent::GetValue;
  functions_map_["LynxSetting.setMockValue"] =
      &InspectorLynxSettingAgent::SetMockValue;
  functions_map_["LynxSetting.removeMockValue"] =
      &InspectorLynxSettingAgent::RemoveMockValue;
  functions_map_["LynxSetting.clearMockValues"] =
      &InspectorLynxSettingAgent::ClearMockValues;
  functions_map_["LynxSetting.getFetchInfo"] =
      &InspectorLynxSettingAgent::GetFetchInfo;
  functions_map_["LynxSetting.fetchLatest"] =
      &InspectorLynxSettingAgent::FetchLatest;
}

void InspectorLynxSettingAgent::CallMethod(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& message) {
  const std::string method = message["method"].asString();
  auto iter = functions_map_.find(method);
  if (iter == functions_map_.end()) {
    responder->SendErrorResponse(CDPErrorCode::kMethodNotFound,
                                 "'" + method + "' wasn't found");
    return;
  }
  (this->*(iter->second))(responder, message);
}

void InspectorLynxSettingAgent::GetValues(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value&) {
  HandleRequest(responder, {"LynxSetting.getValues", "", ""});
}

void InspectorLynxSettingAgent::GetLayeredValues(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value&) {
  HandleRequest(responder, {"LynxSetting.getLayeredValues", "", ""});
}

void InspectorLynxSettingAgent::GetValue(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& message) {
  HandleKeyRequest(responder, message, "LynxSetting.getValue", false);
}

void InspectorLynxSettingAgent::SetMockValue(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& message) {
  HandleKeyRequest(responder, message, "LynxSetting.setMockValue", true);
}

void InspectorLynxSettingAgent::RemoveMockValue(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& message) {
  HandleKeyRequest(responder, message, "LynxSetting.removeMockValue", false);
}

void InspectorLynxSettingAgent::ClearMockValues(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value&) {
  HandleRequest(responder, {"LynxSetting.clearMockValues", "", ""});
}

void InspectorLynxSettingAgent::GetFetchInfo(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value&) {
  HandleRequest(responder, {"LynxSetting.getFetchInfo", "", ""});
}

void InspectorLynxSettingAgent::FetchLatest(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value&) {
  HandleRequest(responder, {"LynxSetting.fetchLatest", "", ""});
}

void InspectorLynxSettingAgent::HandleKeyRequest(
    const std::shared_ptr<CDPResponder>& responder, const Json::Value& message,
    const std::string& method, bool requires_value) {
  const Json::Value& params = message["params"];
  if (!params.isObject() || !params["key"].isString() ||
      params["key"].asString().empty()) {
    responder->SendErrorResponse(CDPErrorCode::kInvalidParams,
                                 "expected string key");
    return;
  }

  LynxSettingRequest request{method, params["key"].asString(), ""};
  if (requires_value) {
    if (!params["value"].isString()) {
      responder->SendErrorResponse(CDPErrorCode::kInvalidParams,
                                   "expected string mock value");
      return;
    }
    request.value = params["value"].asString();
  }
  HandleRequest(responder, std::move(request));
}

void InspectorLynxSettingAgent::HandleRequest(
    const std::shared_ptr<CDPResponder>& responder,
    LynxSettingRequest request) {
  auto task_runner =
      LynxDevToolMediatorBase::GetDevToolsThread().GetTaskRunner();
  if (!task_runner) {
    responder->SendErrorResponse(CDPErrorCode::kServerError,
                                 "Cannot find default task runner");
    return;
  }

  lynx::fml::TaskRunner::RunNowOrPostTask(
      task_runner,
      [responder, request = std::move(request), task_runner]() mutable {
        GlobalDevToolPlatformFacade::GetInstance().HandleLynxSetting(
            std::move(request),
            [responder, task_runner](const std::string& result_json,
                                     const std::string& error_message) {
              auto send_response = [responder, result_json, error_message]() {
                if (!error_message.empty()) {
                  responder->SendErrorResponse(CDPErrorCode::kServerError,
                                               error_message);
                  return;
                }
                Json::Value result;
                Json::Reader reader;
                if (!reader.parse(result_json, result, false) ||
                    !result.isObject()) {
                  responder->SendErrorResponse(
                      CDPErrorCode::kInternalError,
                      "Invalid LynxSetting result JSON");
                  return;
                }
                responder->Result() = std::move(result);
                responder->Send();
              };
              lynx::fml::TaskRunner::RunNowOrPostTask(task_runner,
                                                      std::move(send_response));
            });
      });
}

}  // namespace devtool
}  // namespace lynx
