// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bindings/modules/harmony/platform_module_manager.h"

#include <utility>

#include "base/include/fml/message_loop.h"
#include "base/include/log/logging.h"
#include "base/include/platform/harmony/napi_util.h"
#include "base/threading/task_runner_manufactor.h"

namespace lynx {
namespace harmony {

PlatformModuleManager::PlatformModuleManager(napi_env env,
                                             napi_value module_args[5],
                                             napi_value sendable_module_args[5])
    : env_(env) {
  napi_create_reference(env, module_args[0], 0, &js_module_manager_);
  napi_create_reference(env, module_args[1], 0, &js_get_module_);
  AddPlatformModules(module_args[2], module_args[3], module_args[4], false);
  napi_value undefined;
  napi_get_undefined(env, &undefined);
  napi_serialize(env_, sendable_module_args[0], undefined, undefined,
                 &sendable_js_module_manager_buffer_);
  napi_serialize(env_, sendable_module_args[1], undefined, undefined,
                 &sendable_js_module_buffer_);
  AddPlatformModules(sendable_module_args[2], sendable_module_args[3],
                     sendable_module_args[4], true);
}

PlatformModuleManager::~PlatformModuleManager() {
  LOGI("~PlatformModuleManager");
  napi_env sendable_js_module_manager_env = nullptr;
  napi_ref sendable_js_module_manager = nullptr;
  fml::RefPtr<fml::TaskRunner> sendable_js_module_manager_runner;
  napi_env sendable_js_get_module_env = nullptr;
  napi_ref sendable_js_get_module = nullptr;
  fml::RefPtr<fml::TaskRunner> sendable_js_get_module_runner;
  void* sendable_js_module_manager_buffer = nullptr;
  void* sendable_js_module_buffer = nullptr;
  {
    std::lock_guard<std::mutex> lock(sendable_mutex_);
    sendable_js_module_manager_env =
        std::exchange(sendable_js_module_manager_env_, nullptr);
    sendable_js_module_manager =
        std::exchange(sendable_js_module_manager_, nullptr);
    sendable_js_module_manager_runner =
        std::exchange(sendable_js_module_manager_runner_, nullptr);
    sendable_js_get_module_env =
        std::exchange(sendable_js_get_module_env_, nullptr);
    sendable_js_get_module = std::exchange(sendable_js_get_module_, nullptr);
    sendable_js_get_module_runner =
        std::exchange(sendable_js_get_module_runner_, nullptr);
    sendable_js_module_manager_buffer =
        std::exchange(sendable_js_module_manager_buffer_, nullptr);
    sendable_js_module_buffer =
        std::exchange(sendable_js_module_buffer_, nullptr);
  }
  auto delete_ref = [](napi_env env, napi_ref ref,
                       fml::RefPtr<fml::TaskRunner> runner) {
    if (!ref || !env) {
      return;
    }
    auto task = [env, ref]() { napi_delete_reference(env, ref); };
    if (runner) {
      fml::TaskRunner::RunNowOrPostTask(runner, std::move(task));
    } else {
      task();
    }
  };
  delete_ref(sendable_js_module_manager_env, sendable_js_module_manager,
             sendable_js_module_manager_runner);
  delete_ref(sendable_js_get_module_env, sendable_js_get_module,
             sendable_js_get_module_runner);
  fml::TaskRunner::RunNowOrPostTask(
      base::UIThread::GetRunner(),
      [env = env_, js_module_manager = js_module_manager_,
       js_get_module = js_get_module_,
       sendable_js_module_manager_buffer = sendable_js_module_manager_buffer,
       sendable_js_module_buffer = sendable_js_module_buffer]() {
        napi_delete_reference(env, js_module_manager);
        napi_delete_reference(env, js_get_module);
        if (sendable_js_module_manager_buffer) {
          napi_delete_serialization_data(env,
                                         sendable_js_module_manager_buffer);
        }
        if (sendable_js_module_buffer) {
          napi_delete_serialization_data(env, sendable_js_module_buffer);
        }
      });
}

napi_value PlatformModuleManager::JSModuleManager(napi_env env, bool sendable) {
  napi_value result = nullptr;
  if (sendable) {
    result = EnsureSendable(
        env, sendable_js_module_manager_buffer_, sendable_js_module_manager_,
        sendable_js_module_manager_env_, sendable_js_module_manager_runner_,
        sendable_js_module_manager_initializing_, sendable_mutex_,
        sendable_cv_);
  } else {
    napi_get_reference_value(env, js_module_manager_, &result);
  }
  return result;
}

napi_value PlatformModuleManager::JSGetModuleFunc(napi_env env, bool sendable) {
  napi_value result = nullptr;
  if (sendable) {
    result = EnsureSendable(
        env, sendable_js_module_buffer_, sendable_js_get_module_,
        sendable_js_get_module_env_, sendable_js_get_module_runner_,
        sendable_js_get_module_initializing_, sendable_mutex_, sendable_cv_);
  } else {
    napi_get_reference_value(env, js_get_module_, &result);
  }
  return result;
}

void PlatformModuleManager::AddPlatformModules(napi_value module_key,
                                               napi_value module_value,
                                               napi_value sync_methods_value,
                                               bool sendable) {
  DCHECK(base::NapiUtil::IsArray(env_, module_key));
  DCHECK(base::NapiUtil::IsArray(env_, module_value));
  DCHECK(base::NapiUtil::IsArray(env_, sync_methods_value));
  uint32_t length;
  napi_get_array_length(env_, module_key, &length);
  for (uint32_t i = 0; i < length; i++) {
    napi_value module;
    napi_get_element(env_, module_key, i, &module);
    napi_value methods;
    napi_get_element(env_, module_value, i, &methods);
    napi_value sync_methods;
    napi_get_element(env_, sync_methods_value, i, &sync_methods);
    DCHECK(base::NapiUtil::IsArray(env_, methods));

    std::string key = base::NapiUtil::ConvertToShortString(env_, module);
    std::vector<std::string> method_names;
    base::NapiUtil::ConvertToArrayString(env_, methods, method_names);

    auto& info = js_module_map_[key];
    info.sendable = sendable;
    info.methods = std::move(method_names);
    info.sync_methods.clear();
    if (base::NapiUtil::IsArray(env_, sync_methods)) {
      std::vector<std::string> sync_method_names;
      base::NapiUtil::ConvertToArrayString(env_, sync_methods,
                                           sync_method_names);
      info.sync_methods.insert(sync_method_names.begin(),
                               sync_method_names.end());
    }
  }
}

napi_value PlatformModuleManager::EnsureSendable(
    napi_env env, void*& buffer, napi_ref& ref, napi_env& ref_env,
    fml::RefPtr<fml::TaskRunner>& ref_task_runner, bool& initializing,
    std::mutex& mutex, std::condition_variable& cv) {
  std::lock_guard<std::mutex> lock(mutex);
  napi_value result = nullptr;
  // A PlatformModuleManager is scoped to one renderer ArkTS context. The
  // sendable path lazily materializes one napi_ref per serialized handle and
  // rejects cross-env reuse instead of sharing a napi_ref across napi_envs.
  if (!ref) {
    if (!buffer) {
      LOGE("EnsureSendable failed: both ref and buffer are null");
      return nullptr;
    }
    napi_value value = nullptr;
    if (napi_deserialize(env, buffer, &value) != napi_ok) {
      LOGE("EnsureSendable failed: napi_deserialize failed");
      return nullptr;
    }
    if (napi_create_reference(env, value, 0, &ref) != napi_ok) {
      LOGE("EnsureSendable failed: napi_create_reference failed");
      return nullptr;
    }
    ref_env = env;
    if (auto* message_loop =
            fml::MessageLoop::IsInitializedForCurrentThread()) {
      ref_task_runner = message_loop->GetTaskRunner();
    } else {
      LOGE("EnsureSendable failed: current thread has no message loop");
      napi_delete_reference(env, ref);
      ref = nullptr;
      ref_env = nullptr;
      return nullptr;
    }
    napi_delete_serialization_data(env, buffer);
    buffer = nullptr;
  } else if (ref_env != env) {
    LOGE("EnsureSendable failed: napi_ref belongs to a different napi_env");
    return nullptr;
  }
  if (napi_get_reference_value(env, ref, &result) != napi_ok) {
    return nullptr;
  }
  return result;
}

}  // namespace harmony
}  // namespace lynx
