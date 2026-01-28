// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_devtool/src/main/cpp/message_handler_harmony.h"

#include <memory>
#include <string>

#include "base/include/fml/message_loop.h"
#include "base/include/fml/task_runner.h"
#include "base/include/log/logging.h"
#include "base/include/platform/harmony/napi_util.h"

namespace lynx {
namespace devtool {

MessageHandlerHarmony::MessageHandlerHarmony(napi_env env, napi_value js_object)
    : env_(env) {
  napi_create_reference(env_, js_object, 0, &ref_);
  napi_get_uv_event_loop(env_, &loop_);
}

void MessageHandlerHarmony::OnMessage(const std::string& message) {
  auto ui_task_runner =
      fml::MessageLoop::EnsureInitializedForCurrentThread(loop_)
          .GetTaskRunner();
  if (ui_task_runner == nullptr) {
    return;
  }
  ui_task_runner->PostTask([weak_ref = weak_from_this(), message]() {
    auto sp = weak_ref.lock();
    if (sp == nullptr) {
      return;
    }
    napi_value param[1];
    napi_create_string_utf8(sp->env_, message.c_str(), message.length(),
                            &param[0]);
    napi_status status = base::NapiUtil::InvokeJsMethod(sp->env_, sp->ref_,
                                                        "onMessage", 1, param);
    if (status != napi_ok) {
      LOGE("InvokeJsMethod onMessage failed");
    }
  });
}

}  // namespace devtool
}  // namespace lynx
