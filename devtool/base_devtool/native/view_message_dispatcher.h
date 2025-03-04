// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_NATIVE_VIEW_MESSAGE_DISPATCHER_H_
#define DEVTOOL_BASE_DEVTOOL_NATIVE_VIEW_MESSAGE_DISPATCHER_H_
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#include "devtool/base_devtool/native/public/devtool_message_dispatcher.h"
#include "devtool/base_devtool/native/view_message_channel.h"

namespace lynx {
namespace devtool {
// for view-specific message registration and dispatching
class ViewMessageDispatcher : public DevToolMessageDispatcher {
 public:
  static std::shared_ptr<ViewMessageDispatcher> Create();
  int32_t Attach(const std::string& url);
  void Detach();
  virtual ~ViewMessageDispatcher();
  std::shared_ptr<MessageSender> GetSender() const;
  void DispatchMessage(const std::shared_ptr<MessageSender>& sender,
                       const std::string& type,
                       const std::string& msg) override;
  // for handling all kinds of messages
  void SubscribeMessage(const std::string& type,
                        std::unique_ptr<DevToolMessageHandler>&& handler);

  void UnSubscribeMessage(const std::string& type);

 private:
  ViewMessageDispatcher();
  void Initialize();
  std::shared_ptr<ViewMessageChannel> view_message_channel_;
  std::unordered_map<std::string, std::unique_ptr<DevToolMessageHandler>>
      subscribe_handler_map_;
};
}  // namespace devtool
}  // namespace lynx
#endif  // DEVTOOL_BASE_DEVTOOL_NATIVE_VIEW_MESSAGE_DISPATCHER_H_
