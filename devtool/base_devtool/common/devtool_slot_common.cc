// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/base_devtool/common/devtool_slot_common.h"

namespace lynx {
namespace devtool {

void DevToolSlotCommon::InitWithSlot(
    const std::shared_ptr<lynx::devtool::DevToolSlot>& slot_ptr) {
  slot_ptr_ = slot_ptr;
  debug_router_slot_ = std::make_shared<debugrouter::DebugRouterSlot>();
  std::shared_ptr<DevToolSlotCommon> ptr = shared_from_this();
  debug_router_slot_->SetDelegate(ptr);
  template_url_ = "";
}

std::string DevToolSlotCommon::GetTemplateUrl() { return template_url_; }

void DevToolSlotCommon::OnMessage(const std::string& message,
                                  const std::string& type) {
  std::shared_ptr<lynx::devtool::DevToolSlot> slotPtr = slot_ptr_.lock();
  if (slotPtr) {
    slotPtr->OnMessage(type, message);
  }
}

int32_t DevToolSlotCommon::Plug(std::string url) {
  template_url_ = url;
  return debug_router_slot_->Plug();
}

void DevToolSlotCommon::Pull() { debug_router_slot_->Pull(); }

void DevToolSlotCommon::SendMessage(const std::string& message,
                                    const std::string& type) {
  debug_router_slot_->SendDataAsync(message, type);
}

}  // namespace devtool
}  // namespace lynx
