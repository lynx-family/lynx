// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/console_message_manager.h"

#include "core/runtime/common/lynx_console_helper.h"
#include "devtool/lynx_devtool/agent/lynx_global_devtool_mediator.h"

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
    case lynx::piper::CONSOLE_LOG_VERBOSE:
      return kLogVerbose;
    case lynx::piper::CONSOLE_LOG_WARNING:
      return kLogWarning;
    case lynx::piper::CONSOLE_LOG_ERROR:
      return kLogError;
    default:
      return kLogInfo;
      break;
  }
}
}  // namespace

namespace lynx {
namespace devtool {

void ConsoleMessageManager::EnableConsoleLog(
    const std::shared_ptr<MessageSender>& sender) {
  enable_ = true;
  FireCacheLogs(sender);
}

void ConsoleMessageManager::DisableConsoleLog() { enable_ = false; }

void ConsoleMessageManager::LogEntryAdded(
    const std::shared_ptr<MessageSender>& sender,
    const lynx::piper::ConsoleMessage& message) {
  if (enable_) {
    PostLog(sender, message);
  }
  CacheLog(message);
}

void ConsoleMessageManager::CacheLog(const piper::ConsoleMessage& message) {
  static constexpr int32_t MAX_MSG_NUM = 500;
  if (log_messages_.size() >= MAX_MSG_NUM) {
    log_messages_.pop_front();
  }
  log_messages_.push_back(std::move(message));
}

void ConsoleMessageManager::FireCacheLogs(
    const std::shared_ptr<MessageSender>& sender) {
  for (const auto& log : log_messages_) {
    PostLog(sender, log);
  }
}

void ConsoleMessageManager::ClearConsoleMessages() { log_messages_.clear(); }

void ConsoleMessageManager::PostLog(
    const std::shared_ptr<MessageSender>& sender,
    const piper::ConsoleMessage& message) {
  if (!sender) {
    return;
  }
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
  sender->SendMessage("CDP", content);
}

}  // namespace devtool
}  // namespace lynx
