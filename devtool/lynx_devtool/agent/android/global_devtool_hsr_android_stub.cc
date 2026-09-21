// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <utility>

#include "devtool/lynx_devtool/agent/android/global_devtool_hsr_android.h"
#include "devtool/lynx_devtool/agent/android/global_devtool_platform_android.h"

namespace lynx {
namespace devtool {
void GlobalDevToolPlatformAndroid::HandleHSRScript(HSRScriptRequest request,
                                                   HSRScriptCallback callback) {
  GlobalDevToolPlatformFacade::HandleHSRScript(std::move(request),
                                               std::move(callback));
}
void CompleteHSRScriptSource(int64_t, std::string, std::string) {}
}  // namespace devtool
}  // namespace lynx
