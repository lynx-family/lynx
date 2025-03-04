// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/base_devtool/native/darwin/ios/devtool_global_slot_ios.h"

namespace lynx {
namespace devtool {
DevToolGlobalSlotDelegate::DevToolGlobalSlotDelegate(
    const std::shared_ptr<DebugRouterMessageSubscriber>& delegate)
    : DevToolGlobalSlot(delegate) {}

void DevToolGlobalSlotDelegate::Init() {
  slot_ = [[DevToolGlobalSlotIOS alloc] initWithSlotPtr:shared_from_this()];
}

DevToolGlobalSlotDelegate::~DevToolGlobalSlotDelegate() { slot_ = nil; }

void DevToolGlobalSlotDelegate::SendMessage(const std::string& type, const std::string& msg) {
  NSString* typeNS = [NSString stringWithCString:type.c_str() encoding:NSUTF8StringEncoding];
  NSString* msgNS = [NSString stringWithCString:msg.c_str() encoding:NSUTF8StringEncoding];
  [slot_ sendMessage:msgNS withType:typeNS];
}

std::shared_ptr<DevToolGlobalSlot> DevToolGlobalSlot::Create(
    const std::shared_ptr<DebugRouterMessageSubscriber>& delegate) {
  std::shared_ptr<DevToolGlobalSlotDelegate> slot_delegate_ptr =
      std::make_shared<DevToolGlobalSlotDelegate>(delegate);
  slot_delegate_ptr->Init();
  return slot_delegate_ptr;
}
}  // namespace devtool
}  // namespace lynx
