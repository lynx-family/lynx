// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/global_devtool_platform_facade.h"

#include <atomic>

#include "base/include/string/string_utils.h"
#include "core/services/recorder/record_payload.h"
#include "devtool/base_devtool/native/public/abstract_devtool.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator_base.h"
#include "third_party/modp_b64/modp_b64.h"

namespace lynx {
namespace devtool {

std::string GlobalDevToolPlatformFacade::SendRecordPayload(
    const base::LogContext& context, const char* version, std::string payload) {
  static std::atomic<uint64_t> sequence{0};
  if (!tasm::recorder::GetRecordPayloadCallback()) return {};
  const std::string id =
      std::to_string(sequence.fetch_add(1, std::memory_order_relaxed));
  LynxDevToolMediatorBase::GetDevToolsThread().GetTaskRunner()->PostTask(
      [context, id, version = std::string(version),
       payload = std::move(payload)]() mutable {
        if (tasm::recorder::GetRecordPayloadCallback()) {
          Json::Value event;
          event["method"] = "Lynx.observePayload";
          auto& params = event["params"];
          params["payloadId"] = id;
          params["logVersion"] = version;
          params["viewId"] = context.view_id;
          params["engineId"] = context.engine_id;
          params["runtimeId"] = context.runtime_id;
          params["bytes"] = Json::UInt64(payload.size());
          const bool text = base::IsValidUtf8(
              reinterpret_cast<const uint8_t*>(payload.data()), payload.size());
          params["encoding"] = text ? "utf8" : "base64";
          if (!text) {
            std::string encoded(lynx_modp_b64_encode_len(payload.size()), '\0');
            encoded.resize(lynx_modp_b64_encode(encoded.data(), payload.data(),
                                                payload.size()));
            payload = std::move(encoded);
          }
          params["data"] = std::move(payload);
          AbstractDevTool::GetGlobalSender()->SendMessage("CDP", event);
        }
      });
  return id;
}

void GlobalDevToolPlatformFacade::HandleHSRScript(HSRScriptRequest request,
                                                  HSRScriptCallback callback) {
  // TODO(hsr): Connect the runtime once its ownership and execution API are
  // agreed. A load must replace the script while preserving live View bindings;
  // evaluate must run in the current context. Do not acknowledge either early.
  if (callback) {
    std::move(callback)(Json::Value(), "HSR runtime is not connected");
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
      std::move(callback)(Json::Value(), error);
    }
    return;
  }
  HandleHSRScript(std::move(request), std::move(callback));
}

}  // namespace devtool
}  // namespace lynx
