// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_AGENT_CONSOLE_MESSAGE_MANAGER_H_
#define DEVTOOL_LYNX_DEVTOOL_AGENT_CONSOLE_MESSAGE_MANAGER_H_

#include <list>

#include "core/inspector/console_message_postman.h"

namespace lynx {
namespace devtool {

class MessageSender;

/**
 * When to use this class to send and cache console logs:
 *
 * 1. when js debugger is disabled, logs from js/lepus code will be sent and
 * cached by this class
 *
 * 2. when js runtime is disabled, logs from lepus code will be sent and cached
 * by this class
 *
 * 3. native errors(non-js errors and non-lepus errors) will be sent and cached
 * by this class
 */
class ConsoleMessageManager
    : public std::enable_shared_from_this<ConsoleMessageManager> {
 public:
  ConsoleMessageManager() = default;
  virtual ~ConsoleMessageManager() = default;

  void EnableConsoleLog(const std::shared_ptr<MessageSender>& sender);
  void DisableConsoleLog();
  void LogEntryAdded(const std::shared_ptr<MessageSender>& sender,
                     const lynx::piper::ConsoleMessage& message);
  void ClearConsoleMessages();

 private:
  void CacheLog(const piper::ConsoleMessage& message);
  void PostLog(const std::shared_ptr<MessageSender>& sender,
               const piper::ConsoleMessage& message);
  void FireCacheLogs(const std::shared_ptr<MessageSender>& sender);

  bool enable_{false};
  std::list<piper::ConsoleMessage> log_messages_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_AGENT_CONSOLE_MESSAGE_MANAGER_H_
