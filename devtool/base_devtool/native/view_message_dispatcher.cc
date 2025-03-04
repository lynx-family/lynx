// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/base_devtool/native/view_message_dispatcher.h"

#include <utility>

#include "base/include/log/logging.h"
#include "devtool/base_devtool/native/public/devtool_message_dispatcher.h"
#include "devtool/base_devtool/native/view_message_channel.h"

namespace lynx {
namespace devtool {

std::shared_ptr<ViewMessageDispatcher> ViewMessageDispatcher::Create() {
  std::shared_ptr<ViewMessageDispatcher> view_message_dispatcher(
      new ViewMessageDispatcher());
  view_message_dispatcher->Initialize();
  return view_message_dispatcher;
}

void ViewMessageDispatcher::Initialize() {
  view_message_channel_ = ViewMessageChannel::Create(shared_from_this());
}

void ViewMessageDispatcher::DispatchMessage(
    const std::shared_ptr<MessageSender>& sender, const std::string& type,
    const std::string& msg) {
  DevToolMessageDispatcher::DispatchMessage(sender, type, msg);
  auto it = subscribe_handler_map_.find(type);
  if (it != subscribe_handler_map_.end()) {
    it->second->handle(sender, type, msg);
    return;
  }
}

// for handling all kinds of messages
void ViewMessageDispatcher::SubscribeMessage(
    const std::string& type, std::unique_ptr<DevToolMessageHandler>&& handler) {
  auto it = subscribe_handler_map_.find(type);
  if (it != subscribe_handler_map_.end()) {
    LOGI("SubscribeMessage's handler has exists:" << it->first);
  }
  subscribe_handler_map_[type] = std::move(handler);
}

void ViewMessageDispatcher::UnSubscribeMessage(const std::string& type) {
  LOGI("UnSubscribeMessage :" << type);
  subscribe_handler_map_.erase(type);
}

ViewMessageDispatcher::ViewMessageDispatcher() {}

ViewMessageDispatcher::~ViewMessageDispatcher() {}

int32_t ViewMessageDispatcher::Attach(const std::string& url) {
  return view_message_channel_->Attach(url);
}
void ViewMessageDispatcher::Detach() { view_message_channel_->Detach(); }

std::shared_ptr<MessageSender> ViewMessageDispatcher::GetSender() const {
  return view_message_channel_;
}

}  // namespace devtool
}  // namespace lynx
