// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_NATIVE_DARWIN_IOS_DEVTOOL_SLOT_IOS_H_
#define DEVTOOL_BASE_DEVTOOL_NATIVE_DARWIN_IOS_DEVTOOL_SLOT_IOS_H_
#include <memory>
#include <string>

#import "devtool/base_devtool/darwin/ios/DevToolSlotIOS.h"
#include "devtool/base_devtool/native/devtool_slot.h"

namespace lynx {
namespace devtool {
class DevToolSlotDelegate : public DevToolSlot,
                            public std::enable_shared_from_this<DevToolSlot> {
 public:
  explicit DevToolSlotDelegate(
      const std::shared_ptr<DebugRouterMessageSubscriber>& delegate);
  ~DevToolSlotDelegate();
  void Init();
  int32_t Plug(const std::string& url) override;
  void Pull() override;
  void SendMessage(const std::string& type, const std::string& msg) override;

 private:
  DevToolSlotIOS* slot_;
};
}  // namespace devtool
}  // namespace lynx
#endif  // DEVTOOL_BASE_DEVTOOL_NATIVE_DARWIN_IOS_DEVTOOL_SLOT_IOS_H_
