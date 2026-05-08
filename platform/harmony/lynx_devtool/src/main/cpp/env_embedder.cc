// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/embedder/core/env_embedder.h"

#include "platform/harmony/lynx_devtool/src/main/cpp/devtool_env_harmony.h"

namespace lynx {
namespace devtool {

// TODO(mitchilling): Route EnvEmbedder through the new settings model after the
// shared embedder refactor covers macOS, Windows, and Harmony.
void EnvEmbedder::SetSwitch(const std::string &key, bool value) {
  DevToolEnvHarmony::GetInstance().SetSwitch(
      key, value, DevToolEnvHarmony::NeedPersistent(key));
}

bool EnvEmbedder::GetSwitch(const std::string &key) {
  return DevToolEnvHarmony::GetInstance().GetSwitch(key);
}

}  // namespace devtool
}  // namespace lynx
