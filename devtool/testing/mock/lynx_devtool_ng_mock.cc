// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "devtool/testing/mock/lynx_devtool_ng_mock.h"

#include "devtool/base_devtool/native/test/message_sender_mock.h"
namespace lynx {
namespace testing {
int32_t LynxDevToolNGMock::Attach(const std::string& url) {
  return devtool::MockReceiver::GetInstance().Plug(url);
}

void LynxDevToolNGMock::SendMessageToDebugPlatform(const std::string& type,
                                                   const std::string& message) {
}

void LynxDevToolNGMock::OnTasmCreated(intptr_t shell_ptr) {}

void LynxDevToolNGMock::SetDevToolPlatformFacade(
    const std::shared_ptr<devtool::DevToolPlatformFacade>& platform_facade) {}

std::shared_ptr<lynx::piper::InspectorRuntimeObserverNG>
LynxDevToolNGMock::OnBackgroundRuntimeCreated(
    const std::string& group_thread_name) {
  return std::shared_ptr<lynx::piper::InspectorRuntimeObserverNG>(nullptr);
}

std::shared_ptr<devtool::MessageSender> LynxDevToolNGMock::GetMessageSender()
    const {
  return message_sender_;
}
}  // namespace testing
}  // namespace lynx
