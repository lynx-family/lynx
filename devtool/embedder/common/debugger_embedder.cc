// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/embedder/common/debugger_embedder.h"

#include "core/renderer/utils/lynx_env.h"
#include "devtool/embedder/core/env_embedder.h"

namespace lynx {
namespace devtool {

bool DebuggerEmbedder::ConnectDevtools(
    const std::string& url,
    const std::unordered_map<std::string, std::string>& options) {
  if (!EnvEmbedder::GetSwitch(tasm::LynxEnv::kLynxDevToolEnable)) {
    LOGW("ConnectDevtools failed, due to devtools not enabled!");
    return false;
  }

  return DebugBridgeEmbedder::GetInstance().Enable(url, options);
}

void DebuggerEmbedder::SetOpenCardCallback(DevtoolsOpenCardCallback callback) {
  DebugBridgeEmbedder::GetInstance().SetOpenCardCallback(callback);
}

}  // namespace devtool
}  // namespace lynx
