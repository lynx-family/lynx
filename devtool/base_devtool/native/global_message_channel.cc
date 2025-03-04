// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/base_devtool/native/global_message_channel.h"

#include "devtool/base_devtool/native/public/devtool_message_dispatcher.h"

namespace lynx {
namespace devtool {

std::shared_ptr<GlobalMessageChannel> GlobalMessageChannel::Create(
    const std::shared_ptr<DevToolMessageDispatcher>& agent) {
  std::shared_ptr<GlobalMessageChannel> global_message_channel(
      new GlobalMessageChannel(agent));
  global_message_channel->Initialize();
  return global_message_channel;
}

GlobalMessageChannel::GlobalMessageChannel(
    const std::shared_ptr<DevToolMessageDispatcher>& agent)
    : global_agent_(agent) {}

void GlobalMessageChannel::Initialize() {
  slot_ = DevToolGlobalSlot::Create(shared_from_this());
}

void GlobalMessageChannel::SendMessage(const std::string& type,
                                       const Json::Value& msg) {
  slot_->SendMessage(type, msg.toStyledString());
}

void GlobalMessageChannel::SendMessage(const std::string& type,
                                       const std::string& msg) {
  slot_->SendMessage(type, msg);
}

void GlobalMessageChannel::OnMessageReceivedFromDebugRouter(
    const std::string& type, const std::string& msg) {
  std::shared_ptr<DevToolMessageDispatcher> agent = global_agent_.lock();
  if (agent) {
    agent->DispatchMessage(shared_from_this(), type, msg);
  }
}

}  // namespace devtool
}  // namespace lynx
