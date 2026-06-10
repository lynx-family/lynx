// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "platform/embedder/switch_persist.h"

namespace lynx {
namespace embedder {
bool SwitchPersist::SetValueToPersistent(
    [[maybe_unused]] const std::string& key, [[maybe_unused]] bool value) {
  return false;
}

bool SwitchPersist::GetValueFromPersistent(
    [[maybe_unused]] const std::string& key, bool default_value) {
  return default_value;
}
}  // namespace embedder
}  // namespace lynx
