// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/base_devtool/native/global_message_dispatcher.h"

#include <iostream>

#include "devtool/base_devtool/native/global_message_channel.h"

namespace lynx {
namespace devtool {

std::shared_ptr<GlobalMessageDispatcher> GlobalMessageDispatcher::Create() {
  std::shared_ptr<GlobalMessageDispatcher> global_message_dispatcher(
      new GlobalMessageDispatcher());
  global_message_dispatcher->Initialize();
  return global_message_dispatcher;
}

void GlobalMessageDispatcher::Initialize() {
  global_message_channel_ = GlobalMessageChannel::Create(shared_from_this());
}

GlobalMessageDispatcher::GlobalMessageDispatcher() {}

GlobalMessageDispatcher::~GlobalMessageDispatcher() {}

}  // namespace devtool
}  // namespace lynx
