// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>

#include "clay/lynx_adaptor/native_platform_view.h"
#include "platform/embedder/public/capi/lynx_native_view_capi.h"

class LynxNativeViewPrivate : public clay::NativePlatformView {
 public:
  void Retain() { ref_count_.fetch_add(1, std::memory_order_relaxed); }

  void ReleaseRef() {
    if (ref_count_.fetch_sub(1, std::memory_order_acq_rel) != 1) {
      return;
    }
    if (release_fn) {
      auto release = release_fn;
      auto* user_data = user_data_;
      release_fn = nullptr;
      user_data_ = nullptr;
      release(reinterpret_cast<lynx_native_view_t*>(this), user_data);
    }
    delete this;
  }

  bool OnCreate() override {
    if (on_create_fn) {
      return on_create_fn(reinterpret_cast<lynx_native_view_t*>(this),
                          user_data_);
    }
    return true;
  }
  void OnAttach() override {
    if (on_attach_fn) {
      on_attach_fn(reinterpret_cast<lynx_native_view_t*>(this), user_data_);
    }
  }
  void OnDetach() override {
    if (on_detach_fn) {
      on_detach_fn(reinterpret_cast<lynx_native_view_t*>(this), user_data_);
    }
  }
  void OnDestroy() override {
    if (on_destroy_fn) {
      on_destroy_fn(reinterpret_cast<lynx_native_view_t*>(this), user_data_);
    }
  }
  void OnLayoutChanged(float left, float top, float width, float height,
                       float pixel_ratio) override {
    if (on_layout_changed_fn) {
      on_layout_changed_fn(reinterpret_cast<lynx_native_view_t*>(this),
                           user_data_, left, top, width, height, pixel_ratio);
    }
  }
  void OnPropertiesChanged(lynx_value attrs, lynx_value events) override {
    if (on_properties_changed_fn) {
      on_properties_changed_fn(reinterpret_cast<lynx_native_view_t*>(this),
                               user_data_, attrs, events);
    }
  }
  bool HandleMotionEvent(native_view_motion_event_t* event) override {
    if (on_motion_event_fn) {
      on_motion_event_fn(reinterpret_cast<lynx_native_view_t*>(this),
                         user_data_, event);
    }
    return true;
  }
  void OnFocusChanged(bool focused, bool is_leaf) override {
    if (on_focus_changed_fn) {
      on_focus_changed_fn(reinterpret_cast<lynx_native_view_t*>(this),
                          user_data_, focused, is_leaf);
    }
  }
  void OnMethodInvoked(
      const std::string& method, lynx_value attrs,
      std::function<void(int code, lynx_value data)> callback) override {
    if (on_method_invoked_fn) {
      int64_t callback_id = next_callback_id_++;
      callbacks_[callback_id] = callback;
      return on_method_invoked_fn(
          reinterpret_cast<lynx_native_view_t*>(this), user_data_,
          method.c_str(), attrs,
          [](lynx_native_view_callback_info_t info, int code, lynx_value data) {
            auto* self = reinterpret_cast<LynxNativeViewPrivate*>(info.impl);
            auto search = self->callbacks_.find(info.callback_id);
            if (search == self->callbacks_.end()) {
              return;
            }
            search->second(code, data);
            self->callbacks_.erase(search);
          },
          {(void*)this, callback_id});
    }
    callback(kMethodNotFound, {.val_ptr = 0, .type = lynx_value_undefined});
  }
  bool SupportScrolling() const override {
    if (is_scroll_enabled_fn) {
      return is_scroll_enabled_fn(reinterpret_cast<lynx_native_view_t*>(
                                      const_cast<LynxNativeViewPrivate*>(this)),
                                  user_data_);
    }
    return false;
  }
  bool NeedSharedImageSink() const override {
    if (is_surface_enabled_fn) {
      return is_surface_enabled_fn(
          reinterpret_cast<lynx_native_view_t*>(
              const_cast<LynxNativeViewPrivate*>(this)),
          user_data_);
    }
    return false;
  }
  ClaySharedImageSinkBufferMode buffer_mode() const override {
    if (surface_buffer_mode_fn) {
      auto mode =
          surface_buffer_mode_fn(reinterpret_cast<lynx_native_view_t*>(
                                     const_cast<LynxNativeViewPrivate*>(this)),
                                 user_data_);
      return static_cast<ClaySharedImageSinkBufferMode>(mode);
    }
    return kClaySharedImageSinkBufferModeDoubleBuffer;
  }
  void Release() override { ReleaseRef(); }

