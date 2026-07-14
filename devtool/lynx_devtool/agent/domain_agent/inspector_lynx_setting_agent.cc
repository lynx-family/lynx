// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_lynx_setting_agent.h"

#include <utility>

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
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  const std::string method = message["method"].asString();
  auto iter = functions_map_.find(method);
  if (iter == functions_map_.end()) {
    SendNotImplementedResponse(sender, message["id"].asInt64(), method);
    return;
  }
  (this->*(iter->second))(sender, message);
}

void InspectorLynxSettingAgent::GetValues(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  HandleRequest(sender, message, {"LynxSetting.getValues", "", ""});
}

void InspectorLynxSettingAgent::GetLayeredValues(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  HandleRequest(sender, message, {"LynxSetting.getLayeredValues", "", ""});
}

void InspectorLynxSettingAgent::GetValue(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  HandleKeyRequest(sender, message, "LynxSetting.getValue", false);
}

void InspectorLynxSettingAgent::SetMockValue(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  HandleKeyRequest(sender, message, "LynxSetting.setMockValue", true);
}

void InspectorLynxSettingAgent::RemoveMockValue(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  HandleKeyRequest(sender, message, "LynxSetting.removeMockValue", false);
}

void InspectorLynxSettingAgent::ClearMockValues(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  HandleRequest(sender, message, {"LynxSetting.clearMockValues", "", ""});
}

void InspectorLynxSettingAgent::GetFetchInfo(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  HandleRequest(sender, message, {"LynxSetting.getFetchInfo", "", ""});
}

void InspectorLynxSettingAgent::FetchLatest(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message) {
  HandleRequest(sender, message, {"LynxSetting.fetchLatest", "", ""});
}

void InspectorLynxSettingAgent::HandleKeyRequest(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message,
    const std::string& method, bool requires_value) {
  const int64_t id = message["id"].asInt64();
  const Json::Value& params = message["params"];
  if (!params.isObject() || !params["key"].isString() ||
      params["key"].asString().empty()) {
    sender->SendErrorResponse(id, "Invalid params: expected string key");
    return;
  }

  LynxSettingRequest request{method, params["key"].asString(), ""};
  if (requires_value) {
    if (!params["value"].isString()) {
      sender->SendErrorResponse(id,
                                "Invalid params: expected string mock value");
      return;
    }
    request.value = params["value"].asString();
  }
  HandleRequest(sender, message, std::move(request));
}

void InspectorLynxSettingAgent::HandleRequest(
    const std::shared_ptr<MessageSender>& sender, const Json::Value& message,
    LynxSettingRequest request) {
  const int64_t id = message["id"].asInt64();
  auto task_runner =
      LynxDevToolMediatorBase::GetDevToolsThread().GetTaskRunner();
  if (!task_runner) {
    sender->SendErrorResponse(id, "Cannot find default task runner");
    return;
  }

  lynx::fml::TaskRunner::RunNowOrPostTask(
      task_runner,
      [sender, id, request = std::move(request), task_runner]() mutable {
        GlobalDevToolPlatformFacade::GetInstance().HandleLynxSetting(
            std::move(request),
            [sender, id, task_runner](const std::string& result_json,
                                      const std::string& error_message) {
              auto send_response = [sender, id, result_json, error_message]() {
                if (!error_message.empty()) {
                  sender->SendErrorResponse(id, error_message);
                  return;
                }
                Json::Value result;
                Json::Reader reader;
                if (!reader.parse(result_json, result, false) ||
                    !result.isObject()) {
                  sender->SendErrorResponse(id,
                                            "Invalid LynxSetting result JSON");
                  return;
                }
                Json::Value response(Json::ValueType::objectValue);
                response["id"] = id;
                response["result"] = std::move(result);
                sender->SendMessage("CDP", response);
              };
              lynx::fml::TaskRunner::RunNowOrPostTask(task_runner,
                                                      std::move(send_response));
            });
      });
}

}  // namespace devtool
}  // namespace lynx
