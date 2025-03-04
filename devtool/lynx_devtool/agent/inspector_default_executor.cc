// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/inspector_default_executor.h"

#include "base/include/log/logging.h"
#include "devtool/lynx_devtool/agent/inspector_util.h"

namespace lynx {
namespace devtool {

InspectorDefaultExecutor::InspectorDefaultExecutor(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_wp_(devtool_mediator),
      console_msg_manager_(std::make_unique<ConsoleMessageManager>()) {}

void InspectorDefaultExecutor::Reset() {
  console_msg_manager_->ClearConsoleMessages();
}

// start inspector protocl
void InspectorDefaultExecutor::InspectorEnable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  LOGI("InspectorEnable");
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content(Json::ValueType::objectValue);
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorDefaultExecutor::InspectorDetached(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  LOGI("InspectorDetached");
  Json::Value content;
  content["method"] = "Inspector.detached";
  content["params"] = Json::ValueType::objectValue;
  content["params"]["reason"] = "";
  sender->SendMessage("CDP", content);
}

void InspectorDefaultExecutor::LynxSetTraceMode(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  Json::Value content = Json::Value(Json::ValueType::objectValue);
  Json::Value params = message["params"];
  if (!params.empty()) {
    Json::Value enable_trace_mode = params["enableTraceMode"];
    if (!enable_trace_mode.empty() && enable_trace_mode.isBool()) {
      bool value = enable_trace_mode.asBool();
      CHECK_NULL_AND_LOG_RETURN(devtool_platform_facade_,
                                "devtool_platform_facade_ is null");
      devtool_platform_facade_->SetDevToolSwitch("enable_dom_tree", !value);
      devtool_platform_facade_->SetDevToolSwitch("enable_preview_screen_shot",
                                                 !value);
      devtool_platform_facade_->SetDevToolSwitch("enable_quickjs_debug",
                                                 !value);
      devtool_platform_facade_->SetDevToolSwitch("enable_v8", !value);
    }
  }
  response["result"] = content;
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorDefaultExecutor::LynxGetVersion(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  Json::Value response(Json::ValueType::objectValue);
  CHECK_NULL_AND_LOG_RETURN(devtool_platform_facade_,
                            "devtool_platform_facade_ is null");
  response["result"] = devtool_platform_facade_->GetLynxVersion();
  response["id"] = message["id"].asInt64();
  sender->SendMessage("CDP", response);
}

void InspectorDefaultExecutor::SetDevToolPlatformFacade(
    const std::shared_ptr<DevToolPlatformFacade>& devtool_platform_facade) {
  devtool_platform_facade_ = devtool_platform_facade;
}

// end inspector protocl

// start log protocol
void InspectorDefaultExecutor::LogEnable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  LOGI("LogEnable");
  console_msg_manager_->EnableConsoleLog(sender);
  sender->SendOKResponse(message["id"].asInt64());
}

void InspectorDefaultExecutor::LogDisable(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  LOGI("LogDisable");
  console_msg_manager_->DisableConsoleLog();
  sender->SendOKResponse(message["id"].asInt64());
}

void InspectorDefaultExecutor::LogClear(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const Json::Value& message) {
  console_msg_manager_->ClearConsoleMessages();
  sender->SendOKResponse(message["id"].asInt64());
}

void InspectorDefaultExecutor::LogEntryAdded(
    const std::shared_ptr<lynx::devtool::MessageSender>& sender,
    const lynx::piper::ConsoleMessage& message) {
  console_msg_manager_->LogEntryAdded(sender, message);
}

// end log protocol

}  // namespace devtool
}  // namespace lynx
