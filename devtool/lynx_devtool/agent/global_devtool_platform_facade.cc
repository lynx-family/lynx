// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/global_devtool_platform_facade.h"

namespace lynx {
namespace devtool {

void GlobalDevToolPlatformFacade::HandleHSRScript(HSRScriptRequest request,
                                                  HSRScriptCallback callback) {
  // TODO(hsr): Connect the runtime once its ownership and execution API are
  // agreed. A load must replace the script while preserving live View bindings;
  // evaluate must run in the current context. Do not acknowledge either early.
  if (callback) {
    std::move(callback)("", "HSR runtime is not connected");
  }
}

}  // namespace devtool
}  // namespace lynx
