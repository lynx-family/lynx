// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/embedder/core/debug_state_listener_embedder.h"

#include "core/renderer/utils/devtool_lifecycle.h"
#include "core/renderer/utils/lynx_env.h"

namespace lynx {
namespace devtool {

using DebugRouter = debugrouter::common::DebugRouter;

void DebugStateListenerEmbedder::OnOpen(
    debugrouter::common::ConnectionType type) {
  tasm::DevToolLifecycle::GetInstance().OnConnected();
  // TODO(mitchilling): remove this value set after lifecycle implemented on all
  // platforms
  tasm::LynxEnv::GetInstance().SetBoolLocalEnv("devtool_connected", true);
}

}  // namespace devtool
}  // namespace lynx
