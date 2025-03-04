// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/ios/lynx_engine_proxy_darwin.h"

namespace lynx {
namespace shell {

void LynxEngineProxyDarwin::InvokeLepusApiCallback(const int32_t callback_id,
                                                   const std::string& entry_name,
                                                   const lepus::Value& data) {
  if (engine_actor_ == nullptr) {
    return;
  }
  engine_actor_->Act([callback_id, entry_name, data](auto& engine) {
    return engine->InvokeLepusCallback(callback_id, entry_name, data);
  });
}
}  // namespace shell
}  // namespace lynx
