// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/global_devtool_platform_facade.h"

#include "devtool/base_devtool/native/public/abstract_devtool.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator_base.h"

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

void GlobalDevToolPlatformFacade::SendHSRMessageReceived(
    const std::string& message) {
  auto runner = LynxDevToolMediatorBase::GetDevToolsThread().GetTaskRunner();
  if (!runner) {
    return;
  }
  fml::TaskRunner::RunNowOrPostTask(runner, [message] {
    auto sender = AbstractDevTool::GetGlobalSender();
    if (sender) {
      Json::Value event(Json::objectValue);
      event["method"] = "HSR.messageReceived";
      event["params"]["message"] = message;
      sender->SendMessage("CDP", event);
    }
  });
}

void GlobalDevToolPlatformFacade::LoadHSRScriptFromSchema(
    const Json::Value& params, HSRScriptCallback callback) {
  HSRScriptRequest request;
  std::string error;
  if (!ParseHSRSchemaLoad(params, request, error)) {
    if (callback) {
      std::move(callback)("", error);
    }
    return;
  }
  HandleHSRScript(std::move(request), std::move(callback));
}

}  // namespace devtool
}  // namespace lynx
