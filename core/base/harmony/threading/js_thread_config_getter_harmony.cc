// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <napi/native_api.h>
#include <qos/qos.h>
#include <uv.h>

#include <cstring>
#include <memory>
#include <string>

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
static constexpr char kJSThreadName[] = "Lynx_JS";
static constexpr size_t kPrefixLength = sizeof(kJSThreadName) - 1;
static constexpr char kTASMThreadName[] = "Lynx_TASM";
static constexpr size_t kTASMPrefixLength = sizeof(kTASMThreadName) - 1;

bool HasPrefix(const std::string& value, const char* prefix) {
  return value.compare(0, std::strlen(prefix), prefix) == 0;
}

bool ShouldSetupArkTSRuntime(const std::string& worker_name) {
  return HasPrefix(worker_name, kJSThreadName);
}

bool ShouldSetupThreadQoS(const std::string& worker_name) {
  return HasPrefix(worker_name, kJSThreadName) ||
         HasPrefix(worker_name, kTASMThreadName);
}

void SetCurrentThreadUserInteractiveQoS() {
  int result = OH_QoS_SetThreadQoS(QOS_USER_INTERACTIVE);
  if (result != 0) {
    LOGW("Failed to set thread QoS to QOS_USER_INTERACTIVE, result " << result);
  } else {
    LOGI("Set thread QoS to QOS_USER_INTERACTIVE done.")
  }
}

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

std::shared_ptr<base::closure> CreateThreadSetupClosure(
    bool should_setup_arkts_runtime, bool should_setup_qos) {
  return std::make_shared<base::closure>(
      [should_setup_arkts_runtime, should_setup_qos]() {
        if (should_setup_qos) {
          SetCurrentThreadUserInteractiveQoS();
        }
        if (should_setup_arkts_runtime) {
          SetupArkTSRuntime();
        }
      });
}
}  // namespace

fml::Thread::ThreadConfig GetJSThreadConfig(const std::string& worker_name) {
  const bool should_setup_arkts_runtime = ShouldSetupArkTSRuntime(worker_name);
  const bool should_setup_qos = ShouldSetupThreadQoS(worker_name);
  if (!should_setup_arkts_runtime && !should_setup_qos) {
    return fml::Thread::ThreadConfig{
        worker_name, fml::Thread::ThreadPriority::HIGH, nullptr};
  }
  return fml::Thread::ThreadConfig{
      worker_name, fml::Thread::ThreadPriority::HIGH,
      CreateThreadSetupClosure(should_setup_arkts_runtime, should_setup_qos)};
}
}  // namespace base
}  // namespace lynx
