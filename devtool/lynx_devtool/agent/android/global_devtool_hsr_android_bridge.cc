// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <utility>

#include "core/shell/host_script/runtime/platform/android/process_runtime_android.h"
#include "devtool/lynx_devtool/agent/android/global_devtool_platform_android.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator_base.h"

namespace lynx {
namespace devtool {
void GlobalDevToolPlatformAndroid::HandleHSRScript(HSRScriptRequest request,
                                                   HSRScriptCallback callback) {
  const auto* transport =
      reinterpret_cast<const shell::HostScriptTransport*>(GetHSRTransport());
  if (!transport) {
    GlobalDevToolPlatformFacade::HandleHSRScript(std::move(request),
                                                 std::move(callback));
    return;
  }
  transport->handle(
      std::move(request), std::move(callback),
      LynxDevToolMediatorBase::GetDevToolsThread().GetTaskRunner(),
      FetchHSRScript);
}

void GlobalDevToolPlatformAndroid::OnHSRScriptFetched(JNIEnv* env,
                                                      jlong request_id,
                                                      jbyteArray bytes,
                                                      jbyteArray error) {
  const auto* transport =
      reinterpret_cast<const shell::HostScriptTransport*>(GetHSRTransport());
  if (transport)
    transport->fetched(
        env, request_id, bytes, error,
        LynxDevToolMediatorBase::GetDevToolsThread().GetTaskRunner());
}
}  // namespace devtool
}  // namespace lynx
