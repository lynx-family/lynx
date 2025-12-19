// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <napi/native_api.h>
#include <uv.h>

#include "base/include/closure.h"
#include "base/include/fml/message_loop.h"
#include "base/include/fml/thread.h"
#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/base/harmony/harmony_napi_env_holder.h"
#include "core/base/harmony/harmony_trace_event_def.h"
#include "core/base/threading/js_thread_config_getter.h"

namespace lynx {
namespace base {
namespace {
void SetupArkTSRuntime() {
  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, SETUP_ARK_TS_RUNTIME);
  LOGI("Start setup arkts runtime.")

  napi_env js_thread_env;
  napi_status status = napi_create_ark_runtime(&js_thread_env);
  DCHECK(status == napi_ok);
  LOGI("Create arkts runtime with result " << status);

  uv_loop_t* loop;
  napi_get_uv_event_loop(js_thread_env, &loop);

  fml::MessageLoop::EnsureInitializedForCurrentThread(loop);

  harmony::InitializationNapiEnvForCurrentThread(js_thread_env);

  LOGI("Setup arkts runtime done.")
}
}  // namespace

fml::Thread::ThreadConfig GetJSThreadConfig(const std::string& worker_name) {
  return fml::Thread::ThreadConfig{
      worker_name, fml::Thread::ThreadPriority::HIGH,
      std::make_shared<base::closure>(
          static_cast<void (*)()>(SetupArkTSRuntime))};
}
}  // namespace base
}  // namespace lynx
