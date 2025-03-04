// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/base_devtool/native/common/devtool_global_slot_common.h"

namespace lynx {
namespace devtool {

DevToolGlobalSlotDelegate::DevToolGlobalSlotDelegate(
    const std::shared_ptr<DebugRouterMessageSubscriber>& delegate)
    : DevToolGlobalSlot(delegate) {}

void DevToolGlobalSlotDelegate::Init() {
  devtool_global_slot_common_ = std::make_shared<DevToolGlobalSlotCommon>();
  devtool_global_slot_common_->InitWithGlobalSlot(shared_from_this());
}

void DevToolGlobalSlotDelegate::SendMessage(const std::string& type,
                                            const std::string& msg) {
  devtool_global_slot_common_->sendMessage(msg, type);
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
