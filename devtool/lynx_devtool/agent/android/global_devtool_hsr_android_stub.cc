// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <utility>

#include "core/renderer/utils/devtool_lifecycle.h"
#include "devtool/lynx_devtool/agent/android/global_devtool_platform_android.h"

namespace lynx {
namespace devtool {
void GlobalDevToolPlatformAndroid::OnHSRScriptFetched(JNIEnv*, jlong,
                                                      jbyteArray, jbyteArray) {}

void GlobalDevToolPlatformAndroid::HandleHSRScript(HSRScriptRequest request,
                                                   HSRScriptCallback callback) {
  if (callback &&
      request.operation == HSRScriptRequest::Operation::kGetStatus) {
    Json::Value result(Json::objectValue);
    result["available"] = false;
    result["enabled"] = tasm::DevToolLifecycle::GetInstance().IsEnabled();
    for (const char* domain : {"bts", "mts", "ui"})
      result["ready"][domain] = false;
    result["loaded"] = false;
    result["stopping"] = false;
    result["pending"] = 0;
    std::move(callback)(std::move(result), "");
    return;
  }
  GlobalDevToolPlatformFacade::HandleHSRScript(std::move(request),
                                               std::move(callback));
}
}  // namespace devtool
}  // namespace lynx
