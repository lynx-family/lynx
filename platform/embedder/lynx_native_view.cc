// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "base/include/no_destructor.h"
#include "core/value_wrapper/value_impl_lepus.h"
#include "platform/embedder/interfaces/native_view.h"
#include "platform/embedder/public/capi/lynx_native_view_capi.h"

namespace {
// Callback tokens are independent of the CAPI object's address. A late or
// duplicate completion must not dereference a released native-view handle.
class NativeMethodCallbacks {
 public:
  using Callback = lynx::embedder::NativeView::MethodCallback;
  struct State {
    std::recursive_mutex mutex;
    std::unordered_map<int64_t, Callback> pending;
  };
  NativeMethodCallbacks() : state_(std::make_shared<State>()) {}
  ~NativeMethodCallbacks() { Cancel(); }
  int64_t Add(Callback callback) {
    std::lock_guard<std::recursive_mutex> state_lock(state_->mutex);
    auto& registry = Registry();
    std::lock_guard<std::mutex> lock(registry.mutex);
    auto id = ++registry.next_id;
    state_->pending.emplace(id, std::move(callback));
    registry.states.emplace(id, state_);
    return id;
  }
  void Cancel() {
    std::unordered_map<int64_t, Callback> cancelled;
    {
      std::lock_guard<std::recursive_mutex> state_lock(state_->mutex);
      auto& registry = Registry();
      std::lock_guard<std::mutex> lock(registry.mutex);
      for (const auto& entry : state_->pending) {
        registry.states.erase(entry.first);
      }
      cancelled.swap(state_->pending);
    }
    // Captured owners may release other native views or complete cancelled
    // methods during destruction. Destroy callbacks after releasing both locks.
  }
  static void Complete(lynx_native_view_callback_info_t info, int code,
                       lynx_value data) {
    // The callback consumes the result, including cancelled/duplicate results.
    lynx::lepus::Value value(nullptr, data);
    lynx_value_remove_reference(nullptr, data, nullptr);
    std::shared_ptr<State> state;
    {
      auto& registry = Registry();
      std::lock_guard<std::mutex> lock(registry.mutex);
      auto found = registry.states.find(info.callback_id);
      if (found == registry.states.end()) return;
      state = found->second.lock();
      registry.states.erase(found);
    }
    if (!state) return;
    std::lock_guard<std::recursive_mutex> lock(state->mutex);
    auto found = state->pending.find(info.callback_id);
    if (found == state->pending.end()) return;
    auto callback = std::move(found->second);
    state->pending.erase(found);
    lynx::pub::ValueImplLepus wrapped(value);
    callback(code, wrapped);
  }

 private:
  struct CallbackRegistry {
    std::mutex mutex;
    int64_t next_id = 0;
    std::unordered_map<int64_t, std::weak_ptr<State>> states;
  };
  static CallbackRegistry& Registry() {
    static lynx::base::NoDestructor<CallbackRegistry> registry;
    return *registry;
  }
  std::shared_ptr<State> state_;
};
}  // namespace

class LynxNativeViewPrivate : public lynx::embedder::NativeView {
 public:
  void Retain() { ref_count_.fetch_add(1, std::memory_order_relaxed); }

  void ReleaseRef() override {
    if (ref_count_.fetch_sub(1, std::memory_order_acq_rel) != 1) {
      return;
    }
    callbacks_.Cancel();
    if (release_fn) {
      auto release = release_fn;
      auto* user_data = user_data_;
      release_fn = nullptr;
      user_data_ = nullptr;
      release(reinterpret_cast<lynx_native_view_t*>(this), user_data);
    }
    delete this;
  }