  void (*release_fn)(lynx_native_view_t*, void*) = nullptr;
  bool (*on_create_fn)(lynx_native_view_t*, void*) = nullptr;
  void (*on_attach_fn)(lynx_native_view_t*, void*) = nullptr;
  void (*on_detach_fn)(lynx_native_view_t*, void*) = nullptr;
  void (*on_destroy_fn)(lynx_native_view_t*, void*) = nullptr;
  void (*on_layout_changed_fn)(lynx_native_view_t*, void*, float left,
                               float top, float width, float height,
                               float pixel_ratio) = nullptr;
  void (*on_properties_changed_fn)(lynx_native_view_t*, void*, lynx_value attrs,
                                   lynx_value events) = nullptr;
  void (*on_motion_event_fn)(lynx_native_view_t*, void*,
                             native_view_motion_event_t*) = nullptr;
  void (*on_focus_changed_fn)(lynx_native_view_t*, void*, bool focused,
                              bool is_leaf) = nullptr;
  void (*on_method_invoked_fn)(lynx_native_view_t*, void*, const char* method,
                               lynx_value attrs,
                               lynx_native_view_callback callback,
                               lynx_native_view_callback_info_t) = nullptr;
  bool (*is_scroll_enabled_fn)(lynx_native_view_t*, void*) = nullptr;
  bool (*is_surface_enabled_fn)(lynx_native_view_t*, void*) = nullptr;
  lynx_surface_buffer_mode_t (*surface_buffer_mode_fn)(lynx_native_view_t*,
                                                       void*) = nullptr;
  void* user_data_ = nullptr;
  std::unordered_map<int64_t, std::function<void(int code, lynx_value data)>>
      callbacks_;
  int64_t next_callback_id_ = 1;
  std::atomic<uint32_t> ref_count_{1};
};

LYNX_EXTERN_C lynx_native_view_t* lynx_native_view_create(void* user_data) {
  auto* view = new LynxNativeViewPrivate;
  view->user_data_ = user_data;
  return reinterpret_cast<lynx_native_view_t*>(view);
}

LYNX_EXTERN_C lynx_native_view_t* lynx_native_view_retain(
    lynx_native_view_t* view) {
  if (view) {
    reinterpret_cast<LynxNativeViewPrivate*>(view)->Retain();
  }
  return view;
}

LYNX_EXTERN_C void lynx_native_view_release(lynx_native_view_t* view) {
  if (view) {
    reinterpret_cast<LynxNativeViewPrivate*>(view)->ReleaseRef();
  }
}

