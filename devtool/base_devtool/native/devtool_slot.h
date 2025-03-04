// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_NATIVE_DEVTOOL_SLOT_H_
#define DEVTOOL_BASE_DEVTOOL_NATIVE_DEVTOOL_SLOT_H_
#include <cstdint>
#include <memory>
#include <string>

#include "devtool/base_devtool/native/debug_router_message_subscriber.h"

namespace lynx {
namespace devtool {

// DevToolSlot is a wrapper for the messaging capabilities of DebugRouter.
// Its main responsibility is to send and receive messages related to view
// instances.
class DevToolSlot {
 public:
  static std::shared_ptr<DevToolSlot> Create(
      const std::shared_ptr<DebugRouterMessageSubscriber>& delegate);
  explicit DevToolSlot(
      const std::shared_ptr<DebugRouterMessageSubscriber>& delegate);
  virtual ~DevToolSlot() = default;

  // view start
  virtual int32_t Plug(const std::string& url) = 0;

  // view destroy
  virtual void Pull() = 0;
  virtual void OnMessage(const std::string& type, const std::string& msg);
  virtual void SendMessage(const std::string& type, const std::string& msg) = 0;

 protected:
  std::weak_ptr<DebugRouterMessageSubscriber> delegate_;
};
}  // namespace devtool
}  // namespace lynx
#endif  // DEVTOOL_BASE_DEVTOOL_NATIVE_DEVTOOL_SLOT_H_
