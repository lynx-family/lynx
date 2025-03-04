// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_NATIVE_DARWIN_IOS_DEVTOOL_GLOBAL_SLOT_IOS_H_
#define DEVTOOL_BASE_DEVTOOL_NATIVE_DARWIN_IOS_DEVTOOL_GLOBAL_SLOT_IOS_H_
#include <memory>
#include <string>

#import "devtool/base_devtool/darwin/ios/DevToolGlobalSlotIOS.h"
#include "devtool/base_devtool/native/devtool_global_slot.h"

namespace lynx {
namespace devtool {
class DevToolGlobalSlotDelegate
    : public DevToolGlobalSlot,
      public std::enable_shared_from_this<DevToolGlobalSlot> {
 public:
  explicit DevToolGlobalSlotDelegate(
      const std::shared_ptr<DebugRouterMessageSubscriber>& delegate);
  void Init();
  ~DevToolGlobalSlotDelegate();
  void SendMessage(const std::string& type, const std::string& msg) override;

 private:
  DevToolGlobalSlotIOS* slot_;
};
}  // namespace devtool
}  // namespace lynx
#endif  // DEVTOOL_BASE_DEVTOOL_NATIVE_DARWIN_IOS_DEVTOOL_GLOBAL_SLOT_IOS_H_
