// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/base_devtool/native/devtool_global_slot.h"

namespace lynx {
namespace devtool {
DevToolGlobalSlot::DevToolGlobalSlot(
    const std::shared_ptr<DebugRouterMessageSubscriber>& delegate)
    : delegate_(delegate) {}

void DevToolGlobalSlot::OnMessage(const std::string& type,
                                  const std::string& msg) {
  std::shared_ptr<DebugRouterMessageSubscriber> delegate = delegate_.lock();
  if (delegate) {
    delegate->OnMessageReceivedFromDebugRouter(type, msg);
  }
}
}  // namespace devtool
}  // namespace lynx
