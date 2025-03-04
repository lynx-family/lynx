// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/base_devtool/native/test/devtool_slot_platform_mock.h"

#include "devtool/base_devtool/native/test/mock_receiver.h"

namespace lynx {
namespace devtool {
std::shared_ptr<DevToolSlot> DevToolSlot::Create(
    const std::shared_ptr<DebugRouterMessageSubscriber>& delegate) {
  return std::make_shared<DevToolSlotPlatformMock>(delegate);
}

DevToolSlotPlatformMock::DevToolSlotPlatformMock(
    const std::shared_ptr<DebugRouterMessageSubscriber>& delegate)
    : DevToolSlot(delegate) {}
int32_t DevToolSlotPlatformMock::Plug(const std::string& url) {
  return MockReceiver::GetInstance().Plug(url);
}
void DevToolSlotPlatformMock::Pull() { MockReceiver::GetInstance().Pull(); }

void DevToolSlotPlatformMock::OnMessage(const std::string& type,
                                        const std::string& msg) {}

void DevToolSlotPlatformMock::SendMessage(const std::string& type,
                                          const std::string& msg) {}
}  // namespace devtool
}  // namespace lynx
