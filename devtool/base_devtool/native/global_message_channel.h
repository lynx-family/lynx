// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_NATIVE_GLOBAL_MESSAGE_CHANNEL_H_
#define DEVTOOL_BASE_DEVTOOL_NATIVE_GLOBAL_MESSAGE_CHANNEL_H_
#include <memory>
#include <string>

#include "devtool/base_devtool/native/debug_router_message_subscriber.h"
#include "devtool/base_devtool/native/devtool_global_slot.h"
#include "devtool/base_devtool/native/public/devtool_message_dispatcher.h"

namespace lynx {
namespace devtool {

class GlobalMessageChannel
    : public DebugRouterMessageSubscriber,
      public MessageSender,
      public std::enable_shared_from_this<GlobalMessageChannel> {
 public:
  static std::shared_ptr<GlobalMessageChannel> Create(
      const std::shared_ptr<DevToolMessageDispatcher>& agent);
  virtual ~GlobalMessageChannel() = default;
  void OnMessageReceivedFromDebugRouter(const std::string& type,
                                        const std::string& msg) override;
  void SendMessage(const std::string& type, const Json::Value& msg) override;
  void SendMessage(const std::string& type, const std::string& msg) override;

 private:
  explicit GlobalMessageChannel(
      const std::shared_ptr<DevToolMessageDispatcher>& agent);
  void Initialize();
  std::weak_ptr<DevToolMessageDispatcher> global_agent_;
  std::shared_ptr<DevToolGlobalSlot> slot_;
};
}  // namespace devtool
}  // namespace lynx
#endif  // DEVTOOL_BASE_DEVTOOL_NATIVE_GLOBAL_MESSAGE_CHANNEL_H_
