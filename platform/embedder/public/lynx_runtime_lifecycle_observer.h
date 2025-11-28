// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_LYNX_RUNTIME_LIFECYCLE_OBSERVER_H_
#define PLATFORM_EMBEDDER_PUBLIC_LYNX_RUNTIME_LIFECYCLE_OBSERVER_H_

#include <memory>

#include "capi/lynx_runtime_lifecycle_observer_capi.h"

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

namespace lynx {
namespace pub {

/**
 * @apidoc
 * @brief `LynxRuntimeLifecycleObserver` is the observer class for the
 * lifecycle events of the Lynx runtime.
 */
class LynxRuntimeLifecycleObserver
    : public std::enable_shared_from_this<LynxRuntimeLifecycleObserver> {
 public:
  LynxRuntimeLifecycleObserver() = default;
  virtual ~LynxRuntimeLifecycleObserver() {
    if (observer_) {
      lynx_runtime_lifecycle_observer_release(observer_);
    }
  }

  void InitIfNeeded() {
    if (observer_) {
      return;
    }
    // Hold a weak pointer.
    observer_ = lynx_runtime_lifecycle_observer_create_with_finalizer(
        new std::weak_ptr<LynxRuntimeLifecycleObserver>(weak_from_this()),
        [](lynx_runtime_lifecycle_observer_t*, void* user_data) {
          std::weak_ptr<LynxRuntimeLifecycleObserver>* weak_ptr =
              reinterpret_cast<std::weak_ptr<LynxRuntimeLifecycleObserver>*>(
                  user_data);
          delete weak_ptr;
        });

    lynx_runtime_lifecycle_observer_bind_attach_callback(
        observer_,
        [](lynx_runtime_lifecycle_observer_t* observer, napi_env env) {
          std::weak_ptr<LynxRuntimeLifecycleObserver>* weak_ptr =
              reinterpret_cast<std::weak_ptr<LynxRuntimeLifecycleObserver>*>(
                  lynx_runtime_lifecycle_observer_get_user_data(observer));
          std::shared_ptr<LynxRuntimeLifecycleObserver> shared_observer =
              weak_ptr ? weak_ptr->lock() : nullptr;
          if (shared_observer) {
            shared_observer->OnRuntimeAttach(env);
          }
        });
    lynx_runtime_lifecycle_observer_bind_detach_callback(
        observer_, [](lynx_runtime_lifecycle_observer_t* observer) {
          std::weak_ptr<LynxRuntimeLifecycleObserver>* weak_ptr =
              reinterpret_cast<std::weak_ptr<LynxRuntimeLifecycleObserver>*>(
                  lynx_runtime_lifecycle_observer_get_user_data(observer));
          std::shared_ptr<LynxRuntimeLifecycleObserver> shared_observer =
              weak_ptr ? weak_ptr->lock() : nullptr;
          if (shared_observer) {
            shared_observer->OnRuntimeDetach();
          }
        });
  }

  /**
   * @apidoc
   * @brief Callback function invoked when the runtime is attached.
   * Derived classes can override this method to perform custom operations
   * when the runtime attaches.
   * @param env The Napi environment.
   */
  virtual void OnRuntimeAttach(napi_env env) {}

  /**
   * @apidoc
   * @brief Callback function invoked when the runtime is detached.
   * Derived classes can override this method to perform custom cleanup
   * operations when the runtime detaches.
   */
  virtual void OnRuntimeDetach() {}

  lynx_runtime_lifecycle_observer_t* Impl() { return observer_; }

 private:
  lynx_runtime_lifecycle_observer_t* observer_ = nullptr;
};

}  // namespace pub
}  // namespace lynx

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_undefs.h"
#endif

#endif  // PLATFORM_EMBEDDER_PUBLIC_LYNX_RUNTIME_LIFECYCLE_OBSERVER_H_
