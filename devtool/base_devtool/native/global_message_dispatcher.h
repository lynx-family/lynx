// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_BASE_DEVTOOL_NATIVE_GLOBAL_MESSAGE_DISPATCHER_H_
#define DEVTOOL_BASE_DEVTOOL_NATIVE_GLOBAL_MESSAGE_DISPATCHER_H_
#include <memory>

#include "devtool/base_devtool/native/global_message_channel.h"
#include "devtool/base_devtool/native/public/devtool_message_dispatcher.h"

namespace lynx {
namespace devtool {
// for global message registration and dispatching
class GlobalMessageDispatcher : public DevToolMessageDispatcher {
 public:
  static std::shared_ptr<GlobalMessageDispatcher> Create();
  virtual ~GlobalMessageDispatcher();

 private:
  GlobalMessageDispatcher();
  void Initialize();
  std::shared_ptr<GlobalMessageChannel> global_message_channel_;
};
}  // namespace devtool
}  // namespace lynx
#endif  // DEVTOOL_BASE_DEVTOOL_NATIVE_GLOBAL_MESSAGE_DISPATCHER_H_
