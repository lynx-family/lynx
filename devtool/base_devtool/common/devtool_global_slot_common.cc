// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/base_devtool/common/devtool_global_slot_common.h"

#include "third_party/debug_router/src/DebugRouter/Common/debug_router.h"

namespace lynx {
namespace devtool {

using DebugRouter = debugrouter::DebugRouter;
const int GLOBAL_MESSAGE_SESSION_ID = -1;

void DevToolGlobalSlotCommon::InitWithGlobalSlot(
    const std::shared_ptr<lynx::devtool::DevToolGlobalSlot>& global_slot_ptr) {
  global_slot_ptr_ = global_slot_ptr;
  std::shared_ptr<DevToolGlobalSlotCommon> ptr = shared_from_this();
  DebugRouter::GetInstance().AddGlobalHandler(ptr.get());
}

void DevToolGlobalSlotCommon::sendMessage(const std::string& message,
                                          const std::string& type) {
  DebugRouter::GetInstance().SendDataAsync(message, type,
                                           GLOBAL_MESSAGE_SESSION_ID);
}

void DevToolGlobalSlotCommon::OpenCard(const std::string& url) {
  // To be implemented
}

void DevToolGlobalSlotCommon::OnMessage(const std::string& message,
                                        const std::string& type) {
  std::shared_ptr<lynx::devtool::DevToolGlobalSlot> global_slot_ptr =
      global_slot_ptr_.lock();
  if (global_slot_ptr) {
    global_slot_ptr->OnMessage(type, message);
  }
}

}  // namespace devtool
}  // namespace lynx
