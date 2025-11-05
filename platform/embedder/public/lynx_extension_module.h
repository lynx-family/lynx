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
  virtual void OnRuntimeAttach(napi_env env,
                               std::unique_ptr<VSyncObserver> vsync_observer) {}
  /**
   * @apidoc
   * @brief Called when BTS Runtime is ready. It is always called on the
   * BTS thread.
   * @param env The Napi environment.
   * @param lynx The lynx object in BTS.
   * @param url The url of the current LynxView.
   */
  virtual void OnRuntimeReady(napi_env env, napi_value lynx, const char* url) {}
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
   * @param creator napi module creator.
   */
  inline void SetNapiModuleCreator(napi_module_creator creator) {
    lynx_extension_module_set_napi_module_creator(c_module_, creator);
  }

  inline void Retain() { lynx_extension_module_ref(c_module_); }
  inline void Release() { lynx_extension_module_unref(c_module_); }

 protected:
  lynx_extension_module_t* c_module_ = nullptr;

 private:
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
          extension_module->OnRuntimeReady(env, lynx, url);
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
#endif  // PLATFORM_EMBEDDER_PUBLIC_LYNX_EXTENSION_MODULE_H_
