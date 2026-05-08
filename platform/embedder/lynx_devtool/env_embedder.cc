// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/embedder/core/env_embedder.h"

#include "platform/embedder/lynx_devtool/devtool_env_embedder.h"

namespace lynx {
namespace devtool {

void EnvEmbedder::SetSwitch(const std::string& key, bool value) {
  embedder::DevToolEnvEmbedder::GetInstance().SetDevToolSwitch(key, value);
}

bool EnvEmbedder::GetSwitch(const std::string& key) {
  return embedder::DevToolEnvEmbedder::GetInstance().GetDevToolSwitch(key);
}

}  // namespace devtool
}  // namespace lynx
