// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_PUBLIC_LYNX_EXTENSION_MODULE_H_
#define PLATFORM_EMBEDDER_PUBLIC_LYNX_EXTENSION_MODULE_H_

#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include "capi/lynx_extension_module_capi.h"
#include "capi/lynx_native_module_capi.h"
#include "lynx_view.h"
#include "third_party/weak-node-api/headers/napi.h"
#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_defines.h"
#endif

namespace lynx {
namespace pub {

using VSyncObserverCallback = std::function<void(int64_t, int64_t)>;

class VSyncObserver {
 public:
  VSyncObserver(lynx_vsync_observer_t* c_observer) : c_observer_(c_observer) {}
  virtual ~VSyncObserver() = default;

  /**
   * @apidoc
   * @brief Set a callback to synchronize with a given VSync.
   * @param id The request instance id.
   * @param callback A functional callback. It is called when the next VSync
   * signal arrives.
   */
  void RequestAnimationFrame(uintptr_t id, VSyncObserverCallback callback) {
    if (!c_observer_) return;
    auto* user_data =
        new std::function<void(int64_t, int64_t)>(std::move(callback));
    lynx_vsync_observer_request_animation_frame(
        c_observer_, id, &RequestAnimationFrameCallback, user_data);
  }

  /**
   * @apidoc
   * @brief Set a callback to synchronize with a given VSync.
   * @param id The request instance id.
   * @param callback A C-function callback. The callback is called when the next
   * VSync signal arrives.
   * @param user_data The pass-through context.
   */
  inline void RequestAnimationFrame(uintptr_t id,
                                    vsync_observer_callback callback,
                                    void* user_data) {
    if (!c_observer_) return;
    lynx_vsync_observer_request_animation_frame(c_observer_, id, callback,
                                                user_data);
  }

  /**
   * @apidoc
   * @brief Set a callback to synchronize with a given VSync.
   * @param id The request instance id.
   * @param callback A functional callback. It is called when the next VSync
   * signal arrives. It will be called before any normal callback.
   */
  void RequestBeforeAnimationFrame(uintptr_t id,
                                   VSyncObserverCallback callback) {
    if (!c_observer_) return;
    auto* user_data =
        new std::function<void(int64_t, int64_t)>(std::move(callback));
    lynx_vsync_observer_request_before_animation_frame(
        c_observer_, id, &RequestAnimationFrameCallback, user_data);
  }

  /**
   * @apidoc
   * @brief Set a callback to synchronize with a given VSync.
   * @param id The request instance id.
   * @param callback A C-function callback. It is called when the next VSync
   * signal arrives. It will be called before any normal callback.
   * @param user_data The pass-through context.
   */
  inline void RequestBeforeAnimationFrame(uintptr_t id,
                                          vsync_observer_callback callback,
                                          void* user_data) {
    if (!c_observer_) return;
    lynx_vsync_observer_request_before_animation_frame(c_observer_, id,
                                                       callback, user_data);
  }

  /**
   * @apidoc
   * @brief Set a callback to synchronize with a given VSync. It should be
   * called in the BTS thread.
   * @param callback A functional callback. It is called when the next VSync
   * signal arrives. It will be called after each VSync occurs.
   */
  void RegisterAfterAnimationFrameListener(VSyncObserverCallback callback) {
    if (!c_observer_) return;
    after_animation_frame_callbacks_.emplace_back(std::move(callback));
    if (has_registered_) return;
    lynx_vsync_observer_register_after_animation_frame_listener(
        c_observer_, &AfterAnimationFrameListenerCallback, this);
    has_registered_ = true;
  }

  /**
   * @apidoc
   * @brief Set a callback to synchronize with a given VSync.
   * @param callback A C-function callback. It is called when the next VSync
   * signal arrives. It will be called after each VSync occurs.
   * @param user_data The pass-through context.
   */
  inline void RegisterAfterAnimationFrameListener(
      vsync_observer_callback callback, void* user_data) {
    if (!c_observer_) return;
    lynx_vsync_observer_register_after_animation_frame_listener(
        c_observer_, callback, user_data);
  }

 private:
  static void RequestAnimationFrameCallback(void* user_data,
                                            int64_t frame_start_time,
                                            int64_t frame_end_time) {
    auto* callback =
        reinterpret_cast<std::function<void(int64_t, int64_t)>*>(user_data);
    if (frame_start_time == 0 || frame_end_time == 0) {
      delete callback;
    } else {
      (*callback)(frame_start_time, frame_end_time);
      delete callback;
    }
  }

