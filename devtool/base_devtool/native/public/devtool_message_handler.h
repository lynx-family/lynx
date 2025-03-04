// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_DEVTOOL_MESSAGE_HANDLER_H_
#define DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_DEVTOOL_MESSAGE_HANDLER_H_
#include <memory>
#include <string>

#include "devtool/base_devtool/native/public/message_sender.h"

namespace lynx {
namespace devtool {

/**
 *  The DevToolMessageHandler is used to handle messages of custom types.
 *  You can implement it and register it with the DevToolAgent to enable
 *  handling of messages with specific types from the DebugRouter.
 */
class DevToolMessageHandler {
 public:
  // handle message of specific type,
  // sender is used to send message to debugrouter
  virtual ~DevToolMessageHandler() = default;
  virtual void handle(const std::shared_ptr<MessageSender>& sender,
                      const std::string& type, const Json::Value& message) = 0;
};
}  // namespace devtool
}  // namespace lynx
#endif  // DEVTOOL_BASE_DEVTOOL_NATIVE_PUBLIC_DEVTOOL_MESSAGE_HANDLER_H_
