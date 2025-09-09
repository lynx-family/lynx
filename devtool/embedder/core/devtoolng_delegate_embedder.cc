// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/embedder/core/devtoolng_delegate_embedder.h"

#include "devtool/embedder/core/devtool_message_handler_embedder.h"

namespace lynx {
namespace devtool {

DevToolNGDelegateEmbedder::DevToolNGDelegateEmbedder()
    : LynxDevToolNG(true), session_id_(0) {}

void DevToolNGDelegateEmbedder::SetDevtoolPlatformAbility(
    const std::shared_ptr<devtool::DevToolPlatformFacade>& platform_ptr) {
  SetDevToolPlatformFacade(platform_ptr);
}

void DevToolNGDelegateEmbedder::sendMessageToDebugPlatform(
    const std::string& type, const std::string& msg) {
  SendMessageToDebugPlatform(type, msg);
}

void DevToolNGDelegateEmbedder::attachToDebug(const std::string& url) {
  session_id_ = Attach(url);
}

void DevToolNGDelegateEmbedder::detachFromDebug() {
  Detach();
  session_id_ = 0;
}

void DevToolNGDelegateEmbedder::subscribeMessage(
    const std::string& type, const std::shared_ptr<MessageHandler>& handler) {
  SubscribeMessage(type,
                   std::make_unique<DevtoolMessageHandlerEmbedder>(handler));
}

}  // namespace devtool
}  // namespace lynx
