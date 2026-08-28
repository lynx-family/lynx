// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "platform/harmony/lynx_harmony/src/main/cpp/static_task_napi_bridge.h"

#include <napi/native_api.h>

#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>

#include "base/include/fml/hash_combine.h"
#include "base/include/no_destructor.h"
#include "base/include/platform/harmony/napi_util.h"

namespace lynx {
namespace harmony {
namespace {

constexpr size_t kMaxCachedTasks = 128;

struct TaskKey {
  std::string module_path;
  std::string module_info;
  std::string class_name;

  bool operator==(const TaskKey& other) const {
    return module_path == other.module_path &&
           module_info == other.module_info && class_name == other.class_name;
  }
};

struct TaskKeyHash {
  size_t operator()(const TaskKey& key) const {
    return fml::HashCombine(std::string_view(key.module_path),
                            std::string_view(key.module_info),
                            std::string_view(key.class_name));
  }
};

struct CachedTask {
  napi_ref receiver_ref = nullptr;
  napi_ref function_ref = nullptr;
};

class CacheState {
 public:
  void Init(napi_env env) {
    if (env_ == env) {
      return;
    }

    // StaticTaskNapiBridge historically supports one active env. A new env may
    // only replace state that has already been detached by its cleanup hook.
    if (env_ != nullptr) {
      return;
    }

    env_ = env;
    // NAPI values and references must be accessed from the thread that owns the
    // environment. Remember the initialization thread and reject calls from
    // other threads instead of racing the process-wide cache.
    owner_thread_id_ = std::this_thread::get_id();
    if (napi_add_env_cleanup_hook(env_, Cleanup, this) != napi_ok) {
      env_ = nullptr;
      owner_thread_id_ = std::thread::id();
    }
  }

  napi_env env() const {
    return owner_thread_id_ == std::this_thread::get_id() ? env_ : nullptr;
  }

  CachedTask* Find(const TaskKey& key) {
    auto it = tasks_.find(key);
    return it == tasks_.end() ? nullptr : &it->second;
  }

  bool Cache(TaskKey key, napi_value receiver, napi_value function) {
    if (tasks_.size() >= kMaxCachedTasks) {
      return false;
    }

    CachedTask cached_task;
    if (napi_create_reference(env_, receiver, 1, &cached_task.receiver_ref) !=
        napi_ok) {
      return false;
    }
    if (napi_create_reference(env_, function, 1, &cached_task.function_ref) !=
        napi_ok) {
      napi_delete_reference(env_, cached_task.receiver_ref);
      return false;
    }
    tasks_.emplace(std::move(key), cached_task);
    return true;
  }

 private:
  static void Cleanup(void* arg) {
    auto* state = static_cast<CacheState*>(arg);
    // References belong to the env and are released with it. Clearing only the
    // native state avoids invoking NAPI during teardown or deleting twice.
    state->tasks_.clear();
    state->env_ = nullptr;
    state->owner_thread_id_ = std::thread::id();
  }

  napi_env env_ = nullptr;
  std::thread::id owner_thread_id_;
  std::unordered_map<TaskKey, CachedTask, TaskKeyHash> tasks_;
};

CacheState& GetCacheState() {
  // The bridge entry points are static and the NAPI module currently owns one
  // active environment, so one process-wide state object mirrors that lifetime.
  // NoDestructor keeps the state alive until the environment cleanup hook
  // detaches it; this avoids a static destructor touching NAPI after teardown.
  static base::NoDestructor<CacheState> state;
  return *state;
}

bool ResolveTask(napi_env env, const TaskKey& key, napi_value* receiver,
                 napi_value* function) {
  if (napi_load_module_with_info(env, key.module_path.c_str(),
                                 key.module_info.c_str(),
                                 receiver) != napi_ok ||
      *receiver == nullptr) {
    return false;
  }

  napi_value export_class = nullptr;
  if (napi_get_named_property(env, *receiver, key.class_name.c_str(),
                              &export_class) != napi_ok ||
      export_class == nullptr) {
    return false;
  }

  return napi_get_named_property(env, export_class, "task", function) ==
             napi_ok &&
         *function != nullptr;
}

}  // namespace

void StaticTaskNapiBridge::Init(napi_env env, napi_value exports) {
  GetCacheState().Init(env);
}

bool StaticTaskNapiBridge::LoadAndInvokeTask(const std::string& module_path,
                                             const std::string& module_info,
                                             const std::string& class_name,
                                             std::intptr_t native_context_ptr) {
  napi_env env = GetCacheState().env();
  if (!env) {
    return false;
  }

  base::NapiHandleScope scope(env);
  const TaskKey key{module_path, module_info, class_name};
  napi_value receiver = nullptr;
  napi_value function = nullptr;

  CacheState& state = GetCacheState();
  CachedTask* cached_task = state.Find(key);
  if (cached_task != nullptr) {
    if (napi_get_reference_value(env, cached_task->receiver_ref, &receiver) !=
            napi_ok ||
        receiver == nullptr ||
        napi_get_reference_value(env, cached_task->function_ref, &function) !=
            napi_ok ||
        function == nullptr) {
      return false;
    }
  } else {
    if (!ResolveTask(env, key, &receiver, &function)) {
      return false;
    }
    // A full cache still executes the resolved task and only skips retention.
    state.Cache(std::move(key), receiver, function);
  }

  napi_value args[1] = {nullptr};
  // Pointers must be passed as BigInt on HarmonyOS to avoid precision loss.
  if (napi_create_bigint_int64(env, static_cast<int64_t>(native_context_ptr),
                               &args[0]) != napi_ok) {
    return false;
  }

  napi_value result = nullptr;
  // Keep the module exports object as receiver to preserve the existing `this`.
  return napi_call_function(env, receiver, function, 1, args, &result) ==
         napi_ok;
}

}  // namespace harmony
}  // namespace lynx
