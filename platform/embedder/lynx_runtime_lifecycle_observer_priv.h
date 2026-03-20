// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_LYNX_RUNTIME_LIFECYCLE_OBSERVER_PRIV_H_
#define PLATFORM_EMBEDDER_LYNX_RUNTIME_LIFECYCLE_OBSERVER_PRIV_H_

#include <atomic>
#include <memory>

#include "base/include/fml/memory/ref_counted.h"
#include "core/runtime/js/runtime_lifecycle_listener_delegate.h"
#include "platform/embedder/public/capi/lynx_runtime_lifecycle_observer_capi.h"
#include "third_party/weak-node-api/headers/node_api.h"

#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_defines.h"
#endif

// Use fml::RefCountedThreadSafe to manage the lifetime of
// lynx_runtime_lifecycle_observer_t.
struct lynx_runtime_lifecycle_observer_t
    : public lynx::fml::RefCountedThreadSafe<
          lynx_runtime_lifecycle_observer_t> {
  lynx_runtime_lifecycle_observer_t() = default;
  ~lynx_runtime_lifecycle_observer_t();

  void* user_data = nullptr;
  void (*finalizer)(lynx_runtime_lifecycle_observer_t*, void*) = nullptr;
  runtime_attach_callback attach_callback = nullptr;
  runtime_detach_callback detach_callback = nullptr;
};

namespace lynx {
namespace embedder {

class NapiEnvHolder : public std::enable_shared_from_this<NapiEnvHolder> {
 public:
  NapiEnvHolder() = default;

  void OnRuntimeAttach(napi_env env);
  void OnRuntimeDetach();

  // Get the napi env attached to the runtime, it is necessary to ensure that it
  // is running on JSThread.
  napi_env Env() { return env_; }

 private:
  napi_env env_ = nullptr;
};

class LynxRuntimeLifecycleListenerDelegate
    : public runtime::RuntimeLifecycleListenerDelegate {
 public:
  LynxRuntimeLifecycleListenerDelegate();
  explicit LynxRuntimeLifecycleListenerDelegate(
      lynx_runtime_lifecycle_observer_t* observer,
      std::function<void(napi_env env)> on_attach_callback = nullptr);

  ~LynxRuntimeLifecycleListenerDelegate() override;

  std::shared_ptr<NapiEnvHolder> GetEnvHolder() { return env_holder_; }

  void OnRuntimeCreate(
      std::shared_ptr<runtime::IVSyncObserver> observer) override;
  void OnRuntimeInit(int64_t runtime_id) override;
  void OnAppEnterForeground() override;
  void OnAppEnterBackground() override;
  void OnRuntimeAttach(void* env) override;
  void OnRuntimeDetach() override;

 private:
  lynx_runtime_lifecycle_observer_t* observer_ = nullptr;
  std::shared_ptr<NapiEnvHolder> env_holder_;
  std::function<void(napi_env env)> on_attach_callback_ = nullptr;
};

}  // namespace embedder
}  // namespace lynx

#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_undefs.h"
#endif

#endif  // PLATFORM_EMBEDDER_LYNX_RUNTIME_LIFECYCLE_OBSERVER_PRIV_H_