  static void AfterAnimationFrameListenerCallback(void* user_data,
                                                  int64_t frame_start_time,
                                                  int64_t frame_end_time) {
    auto* observer = reinterpret_cast<VSyncObserver*>(user_data);
    observer->CallAfterAnimationFrameListenerCallback(frame_start_time,
                                                      frame_end_time);
  }

  void CallAfterAnimationFrameListenerCallback(int64_t frame_start_time,
                                               int64_t frame_end_time) {
    for (auto& cb : after_animation_frame_callbacks_) {
      cb(frame_start_time, frame_end_time);
    }
  }

  lynx_vsync_observer_t* c_observer_;
  std::vector<VSyncObserverCallback> after_animation_frame_callbacks_;
  bool has_registered_ = false;
};

class LynxExtensionModule {
 public:
  LynxExtensionModule() = default;
  virtual ~LynxExtensionModule() = default;

  /**
   * @apidoc
   * @brief Set up lynx_extension_module_t instance.
   * @param c_module Current LynxView instance.
   */
  void SetCModule(lynx_extension_module_t* c_module) {
    c_module_ = c_module;
    BindFunction();
  }

  /**
   * @apidoc
   * @brief Called when LynxView instance is created. It is always called on the
   * UI thread.
   * @param lynx_view Current LynxView instance.
   */
  virtual void OnLynxViewCreate(lynx_view_t* lynx_view) {}
  /**
   * @apidoc
   * @brief Called when LynxView instance will be destroyed. It is always called
   * on the UI thread.
   */
  virtual void OnLynxViewDestroy() {}
  /**
   * @apidoc
   * @brief Called when BTS Runtime instance is created. It is always called
   * on the UI thread.
   */
  virtual void OnRuntimeInit() {}
  /**
   * @apidoc
   * @brief Called when BTS runtime is attached. It is always called on the
   * BTS thread.
   * @param env The Napi environment.
   * @param vsync_observer The VSyncObserver instance. It will be invalid when
   * LynxExtensionModule is Destroyed.
   */
  virtual void OnRuntimeAttach(Napi::Env env,
                               std::unique_ptr<VSyncObserver> vsync_observer) {}
  /**
   * @apidoc
   * @brief Called when BTS Runtime is ready. It is always called on the
   * BTS thread.
   * @param env The Napi environment.
   * @param lynx The lynx object in BTS.
   * @param url The url of the current LynxView.
   */
  virtual void OnRuntimeReady(Napi::Env env, Napi::Value lynx,
                              const char* url) {}
  /**
   * @apidoc
   * @brief Called when BTS Runtime is detached. It is always called on the
   * BTS thread.
   */
  virtual void OnRuntimeDetach() {}
  /**
   * @apidoc
   * @brief Called when Application did enter foreground. It is always called on
   * the UI thread.
   */
  virtual void OnEnterForeground() {}
  /**
   * @apidoc
   * @brief Called when Application did enter background. It is always called on
   * the UI thread.
   */
  virtual void OnEnterBackground() {}
  /**
   * @apidoc
   * @brief Called when LynxExtensionModule instance will be destroyed. It is
   * always called on the BTS thread.
   */
  virtual void Destroy() {}

  /**
   * @apidoc
   * @brief Post task to BTS Thread.
   * @param task A functional task.
   */
  void PostTaskToRuntime(std::function<void()> task) {
    auto* user_data = new std::function<void()>(std::move(task));
    lynx_extension_module_post_task_to_runtime(c_module_, &PostTaskCallback,
                                               user_data);
  }

  /**
   * @apidoc
   * @brief Post task to BTS Thread.
   * @param task A c-function task.
   * @param user_data The pass-through context.
   */
  inline void PostTaskToRuntime(lynx_extension_module_post_task_func func,
                                void* user_data) {
    lynx_extension_module_post_task_to_runtime(c_module_, func, user_data);
  }

  /**
   * @apidoc
   * @brief Is running tasks on current thread.
   */
  inline bool IsRunningTasksOnBTSThread() {
    return lynx_extension_module_is_running_on_bts_thread(c_module_);
  }

