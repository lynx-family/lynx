// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_COMMON_DEVTOOL_GLOBAL_SLOT_COMMON_H_
#define DEVTOOL_BASE_DEVTOOL_COMMON_DEVTOOL_GLOBAL_SLOT_COMMON_H_

#include <memory>
#include <string>

#include "devtool/base_devtool/native/devtool_global_slot.h"
#include "third_party/debug_router/src/DebugRouter/Common/debug_router_global_handler.h"

namespace lynx {
namespace devtool {

class DevToolGlobalSlotCommon
    : public debugrouter::DebugRouterGlobalHandler,
      public std::enable_shared_from_this<DevToolGlobalSlotCommon> {
 public:
  void InitWithGlobalSlot(
      const std::shared_ptr<lynx::devtool::DevToolGlobalSlot>& global_slot_ptr);
  void sendMessage(const std::string& message, const std::string& type);

  void OnMessage(const std::string& message, const std::string& type) override;
  void OpenCard(const std::string& url) override;
  virtual ~DevToolGlobalSlotCommon() = default;

 private:
  std::weak_ptr<lynx::devtool::DevToolGlobalSlot> global_slot_ptr_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_BASE_DEVTOOL_COMMON_DEVTOOL_GLOBAL_SLOT_COMMON_H_