LYNX_EXTERN_C void lynx_native_view_bind_on_create(
    lynx_native_view_t* view, bool (*callback)(lynx_native_view_t*, void*)) {
  reinterpret_cast<LynxNativeViewPrivate*>(view)->on_create_fn = callback;
}
LYNX_EXTERN_C void lynx_native_view_bind_on_attach(
    lynx_native_view_t* view, void (*callback)(lynx_native_view_t*, void*)) {
  reinterpret_cast<LynxNativeViewPrivate*>(view)->on_attach_fn = callback;
}
LYNX_EXTERN_C void lynx_native_view_bind_on_detach(
    lynx_native_view_t* view, void (*callback)(lynx_native_view_t*, void*)) {
  reinterpret_cast<LynxNativeViewPrivate*>(view)->on_detach_fn = callback;
}
LYNX_EXTERN_C void lynx_native_view_bind_on_destroy(
    lynx_native_view_t* view, void (*callback)(lynx_native_view_t*, void*)) {
  reinterpret_cast<LynxNativeViewPrivate*>(view)->on_destroy_fn = callback;
}
LYNX_EXTERN_C void lynx_native_view_bind_release(
    lynx_native_view_t* view, void (*callback)(lynx_native_view_t*, void*)) {
  reinterpret_cast<LynxNativeViewPrivate*>(view)->release_fn = callback;
}
LYNX_EXTERN_C void lynx_native_view_bind_on_layout_changed(
    lynx_native_view_t* view,
    void (*callback)(lynx_native_view_t*, void*, float left, float top,
                     float width, float height, float pixel_ratio)) {
  reinterpret_cast<LynxNativeViewPrivate*>(view)->on_layout_changed_fn =
      callback;
}
LYNX_EXTERN_C void lynx_native_view_bind_on_properties_changed(
    lynx_native_view_t* view,
    void (*callback)(lynx_native_view_t*, void*, lynx_value attrs,
                     lynx_value events)) {
  reinterpret_cast<LynxNativeViewPrivate*>(view)->on_properties_changed_fn =
      callback;
}
LYNX_EXTERN_C void lynx_native_view_bind_on_motion_event(
    lynx_native_view_t* view,
    void (*callback)(lynx_native_view_t*, void*, native_view_motion_event_t*)) {
  reinterpret_cast<LynxNativeViewPrivate*>(view)->on_motion_event_fn = callback;
}
LYNX_EXTERN_C void lynx_native_view_bind_on_focus_changed(
    lynx_native_view_t* view,
    void (*callback)(lynx_native_view_t*, void*, bool focused, bool is_leaf)) {
  reinterpret_cast<LynxNativeViewPrivate*>(view)->on_focus_changed_fn =
      callback;
}
LYNX_EXTERN_C void lynx_native_view_bind_on_method_invoked(
    lynx_native_view_t* view,
    void (*callback)(lynx_native_view_t*, void*, const char* method,
                     lynx_value attrs, lynx_native_view_callback callback,
                     lynx_native_view_callback_info_t)) {
  reinterpret_cast<LynxNativeViewPrivate*>(view)->on_method_invoked_fn =
      callback;
}
LYNX_EXTERN_C void lynx_native_view_bind_is_scroll_enabled(
    lynx_native_view_t* view, bool (*callback)(lynx_native_view_t*, void*)) {
  reinterpret_cast<LynxNativeViewPrivate*>(view)->is_scroll_enabled_fn =
      callback;
}
LYNX_EXTERN_C void lynx_native_view_bind_is_surface_enabled(
    lynx_native_view_t* view, bool (*callback)(lynx_native_view_t*, void*)) {
  reinterpret_cast<LynxNativeViewPrivate*>(view)->is_surface_enabled_fn =
      callback;
}
LYNX_CAPI_EXPORT void lynx_native_view_bind_surface_buffer_mode(
    lynx_native_view_t* view,
    lynx_surface_buffer_mode_t (*callback)(lynx_native_view_t*,
                                           void* user_data)) {
  reinterpret_cast<LynxNativeViewPrivate*>(view)->surface_buffer_mode_fn =
      callback;
}

LYNX_EXTERN_C lynx_surface_buffer_mode_t
lynx_native_view_get_surface_buffer_mode(lynx_native_view_t* view) {
  auto* native_view = reinterpret_cast<LynxNativeViewPrivate*>(view);
  if (native_view->surface_buffer_mode_fn) {
    return native_view->surface_buffer_mode_fn(view, native_view->user_data_);
  }
  return kDoubleBuffer;
}

LYNX_EXTERN_C bool lynx_native_view_present_surface(
    lynx_native_view_t* view, int width, int height, const float* transform,
    lynx_surface_handle_t* handle) {
  static_assert(sizeof(ClayTransformation) == sizeof(float) * 3 * 3);
  auto self = reinterpret_cast<LynxNativeViewPrivate*>(view);
  if (!self->GetSharedImageSink()) {
    return false;
  }
  return self->PresentSurface(
      width, height, reinterpret_cast<const ClayTransformation*>(transform),
      handle);
}
LYNX_EXTERN_C lynx_surface_handle_t* lynx_native_view_acquire_surface(
    lynx_native_view_t* view, int width, int height) {
  auto self = reinterpret_cast<LynxNativeViewPrivate*>(view);
  if (!self->GetSharedImageSink()) {
    return nullptr;
  }
  auto handle = self->AcquireSurface(width, height);
  return reinterpret_cast<lynx_surface_handle_t*>(handle);
}
LYNX_EXTERN_C bool lynx_native_view_swap_back(lynx_native_view_t* view) {
  return reinterpret_cast<LynxNativeViewPrivate*>(view)->SwapBack();
}
LYNX_EXTERN_C void lynx_native_view_trigger_event(lynx_native_view_t* view,
                                                  const char* name,
                                                  lynx_value params) {
  reinterpret_cast<LynxNativeViewPrivate*>(view)->TriggerEvent(name, params);
}
LYNX_EXTERN_C void lynx_native_view_request_focus(lynx_native_view_t* view) {
  reinterpret_cast<LynxNativeViewPrivate*>(view)->RequestFocus();
}
LYNX_EXTERN_C void lynx_native_view_mark_dirty(lynx_native_view_t* view) {
  reinterpret_cast<LynxNativeViewPrivate*>(view)->MarkDirty();
}