  /**
   * @apidoc
   * @brief Set a napi_module creator to bind custom native methods.
   *
   * The module type T is automatically deduced from the creator function's
   * fourth argument.
   *
   * @param creator The creator function/functor with signature:
   *                Napi::Value(Napi::Env, Napi::Value, const char*, T&)
   *                - env: The Napi environment
   *                - exports: The exports object to populate
   *                - module_name: The name of the module
   *                - module: The LynxExtensionModule instance (of type T)
   * @return The modified exports object
   */
  template <typename Creator>
  inline void SetNapiModuleCreator(Creator&& creator) {
    using ModuleType = typename std::remove_reference<
        typename ArgumentTypeDeducer<std::decay_t<Creator>>::ModuleType>::type;
    static_assert(std::is_base_of<LynxExtensionModule, ModuleType>::value,
                  "Module type must inherit from LynxExtensionModule");

    napi_module_creator_ =
        [creator_holder = std::forward<Creator>(creator)](
            Napi::Env env, Napi::Value exports, const char* module_name,
            LynxExtensionModule& module) mutable -> Napi::Value {
      return creator_holder(env, exports, module_name,
                            static_cast<ModuleType&>(module));
    };
    lynx_extension_module_set_napi_module_creator(c_module_,
                                                  &NapiModuleCreatorCallback);
  }

  /**
   * @apidoc
   * @brief Stores a data with key to the global map with C++ finalizer.
   * @param env The Napi environment.
   * @param key The key to identify the data.
   * @param data The data to store.
   * @param finalize_cb The C++ finalizer callback. It must be a callable object
   *                    (e.g., function pointer, lambda, functor) with the
   *                    signature: void(Napi::Env, void* data, void* hint).
   * @param finalize_hint The hint for the finalizer.
   */
  template <typename Finalizer>
  inline static void SetNapiInstanceData(Napi::Env env, uint64_t key,
                                         void* data, Finalizer finalize_cb,
                                         void* finalize_hint) {
    auto* wrapper =
        new FinalizerWrapper<Finalizer>{std::move(finalize_cb), finalize_hint};
    lynx_napi_set_instance_data(static_cast<napi_env>(env), key, data,
                                &FinalizerWrapper<Finalizer>::Finalize,
                                wrapper);
  }

  /**
   * @apidoc
   * @brief Gets the data with key from the global map.
   * @param env The Napi environment.
   * @param key The key to identify the data.
   * @param data The pointer to receive the data.
   */
  inline static void GetNapiInstanceData(Napi::Env env, uint64_t key,
                                         void** data) {
    lynx_napi_get_instance_data(static_cast<napi_env>(env), key, data);
  }

  inline void Retain() { lynx_extension_module_ref(c_module_); }
  inline void Release() { lynx_extension_module_unref(c_module_); }

 protected:
  using NapiModuleCreator = std::function<Napi::Value(
      Napi::Env, Napi::Value, const char*, LynxExtensionModule&)>;

  lynx_extension_module_t* c_module_ = nullptr;
  NapiModuleCreator napi_module_creator_ = nullptr;

 private:
  template <typename T>
  struct ArgumentTypeDeducer;

  // Specialization for function pointers
  template <typename Ret, typename Arg1, typename Arg2, typename Arg3,
            typename Arg4>
  struct ArgumentTypeDeducer<Ret (*)(Arg1, Arg2, Arg3, Arg4)> {
    using ModuleType = Arg4;
  };

  // Specialization for functors/lambdas (via operator())
  template <typename T>
  struct ArgumentTypeDeducer : ArgumentTypeDeducer<decltype(&T::operator())> {};

  // Specialization for const member functions (lambdas)
  template <typename C, typename Ret, typename Arg1, typename Arg2,
            typename Arg3, typename Arg4>
  struct ArgumentTypeDeducer<Ret (C::*)(Arg1, Arg2, Arg3, Arg4) const> {
    using ModuleType = Arg4;
  };

  // Specialization for mutable member functions (mutable lambdas)
  template <typename C, typename Ret, typename Arg1, typename Arg2,
            typename Arg3, typename Arg4>
  struct ArgumentTypeDeducer<Ret (C::*)(Arg1, Arg2, Arg3, Arg4)> {
    using ModuleType = Arg4;
  };

  template <typename Finalizer>
  struct FinalizerWrapper {
    Finalizer finalizer;
    void* hint;

    FinalizerWrapper(Finalizer fin, void* h)
        : finalizer(std::move(fin)), hint(h) {}

    static void Finalize(napi_env env, void* finalize_data,
                         void* finalize_hint) {
      auto* wrapper = static_cast<FinalizerWrapper*>(finalize_hint);
      wrapper->finalizer(Napi::Env(env), finalize_data, wrapper->hint);
      delete wrapper;
    }
  };

