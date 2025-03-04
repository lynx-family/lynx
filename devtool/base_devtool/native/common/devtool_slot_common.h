// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_NATIVE_COMMON_DEVTOOL_SLOT_COMMON_H_
#define DEVTOOL_BASE_DEVTOOL_NATIVE_COMMON_DEVTOOL_SLOT_COMMON_H_
#include <memory>
#include <string>

#include "devtool/base_devtool/common/devtool_slot_common.h"
#include "devtool/base_devtool/native/devtool_slot.h"

namespace lynx {
namespace devtool {
class DevToolSlotDelegate : public DevToolSlot,
                            public std::enable_shared_from_this<DevToolSlot> {
 public:
  explicit DevToolSlotDelegate(
      const std::shared_ptr<DebugRouterMessageSubscriber>& delegate);
  void Init();
  int32_t Plug(const std::string& url) override;
  void Pull() override;
  void SendMessage(const std::string& type, const std::string& msg) override;

 private:
  std::shared_ptr<DevToolSlotCommon> devtool_slot_common_;
};
}  // namespace devtool
}  // namespace lynx
#endif  // DEVTOOL_BASE_DEVTOOL_NATIVE_COMMON_DEVTOOL_SLOT_COMMON_H_
