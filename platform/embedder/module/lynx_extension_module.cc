// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/closure.h"
#include "platform/embedder/module/lynx_extension_module_priv.h"
#include "platform/embedder/module/lynx_native_module_napi.h"
#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

LYNX_EXTERN_C void lynx_vsync_observer_request_before_animation_frame(
    lynx_vsync_observer_t* observer, uintptr_t id,
    vsync_observer_callback callback, void* user_data) {
  auto* vsync_observer = observer->extension_module->VSyncObserver();
  if (!vsync_observer) {
    callback(user_data, 0, 0);
    return;
  }
  vsync_observer->RequestBeforeAnimationFrame(
      id, [callback = callback, user_data = user_data](
              int64_t start, int64_t end) { callback(user_data, start, end); });
}

LYNX_EXTERN_C void lynx_vsync_observer_request_animation_frame(
    lynx_vsync_observer_t* observer, uintptr_t id,
    vsync_observer_callback callback, void* user_data) {
  auto* vsync_observer = observer->extension_module->VSyncObserver();
  if (!vsync_observer) {
    callback(user_data, 0, 0);
    return;
  }
  vsync_observer->RequestAnimationFrame(
      id, [callback = callback, user_data = user_data](
              int64_t start, int64_t end) { callback(user_data, start, end); });
}

LYNX_EXTERN_C void lynx_vsync_observer_register_after_animation_frame_listener(
    lynx_vsync_observer_t* observer, vsync_observer_callback callback,
    void* user_data) {
  auto* vsync_observer = observer->extension_module->VSyncObserver();
  if (!vsync_observer) {
    callback(user_data, 0, 0);
    return;
  }
  vsync_observer->RegisterAfterAnimationFrameListener(
      [callback = callback, user_data = user_data](int64_t start, int64_t end) {
        callback(user_data, start, end);
      });
}

// lynx_extension_module_t
LYNX_EXTERN_C lynx_extension_module_t* lynx_extension_module_create(
    void* user_data) {
  return lynx_extension_module_create_with_finalizer(user_data, nullptr);
}

LYNX_EXTERN_C lynx_extension_module_t*
lynx_extension_module_create_with_finalizer(
    void* user_data, void (*finalizer)(lynx_extension_module_t*, void*)) {
  lynx_extension_module_t* module = new lynx_extension_module_t;
  module->user_data = user_data;
  module->finalizer = finalizer;
  lynx_extension_module_ref(module);
  return module;
}

LYNX_EXTERN_C void* lynx_extension_module_get_user_data(
    lynx_extension_module_t* module) {
  return module->user_data;
}

LYNX_EXTERN_C void lynx_extension_module_bind_lynx_view_create(
    lynx_extension_module_t* module,
    lynx_extension_module_on_lynx_view_create_func func) {
  module->on_lynx_view_create_func = func;
}

LYNX_EXTERN_C void lynx_extension_module_bind_lynx_view_destroy(
    lynx_extension_module_t* module,
    lynx_extension_module_on_lynx_view_destroy_func func) {
  module->on_lynx_view_destroy_func = func;
}

LYNX_EXTERN_C void lynx_extension_module_bind_runtime_init(
    lynx_extension_module_t* module,
    lynx_extension_module_on_runtime_init_func func) {
  module->on_runtime_init_func = func;
}

LYNX_EXTERN_C void lynx_extension_module_bind_runtime_attach(
    lynx_extension_module_t* module,
    lynx_extension_module_on_runtime_attach_func func) {
  module->on_runtime_attach_func = func;
}

LYNX_CAPI_EXPORT void lynx_extension_module_bind_runtime_ready(
    lynx_extension_module_t* module,
    lynx_extension_module_on_runtime_ready_func func) {
  module->on_runtime_ready_func = func;
}

LYNX_EXTERN_C void lynx_extension_module_bind_runtime_detach(
    lynx_extension_module_t* module,
    lynx_extension_module_on_runtime_detach_func func) {
  module->on_runtime_detach_func = func;
}

LYNX_EXTERN_C void lynx_extension_module_bind_enter_foreground(
    lynx_extension_module_t* module,
    lynx_extension_module_on_enter_foreground_func func) {
  module->on_enter_foreground_func = func;
}

LYNX_EXTERN_C void lynx_extension_module_bind_enter_background(
    lynx_extension_module_t* module,
    lynx_extension_module_on_enter_background_func func) {
  module->on_enter_background_func = func;
}

LYNX_EXTERN_C void lynx_extension_module_bind_on_destroy(
    lynx_extension_module_t* module,
    lynx_extension_module_on_destroy_func func) {
  module->on_destroy_func = func;
}

LYNX_EXTERN_C void lynx_extension_module_set_napi_module_creator(
    lynx_extension_module_t* module, napi_module_creator creator) {
  module->napi_creator = creator;
}

LYNX_EXTERN_C void lynx_extension_module_post_task_to_runtime(
    lynx_extension_module_t* module, lynx_extension_module_post_task_func func,
    void* user_data) {
  if (module->task_runner) {
    module->task_runner->PostTask(
        [func = func, user_data = user_data]() { func(user_data); });
  }
}

LYNX_EXTERN_C bool lynx_extension_module_is_running_on_bts_thread(
    lynx_extension_module_t* module) {
  if (module->task_runner) {
    return module->task_runner->RunsTasksOnCurrentThread();
  }
  return false;
}

LYNX_CAPI_EXPORT void lynx_extension_module_ref(
    lynx_extension_module_t* module) {
  module->ref_count.fetch_add(1);
  DCHECK(module->ref_count.load() > 0);
}