  static napi_value NapiModuleCreatorCallback(napi_env env, napi_value exports,
                                              const char* module_name,
                                              void* opaque) {
    auto* extension_module = reinterpret_cast<LynxExtensionModule*>(opaque);
    if (extension_module->napi_module_creator_) {
      return extension_module->napi_module_creator_(
          Napi::Env(env), Napi::Value(env, exports), module_name,
          *extension_module);
    }
    return exports;
  }

  void BindFunction() {
    // on lynx_view create
    lynx_extension_module_bind_lynx_view_create(
        c_module_,
        [](lynx_extension_module_t* c_module, lynx_view_t* lynx_view) {
          auto* user_data = lynx_extension_module_get_user_data(c_module);
          if (!user_data) {
            return;
          }
          auto* extension_module =
              reinterpret_cast<LynxExtensionModule*>(user_data);
          extension_module->OnLynxViewCreate(lynx_view);
        });
    // on lynx_view destroy
    lynx_extension_module_bind_lynx_view_destroy(
        c_module_, [](lynx_extension_module_t* c_module) {
          auto* user_data = lynx_extension_module_get_user_data(c_module);
          if (!user_data) {
            return;
          }
          auto* extension_module =
              reinterpret_cast<LynxExtensionModule*>(user_data);
          extension_module->OnLynxViewDestroy();
        });
    // on runtime init
    lynx_extension_module_bind_runtime_init(
        c_module_, [](lynx_extension_module_t* c_module) {
          auto* user_data = lynx_extension_module_get_user_data(c_module);
          if (!user_data) {
            return;
          }
          auto* extension_module =
              reinterpret_cast<LynxExtensionModule*>(user_data);
          extension_module->OnRuntimeInit();
        });
    // on runtime attach
    lynx_extension_module_bind_runtime_attach(
        c_module_, [](lynx_extension_module_t* c_module, napi_env env,
                      lynx_vsync_observer_t* observer) {
          auto* user_data = lynx_extension_module_get_user_data(c_module);
          if (!user_data) {
            return;
          }
          auto* extension_module =
              reinterpret_cast<LynxExtensionModule*>(user_data);
          extension_module->OnRuntimeAttach(
              env, std::make_unique<VSyncObserver>(observer));
        });
    // on runtime ready
    lynx_extension_module_bind_runtime_ready(
        c_module_, [](lynx_extension_module_t* c_module, napi_env env,
                      napi_value lynx, const char* url) {
          auto* user_data = lynx_extension_module_get_user_data(c_module);
          if (!user_data) {
            return;
          }
          auto* extension_module =
              reinterpret_cast<LynxExtensionModule*>(user_data);
          extension_module->OnRuntimeReady(env, Napi::Value(env, lynx), url);
        });
    // on runtime detach
    lynx_extension_module_bind_runtime_detach(
        c_module_, [](lynx_extension_module_t* c_module) {
          auto* user_data = lynx_extension_module_get_user_data(c_module);
          if (!user_data) {
            return;
          }
          auto* extension_module =
              reinterpret_cast<LynxExtensionModule*>(user_data);
          extension_module->OnRuntimeDetach();
        });
    // on enter foreground
    lynx_extension_module_bind_enter_foreground(
        c_module_, [](lynx_extension_module_t* c_module) {
          auto* user_data = lynx_extension_module_get_user_data(c_module);
          if (!user_data) {
            return;
          }
          auto* extension_module =
              reinterpret_cast<LynxExtensionModule*>(user_data);
          extension_module->OnEnterForeground();
        });
    // on enter background
    lynx_extension_module_bind_enter_background(
        c_module_, [](lynx_extension_module_t* c_module) {
          auto* user_data = lynx_extension_module_get_user_data(c_module);
          if (!user_data) {
            return;
          }
          auto* extension_module =
              reinterpret_cast<LynxExtensionModule*>(user_data);
          extension_module->OnEnterBackground();
        });
    // destroy
    lynx_extension_module_bind_on_destroy(
        c_module_, [](lynx_extension_module_t* c_module) {
          auto* user_data = lynx_extension_module_get_user_data(c_module);
          if (!user_data) {
            return;
          }
          auto* extension_module =
              reinterpret_cast<LynxExtensionModule*>(user_data);
          extension_module->Destroy();
        });
  }

  static void PostTaskCallback(void* user_data) {
    auto* task = reinterpret_cast<std::function<void()>*>(user_data);
    (*task)();
    delete task;
  }
};

}  // namespace pub
}  // namespace lynx

#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_undefs.h"
#endif

#endif  // PLATFORM_EMBEDDER_PUBLIC_LYNX_EXTENSION_MODULE_H_
