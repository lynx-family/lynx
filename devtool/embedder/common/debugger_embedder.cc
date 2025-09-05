// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/embedder/common/debugger_embedder.h"

#include "core/renderer/utils/lynx_env.h"

namespace lynx {
namespace devtool {

static std::atomic_bool g_devtools_enabled = false;

void DebuggerEmbedder::SetDevtoolsEnabled(bool enabled) {
  if (g_devtools_enabled == enabled) {
    return;
  }
  g_devtools_enabled = enabled;

  if (g_devtools_enabled) {
    lynx::tasm::LynxEnv::GetInstance().SetBoolLocalEnv("enable_devtool",
                                                       enabled);
    lynx::tasm::LynxEnv::GetInstance().SetBoolLocalEnv(
        "enable_devtool_for_debuggable_view", enabled);
    lynx::tasm::LynxEnv::GetInstance().SetBoolLocalEnv(
        "devtool_component_attach", enabled);
#if (OS_WIN || OS_OSX) && JS_ENGINE_TYPE == 0
    lynx::tasm::LynxEnv::GetInstance().SetBoolLocalEnv("enable_v8", true);
#endif
  }
}

bool DebuggerEmbedder::IsDevtoolsEnabled() { return g_devtools_enabled; }

bool DebuggerEmbedder::ConnectDevtools(
    const std::string& url,
    const std::unordered_map<std::string, std::string>& options) {
  if (!IsDevtoolsEnabled()) {
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
