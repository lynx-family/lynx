// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/base_devtool/native/darwin/ios/devtool_slot_ios.h"

namespace lynx {
namespace devtool {
DevToolSlotDelegate::DevToolSlotDelegate(
    const std::shared_ptr<DebugRouterMessageSubscriber>& delegate)
    : DevToolSlot(delegate) {}

void DevToolSlotDelegate::Init() {
  slot_ = [[DevToolSlotIOS alloc] initWithSlotPtr:shared_from_this()];
}

DevToolSlotDelegate::~DevToolSlotDelegate() { slot_ = nil; }
int32_t DevToolSlotDelegate::Plug(const std::string& url) {
  NSString* urlNS = [NSString stringWithCString:url.c_str() encoding:NSUTF8StringEncoding];
  return [slot_ plug:urlNS];
}
void DevToolSlotDelegate::Pull() { [slot_ pull]; }
void DevToolSlotDelegate::SendMessage(const std::string& type, const std::string& msg) {
  @autoreleasepool {
    NSString* typeNS = [NSString stringWithCString:type.c_str() encoding:NSUTF8StringEncoding];
    NSString* msgNS = [NSString stringWithCString:msg.c_str() encoding:NSUTF8StringEncoding];
    [slot_ sendMessage:msgNS withType:typeNS];
  }
}

std::shared_ptr<DevToolSlot> DevToolSlot::Create(
    const std::shared_ptr<DebugRouterMessageSubscriber>& delegate) {
  std::shared_ptr<DevToolSlotDelegate> slot_delegate_ptr =
      std::make_shared<DevToolSlotDelegate>(delegate);
  slot_delegate_ptr->Init();
  return slot_delegate_ptr;
}
}  // namespace devtool
}  // namespace lynx
