// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_NATIVE_TEST_DEVTOOL_GLOBAL_SLOT_PLATFORM_MOCK_H_
#define DEVTOOL_BASE_DEVTOOL_NATIVE_TEST_DEVTOOL_GLOBAL_SLOT_PLATFORM_MOCK_H_
#include <memory>
#include <string>

#include "devtool/base_devtool/native/devtool_global_slot.h"

namespace lynx {
namespace devtool {
class DevToolGlobalSlotPlatformMock : public DevToolGlobalSlot {
 public:
  explicit DevToolGlobalSlotPlatformMock(
      const std::shared_ptr<DebugRouterMessageSubscriber>& delegate);
  void OnMessage(const std::string& type, const std::string& msg) override;
  void SendMessage(const std::string& type, const std::string& msg) override;
};
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_BASE_DEVTOOL_NATIVE_TEST_DEVTOOL_GLOBAL_SLOT_PLATFORM_MOCK_H_
