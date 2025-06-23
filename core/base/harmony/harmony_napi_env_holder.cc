// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/harmony/harmony_napi_env_holder.h"

#include <napi/native_api.h>

#include "base/include/log/logging.h"

namespace lynx {
namespace base {
namespace harmony {
namespace {
napi_env& Holder() {
  static thread_local napi_env env = nullptr;
  return env;
}
}  // namespace

napi_env GetJSThreadNapiEnv() {
  DCHECK(Holder());
  return Holder();
}

void InitializationNapiEnvForCurrentThread(napi_env env) {
  LOGI("InitializationNapiEnvForCurrentThread with " << env);
  Holder() = env;
}

}  // namespace harmony
}  // namespace base
}  // namespace lynx