LYNX_CAPI_EXPORT void lynx_extension_module_unref(
    lynx_extension_module_t* module) {
  DCHECK(module->ref_count.load() > 0);
  if (module->ref_count.fetch_sub(1) == 1) {
    if (module->finalizer) {
      module->finalizer(module, module->user_data);
    }
    if (module->vsync_observer) {
      delete module->vsync_observer;
    }
    delete module;
  }
}

namespace lynx {
namespace embedder {

ExtensionModuleImpl::ExtensionModuleImpl(lynx_extension_module_t* module)
    : c_module_(module),
      env_(nullptr),
      vsync_observer_(nullptr),
      napi_module_(nullptr) {
  if (module) {
    module->extension_module = this;
  }
}

ExtensionModuleImpl::~ExtensionModuleImpl() {
  if (c_module_) {
    lynx_extension_module_unref(c_module_);
  }
}

void ExtensionModuleImpl::SetLynxViewCreatedState(
    lynx_view_t* lynx_view, tasm::UIDelegate* ui_delegate) {
  if (!c_module_ || !c_module_->on_lynx_view_create_func) {
    LOGE("ExtensionModuleImpl c_module or on_lynx_view_create_func is nullptr");
    return;
  }
  c_module_->on_lynx_view_create_func(c_module_, lynx_view);
}

void ExtensionModuleImpl::SetLynxViewDestroyedState() {
  if (!c_module_ || !c_module_->on_lynx_view_destroy_func) {
    LOGE(
        "ExtensionModuleImpl c_module or on_lynx_view_destroy_func is "
        "nullptr");
    return;
  }
  c_module_->on_lynx_view_destroy_func(c_module_);
}

void ExtensionModuleImpl::SetRuntimeInitState(
    const fml::RefPtr<fml::TaskRunner>& task_runner) {
  if (!c_module_ || !c_module_->on_runtime_init_func) {
    LOGE(
        "ExtensionModuleImpl c_module or on_runtime_init_func is "
        "nullptr");
    return;
  }
  c_module_->task_runner = task_runner;
  c_module_->on_runtime_init_func(c_module_);
}

void ExtensionModuleImpl::SetRuntimeAttachedState(
    napi_env env,
    const std::shared_ptr<runtime::IVSyncObserver>& vsync_observer) {
  if (!c_module_ || !c_module_->on_runtime_attach_func) {
    LOGE("ExtensionModuleImpl c_module or on_runtime_attach_func is nullptr");
    return;
  }
  env_ = env;
  vsync_observer_ = vsync_observer;
  lynx_vsync_observer_t* observer = new lynx_vsync_observer_t;
  observer->extension_module = this;
  c_module_->vsync_observer = observer;
  c_module_->on_runtime_attach_func(c_module_, env, observer);
}

void ExtensionModuleImpl::SetRuntimeReadyState(napi_env env, napi_value lynx,
                                               const std::string& url) {
  if (!c_module_ || !c_module_->on_runtime_ready_func) {
    LOGE("ExtensionModuleImpl c_module or on_runtime_ready_func is nullptr");
    return;
  }
  c_module_->on_runtime_ready_func(c_module_, env, lynx, url.c_str());
}

void ExtensionModuleImpl::SetRuntimeDetachedState() {
  if (!c_module_ || !c_module_->on_runtime_detach_func) {
    LOGE("ExtensionModuleImpl c_module or on_runtime_detach_func is nullptr");
    return;
  }
  c_module_->on_runtime_detach_func(c_module_);
}

void ExtensionModuleImpl::SetEnteringForegroundState() {
  if (!c_module_ || !c_module_->on_enter_foreground_func) {
    LOGE("ExtensionModuleImpl c_module or on_enter_foreground_func is nullptr");
    return;
  }
  c_module_->on_enter_foreground_func(c_module_);
}

void ExtensionModuleImpl::SetEnteringBackgroundState() {
  if (!c_module_ || !c_module_->on_enter_background_func) {
    LOGE("ExtensionModuleImpl c_module or on_enter_background_func is nullptr");
    return;
  }
  c_module_->on_enter_background_func(c_module_);
}

base::expected<std::unique_ptr<pub::Value>, std::string>
ExtensionModuleImpl::InvokeMethod(const std::string& method_name,
                                  std::unique_ptr<pub::Value> args,
                                  size_t count,
                                  const runtime::CallbackMap& callbacks) {
  if (napi_module_) {
    return napi_module_->InvokeMethod(method_name, std::move(args), count,
                                      callbacks);
  }
  return std::unique_ptr<pub::Value>(nullptr);
}

void ExtensionModuleImpl::SetupNapiModule() {
  if (c_module_->napi_creator && env_ && !napi_module_) {
    auto exports = Napi::Object::New(env_);
    napi_value ret_exports =
        c_module_->napi_creator(env_, exports, "", nullptr);
    napi_module_ = std::make_unique<LynxNativeModuleNAPI>(env_, ret_exports);
    methods_ = napi_module_->AdoptMethods();

  } else {
    LOGE("ExtensionModuleImpl SetupNapiModule failed");
  }
}

void ExtensionModuleImpl::SetDelegate(
    std::weak_ptr<runtime::LynxNativeModule::Delegate> delegate) {
  if (napi_module_) {
    napi_module_->SetDelegate(delegate);
  }
}

void ExtensionModuleImpl::Destroy() {
  if (!c_module_ || !c_module_->on_destroy_func) {
    LOGE("ExtensionModuleImpl c_module or on_destroy_func is nullptr");
    return;
  }
  c_module_->on_destroy_func(c_module_);
}

}  // namespace embedder
}  // namespace lynx
