// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_NATIVE_DEBUG_ROUTER_MESSAGE_SUBSCRIBER_H_
#define DEVTOOL_BASE_DEVTOOL_NATIVE_DEBUG_ROUTER_MESSAGE_SUBSCRIBER_H_
#include <memory>
#include <string>

namespace lynx {
namespace devtool {
class DebugRouterMessageSubscriber {
 public:
  virtual void OnMessageReceivedFromDebugRouter(const std::string& type,
                                                const std::string& msg) = 0;
};
}  // namespace devtool
}  // namespace lynx
#endif  // DEVTOOL_BASE_DEVTOOL_NATIVE_DEBUG_ROUTER_MESSAGE_SUBSCRIBER_H_
