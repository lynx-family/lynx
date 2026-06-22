// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "platform/embedder/switch_persist.h"

#include "core/renderer/utils/lynx_env.h"

namespace lynx {
namespace embedder {
bool SwitchPersist::SetValueToPersistent(
    [[maybe_unused]] const std::string& key, [[maybe_unused]] bool value) {
  return false;
}

bool SwitchPersist::GetValueFromPersistent(const std::string& key,
                                           bool default_value) {
  // This is a process-local persistence mock for platforms without real
  // embedder persistence, such as NodeLynx/Linux. It preserves the old
  // SetBoolLocalEnv/GetBoolEnv read-after-write behavior without adding durable
  // storage.
  auto& env = lynx::tasm::LynxEnv::GetInstance();
  return env.GetBoolEnv(key, default_value);
}
}  // namespace embedder
}  // namespace lynx
