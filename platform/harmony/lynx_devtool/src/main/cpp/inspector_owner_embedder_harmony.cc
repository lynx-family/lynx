// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_devtool/src/main/cpp/inspector_owner_embedder_harmony.h"

#include <memory>
#include <string>

#include "base/include/platform/harmony/napi_util.h"

namespace lynx {
namespace devtool {

InspectorOwnerEmbedderHarmony::InspectorOwnerEmbedderHarmony(napi_env env,
                                                             napi_ref ref)
    : env_(env), ref_(ref) {}

void InspectorOwnerEmbedderHarmony::OnConsoleMessage(
    const std::string& message) {
  auto ui_task_runner = GetUITaskRunner();
  if (ui_task_runner == nullptr) {
    return;
  }
  std::weak_ptr<InspectorOwnerEmbedderHarmony> weak_ref =
      std::static_pointer_cast<InspectorOwnerEmbedderHarmony>(
          shared_from_this());
  lynx::fml::TaskRunner::RunNowOrPostTask(
      ui_task_runner, [weak_ref, message]() {
        auto sp = weak_ref.lock();
        if (sp == nullptr || sp->env_ == nullptr || sp->ref_ == nullptr) {
          return;
        }
        napi_value param[1];
        napi_create_string_utf8(sp->env_, message.c_str(), message.length(),
                                &param[0]);
        napi_status status = base::NapiUtil::InvokeJsMethod(
            sp->env_, sp->ref_, "onConsoleMessage", 1, param, nullptr);
        if (status != napi_ok) {
          LOGE("lepus debug: InvokeJsMethod onConsoleMessage failed!");
        }
      });
}

void InspectorOwnerEmbedderHarmony::OnConsoleObject(const std::string& detail,
                                                    int callback_id) {
  auto ui_task_runner = GetUITaskRunner();
  if (ui_task_runner == nullptr) {
    return;
  }
  std::weak_ptr<InspectorOwnerEmbedderHarmony> weak_ref =
      std::static_pointer_cast<InspectorOwnerEmbedderHarmony>(
          shared_from_this());
  lynx::fml::TaskRunner::RunNowOrPostTask(
      ui_task_runner, [weak_ref, detail, callback_id]() {
        auto sp = weak_ref.lock();
        if (sp == nullptr || sp->env_ == nullptr || sp->ref_ == nullptr) {
          return;
        }
        napi_value param[2];
        napi_create_string_utf8(sp->env_, detail.c_str(), detail.length(),
                                &param[0]);
        napi_create_int32(sp->env_, callback_id, &param[1]);
        napi_status status = base::NapiUtil::InvokeJsMethod(
            sp->env_, sp->ref_, "onConsoleObject", 2, param, nullptr);
        if (status != napi_ok) {
          LOGE("lepus debug: InvokeJsMethod onConsoleObject failed!");
        }
      });
}

void InspectorOwnerEmbedderHarmony::Destroy() {
  env_ = nullptr;
  ref_ = nullptr;
}

}  // namespace devtool
}  // namespace lynx
