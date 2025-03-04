// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/base_devtool/native/test/devtool_global_slot_platform_mock.h"

namespace lynx {
namespace devtool {

std::shared_ptr<DevToolGlobalSlot> DevToolGlobalSlot::Create(
    const std::shared_ptr<DebugRouterMessageSubscriber>& delegate) {
  return std::make_shared<DevToolGlobalSlotPlatformMock>(delegate);
}

DevToolGlobalSlotPlatformMock::DevToolGlobalSlotPlatformMock(
    const std::shared_ptr<DebugRouterMessageSubscriber>& delegate)
    : DevToolGlobalSlot(delegate) {}
void DevToolGlobalSlotPlatformMock::OnMessage(const std::string& type,
                                              const std::string& msg) {}
void DevToolGlobalSlotPlatformMock::SendMessage(const std::string& type,
                                                const std::string& msg) {}
}  // namespace devtool
}  // namespace lynx