  void SetRenderer(
      std::shared_ptr<lynx::embedder::NativeViewRenderer> renderer) override {
    renderer_ = std::move(renderer);
  }
  lynx_surface_handle_t* AcquireSurface(int width, int height) {
    return renderer_ ? renderer_->AcquireSurface(width, height) : nullptr;
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
    callbacks_.Cancel();
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
  void OnPropertiesChanged(const lynx::pub::Value& attrs,
                           const lynx::pub::Value& events) override {
    if (on_properties_changed_fn) {
      auto attrs_value = lynx::pub::ValueUtils::ConvertValueToLepusValue(attrs);
      auto events_value =
          lynx::pub::ValueUtils::ConvertValueToLepusValue(events);
      on_properties_changed_fn(reinterpret_cast<lynx_native_view_t*>(this),
                               user_data_, attrs_value.value(),
                               events_value.value());
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
  void OnMethodInvoked(const std::string& method, const lynx::pub::Value& attrs,
                       MethodCallback callback) override {
    if (on_method_invoked_fn) {
      auto attrs_value = lynx::pub::ValueUtils::ConvertValueToLepusValue(attrs);
      int64_t callback_id = callbacks_.Add(std::move(callback));
      return on_method_invoked_fn(
          reinterpret_cast<lynx_native_view_t*>(this), user_data_,
          method.c_str(), attrs_value.value(), NativeMethodCallbacks::Complete,
          {this, callback_id});
    }
    lynx::lepus::Value value(lynx::lepus::Value::kCreateAsUndefinedTag);
    lynx::pub::ValueImplLepus wrapped(value);
    callback(3, wrapped);
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
  lynx_surface_buffer_mode_t BufferMode() const override {
    if (surface_buffer_mode_fn) {
      auto mode =
          surface_buffer_mode_fn(reinterpret_cast<lynx_native_view_t*>(
                                     const_cast<LynxNativeViewPrivate*>(this)),
                                 user_data_);
      return mode;
    }
    return kDoubleBuffer;
  }
  std::shared_ptr<lynx::embedder::NativeViewRenderer> renderer_;

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
  NativeMethodCallbacks callbacks_;
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
LYNX_EXTERN_C void lynx_native_view_bind_surface_buffer_mode(
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
  auto* self = reinterpret_cast<LynxNativeViewPrivate*>(view);
  return self->renderer_ &&
         self->renderer_->PresentSurface(width, height, transform, handle);
}

LYNX_EXTERN_C lynx_surface_handle_t* lynx_native_view_acquire_surface(
    lynx_native_view_t* view, int width, int height) {
  return reinterpret_cast<LynxNativeViewPrivate*>(view)->AcquireSurface(width,
                                                                        height);
}
LYNX_EXTERN_C lynx_surface_handle_t*
lynx_native_view_acquire_surface_with_transform(lynx_native_view_t* view,
                                                int width, int height,
                                                const float* transform) {
  auto* self = reinterpret_cast<LynxNativeViewPrivate*>(view);
  return self->renderer_
             ? self->renderer_->AcquireSurface(width, height, transform)
             : nullptr;
}

LYNX_EXTERN_C bool lynx_native_view_swap_back(lynx_native_view_t* view) {
  auto* self = reinterpret_cast<LynxNativeViewPrivate*>(view);
  return self->renderer_ && self->renderer_->SwapBack();
}
LYNX_EXTERN_C void lynx_native_view_trigger_event(lynx_native_view_t* view,
                                                  const char* name,
                                                  lynx_value params) {
  auto* self = reinterpret_cast<LynxNativeViewPrivate*>(view);
  lynx::lepus::Value value(nullptr, params);
  lynx::pub::ValueImplLepus wrapped(value);
  if (self->renderer_) self->renderer_->TriggerEvent(name, wrapped);
  lynx_value_remove_reference(nullptr, params, nullptr);
}
LYNX_EXTERN_C void lynx_native_view_request_focus(lynx_native_view_t* view) {
  auto* self = reinterpret_cast<LynxNativeViewPrivate*>(view);
  if (self->renderer_) self->renderer_->RequestFocus();
}
LYNX_EXTERN_C void lynx_native_view_mark_dirty(lynx_native_view_t* view) {
  auto* self = reinterpret_cast<LynxNativeViewPrivate*>(view);
  if (self->renderer_) self->renderer_->MarkDirty();
}
