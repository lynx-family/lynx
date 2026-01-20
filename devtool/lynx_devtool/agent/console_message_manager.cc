// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/console_message_manager.h"

#include "core/runtime/common/lynx_console_helper.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"

namespace {
static constexpr std::string_view kLogVerbose = "verbose";
static constexpr std::string_view kLogInfo = "info";
static constexpr std::string_view kLogWarning = "warning";
static constexpr std::string_view kLogError = "error";

// There are some differences between the definition of level in logging.h and
// lynx_console.cc, need to use the same definition as lynx_console.cc here.
// (eg: 5 defined as LOG_FATAL in logging.h but CONSOLE_LOG_ALOG in
// lynx_console.cc)
std::string_view MessageLogLevel(int level) {
  switch (level) {
    case lynx::runtime::js::CONSOLE_LOG_VERBOSE:
      return kLogVerbose;
    case lynx::runtime::js::CONSOLE_LOG_WARNING:
      return kLogWarning;
    case lynx::runtime::js::CONSOLE_LOG_ERROR:
      return kLogError;
    default:
      return kLogInfo;
      break;
  }
}
}  // namespace

namespace lynx {
namespace devtool {

ConsoleMessageManager::ConsoleMessageManager(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_wp_(devtool_mediator) {}

void ConsoleMessageManager::EnableConsoleLog(
    const std::shared_ptr<MessageSender>& sender) {
  enable_ = true;
  FireCacheLogs();
}

void ConsoleMessageManager::DisableConsoleLog() { enable_ = false; }

void ConsoleMessageManager::LogEntryAdded(
    const lynx::runtime::js::ConsoleMessage& message) {
  if (enable_) {
    PostLog(message);
  }
  CacheLog(message);
}

void ConsoleMessageManager::CacheLog(
    const runtime::js::ConsoleMessage& message) {
  static constexpr int32_t MAX_MSG_NUM = 500;
  if (log_messages_.size() >= MAX_MSG_NUM) {
    log_messages_.pop_front();
  }
  log_messages_.push_back(std::move(message));
}

void ConsoleMessageManager::FireCacheLogs() {
  for (const auto& log : log_messages_) {
    PostLog(log);
  }
}

void ConsoleMessageManager::ClearConsoleMessages() { log_messages_.clear(); }

void ConsoleMessageManager::PostLog(
    const runtime::js::ConsoleMessage& message) {
  auto devtool_mediator = devtool_mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");

  Json::Value content;
  Json::Value params;
  Json::Value msg;
  msg["source"] = "javascript";
  msg["level"] = std::string(MessageLogLevel(message.level_));
  msg["text"] = message.text_;
  msg["timestamp"] = message.timestamp_;
  params["entry"] = msg;
  content["method"] = "Log.entryAdded";
  content["params"] = params;
  devtool_mediator->SendCDPEvent(content);
}

}  // namespace devtool
}  // namespace lynx
