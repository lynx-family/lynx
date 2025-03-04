// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/base_devtool/native/common/devtool_slot_common.h"

namespace lynx {
namespace devtool {

DevToolSlotDelegate::DevToolSlotDelegate(
    const std::shared_ptr<DebugRouterMessageSubscriber>& delegate)
    : DevToolSlot(delegate) {}

void DevToolSlotDelegate::Init() {
  devtool_slot_common_ = std::make_shared<DevToolSlotCommon>();
  devtool_slot_common_->InitWithSlot(shared_from_this());
}

int32_t DevToolSlotDelegate::Plug(const std::string& url) {
  return devtool_slot_common_->Plug(url);
}

void DevToolSlotDelegate::Pull() { devtool_slot_common_->Pull(); }

void DevToolSlotDelegate::SendMessage(const std::string& type,
                                      const std::string& msg) {
  devtool_slot_common_->SendMessage(msg, type);
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
