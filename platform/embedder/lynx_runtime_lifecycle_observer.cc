// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "platform/embedder/lynx_runtime_lifecycle_observer_priv.h"
#include "third_party/weak-node-api/headers/napi.h"

#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_defines.h"
#endif

LYNX_EXTERN_C lynx_runtime_lifecycle_observer_t*
lynx_runtime_lifecycle_observer_create(void* user_data) {
  return lynx_runtime_lifecycle_observer_create_with_finalizer(user_data,
                                                               nullptr);
}

LYNX_EXTERN_C lynx_runtime_lifecycle_observer_t*
lynx_runtime_lifecycle_observer_create_with_finalizer(
    void* user_data,
    void (*finalizer)(lynx_runtime_lifecycle_observer_t*, void*)) {
  auto* observer = new lynx_runtime_lifecycle_observer_t;
  observer->user_data = user_data;
  observer->finalizer = finalizer;
  // The ref count has been initialized to 1, there is no need to call AddRef.
  return observer;
}

LYNX_EXTERN_C void* lynx_runtime_lifecycle_observer_get_user_data(
    lynx_runtime_lifecycle_observer_t* observer) {
  return observer->user_data;
}

LYNX_EXTERN_C void lynx_runtime_lifecycle_observer_bind_attach_callback(
    lynx_runtime_lifecycle_observer_t* observer,
    runtime_attach_callback callback) {
  observer->attach_callback = callback;
}

LYNX_EXTERN_C void lynx_runtime_lifecycle_observer_bind_detach_callback(
    lynx_runtime_lifecycle_observer_t* observer,
    runtime_detach_callback callback) {
  observer->detach_callback = callback;
}

LYNX_EXTERN_C void lynx_runtime_lifecycle_observer_release(
    lynx_runtime_lifecycle_observer_t* observer) {
  // Unref the observer.
  observer->Release();
}

lynx_runtime_lifecycle_observer_t::~lynx_runtime_lifecycle_observer_t() {
  if (finalizer) {
    finalizer(this, user_data);
  }
}

namespace lynx {
namespace embedder {

void NapiEnvHolder::OnRuntimeAttach(napi_env env) { env_ = env; }

void NapiEnvHolder::OnRuntimeDetach() { env_ = nullptr; }

LynxRuntimeLifecycleListenerDelegate::LynxRuntimeLifecycleListenerDelegate()
    : RuntimeLifecycleListenerDelegate(
          RuntimeLifecycleListenerDelegate::DelegateType::FULL),
      env_holder_(std::make_shared<NapiEnvHolder>()) {}

LynxRuntimeLifecycleListenerDelegate::LynxRuntimeLifecycleListenerDelegate(
    lynx_runtime_lifecycle_observer_t* observer,
    std::function<void(napi_env env)> on_attach_callback)
    : RuntimeLifecycleListenerDelegate(
          RuntimeLifecycleListenerDelegate::DelegateType::FULL),
      observer_(observer),
      env_holder_(std::make_shared<NapiEnvHolder>()),
      on_attach_callback_(on_attach_callback) {
  if (observer_) {
    // Ref the observer.
    observer_->AddRef();
  }
}

LynxRuntimeLifecycleListenerDelegate::~LynxRuntimeLifecycleListenerDelegate() {
  if (observer_) {
    // Unref the observer.
    observer_->Release();
  }
}

void LynxRuntimeLifecycleListenerDelegate::OnRuntimeCreate(
    std::shared_ptr<runtime::IVSyncObserver> observer) {}

void LynxRuntimeLifecycleListenerDelegate::OnRuntimeInit(int64_t runtime_id) {}

void LynxRuntimeLifecycleListenerDelegate::OnAppEnterForeground() {}

void LynxRuntimeLifecycleListenerDelegate::OnAppEnterBackground() {}

void LynxRuntimeLifecycleListenerDelegate::OnRuntimeAttach(void* env) {
  env_holder_->OnRuntimeAttach(static_cast<napi_env>(env));
  if (observer_ && observer_->attach_callback) {
    observer_->attach_callback(observer_, static_cast<napi_env>(env));
  }
  if (on_attach_callback_) {
    on_attach_callback_(static_cast<napi_env>(env));
  }
}

void LynxRuntimeLifecycleListenerDelegate::OnRuntimeDetach() {
  if (observer_ && observer_->detach_callback) {
    observer_->detach_callback(observer_);
  }
  env_holder_->OnRuntimeDetach();
}

}  // namespace embedder
}  // namespace lynx

#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_undefs.h"
#endif
