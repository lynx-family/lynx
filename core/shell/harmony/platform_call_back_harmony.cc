// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/harmony/platform_call_back_harmony.h"

#include <utility>

#include "base/include/log/logging.h"
#include "base/include/platform/harmony/napi_util.h"
#include "core/base/harmony/napi_convert_helper.h"
#include "core/base/threading/task_runner_manufactor.h"

namespace lynx {
namespace shell {
namespace {

void DeleteReference(napi_env env, napi_ref callback_ref) {
  if (env != nullptr && callback_ref != nullptr) {
    napi_delete_reference(env, callback_ref);
  }
}

void DeleteReferenceOnUIThread(napi_env env, napi_ref callback_ref) {
  if (env == nullptr || callback_ref == nullptr) {
    return;
  }
  auto runner = base::UIThread::GetRunner();
  if (!runner) {
    DeleteReference(env, callback_ref);
    return;
  }
  if (runner->RunsTasksOnCurrentThread()) {
    DeleteReference(env, callback_ref);
    return;
  }
  runner->PostTask(
      [env, callback_ref]() { DeleteReference(env, callback_ref); });
}

napi_value CreateCallbackValue(napi_env env, const lepus::Value& value) {
  napi_value arg = base::NapiConvertHelper::CreateNapiValue(env, value);
  if (arg == nullptr) {
    napi_get_null(env, &arg);
  }
  return arg;
}

void InvokeNapiFunction(napi_env env, napi_ref callback_ref,
                        lepus::Value value) {
  base::NapiHandleScope scope(env);
  napi_value callback =
      base::NapiUtil::GetReferenceNapiValue(env, callback_ref);
  if (callback == nullptr) {
    DeleteReference(env, callback_ref);
    return;
  }

  napi_value js_this = nullptr;
  napi_get_undefined(env, &js_this);

  napi_value arg = CreateCallbackValue(env, value);
  napi_status status =
      napi_call_function(env, js_this, callback, 1, &arg, nullptr);
  if (status != napi_ok) {
    bool has_exception = false;
    napi_is_exception_pending(env, &has_exception);
    if (has_exception) {
      napi_value exception = nullptr;
      napi_get_and_clear_last_exception(env, &exception);
    }
    LOGE("napi_call_function failed: " << base::NapiUtil::StatusToString(status)
                                       << ", Harmony platform callback");
  }
  DeleteReference(env, callback_ref);
}

}  // namespace

PlatformCallBackHarmony::PlatformCallBackHarmony(napi_env env,
                                                 napi_value callback)
    : env_(env) {
  if (env_ == nullptr || callback == nullptr) {
    return;
  }
  napi_status status = napi_create_reference(env_, callback, 1, &callback_ref_);
  if (status != napi_ok) {
    LOGE("napi_create_reference failed: "
         << base::NapiUtil::StatusToString(status)
         << ", Harmony platform callback");
    callback_ref_ = nullptr;
  }
}

PlatformCallBackHarmony::~PlatformCallBackHarmony() {
  DeleteReferenceOnUIThread(env_, callback_ref_);
  callback_ref_ = nullptr;
}

void PlatformCallBackHarmony::InvokeWithValue(const lepus::Value& value) {
  if (env_ == nullptr || callback_ref_ == nullptr) {
    return;
  }
  auto env = env_;
  auto callback_ref = callback_ref_;
  callback_ref_ = nullptr;
  auto runner = base::UIThread::GetRunner();
  if (!runner) {
    DeleteReference(env, callback_ref);
    return;
  }
  runner->PostTask([env, callback_ref, value]() mutable {
    InvokeNapiFunction(env, callback_ref, std::move(value));
  });
}

}  // namespace shell
}  // namespace lynx
