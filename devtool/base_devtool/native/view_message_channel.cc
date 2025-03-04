// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/base_devtool/native/view_message_channel.h"

#include <iostream>

#include "devtool/base_devtool/native/public/devtool_message_dispatcher.h"

namespace lynx {
namespace devtool {

std::shared_ptr<ViewMessageChannel> ViewMessageChannel::Create(
    const std::shared_ptr<DevToolMessageDispatcher>& agent) {
  std::shared_ptr<ViewMessageChannel> view_message_channel(
      new ViewMessageChannel(agent));
  view_message_channel->Initialize();
  return view_message_channel;
}

void ViewMessageChannel::Initialize() {
  slot_ = DevToolSlot::Create(shared_from_this());
}

ViewMessageChannel::ViewMessageChannel(
    const std::shared_ptr<DevToolMessageDispatcher>& agent)
    : slot_agent_(agent) {}

void ViewMessageChannel::OnMessageReceivedFromDebugRouter(
    const std::string& type, const std::string& msg) {
  std::shared_ptr<DevToolMessageDispatcher> slot_agent = slot_agent_.lock();
  if (slot_agent) {
    slot_agent->DispatchMessage(shared_from_this(), type, msg);
  }
}

void ViewMessageChannel::SendMessage(const std::string& type,
                                     const Json::Value& msg) {
  slot_->SendMessage(type, msg.toStyledString());
}

void ViewMessageChannel::SendMessage(const std::string& type,
                                     const std::string& msg) {
  slot_->SendMessage(type, msg);
}

int32_t ViewMessageChannel::Attach(const std::string& url) {
  return slot_->Plug(url);
}
void ViewMessageChannel::Detach() { slot_->Pull(); }

}  // namespace devtool
}  // namespace lynx
