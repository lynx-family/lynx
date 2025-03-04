// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_COMMON_DEVTOOL_SLOT_COMMON_H_
#define DEVTOOL_BASE_DEVTOOL_COMMON_DEVTOOL_SLOT_COMMON_H_

#include <memory>
#include <string>

#include "devtool/base_devtool/native/devtool_slot.h"
#include "third_party/debug_router/src/DebugRouter/Common/debug_router_slot.h"
#include "third_party/debug_router/src/DebugRouter/Common/debug_router_slot_delegate.h"

namespace lynx {
namespace devtool {

class DevToolSlotCommon
    : public debugrouter::DebugRouterSlotDelegate,
      public std::enable_shared_from_this<DevToolSlotCommon> {
 public:
  void InitWithSlot(
      const std::shared_ptr<lynx::devtool::DevToolSlot>& slot_ptr);

  std::string GetTemplateUrl() override;
  void OnMessage(const std::string& message, const std::string& type) override;

  int32_t Plug(std::string url);
  void Pull();
  void SendMessage(const std::string& message, const std::string& type);

  virtual ~DevToolSlotCommon() = default;

 private:
  std::string template_url_;
  std::weak_ptr<lynx::devtool::DevToolSlot> slot_ptr_;
  std::shared_ptr<debugrouter::DebugRouterSlot> debug_router_slot_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_BASE_DEVTOOL_COMMON_DEVTOOL_SLOT_COMMON_H_
