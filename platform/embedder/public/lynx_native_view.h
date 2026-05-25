// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_PUBLIC_LYNX_NATIVE_VIEW_H_
#define PLATFORM_EMBEDDER_PUBLIC_LYNX_NATIVE_VIEW_H_

#include <functional>

#include "capi/lynx_native_view_capi.h"
#include "lynx_value.h"
#include "lynx_view.h"

namespace lynx {
namespace pub {

/**
 * @apidoc
 * @brief LynxNativeView is the base class that to embed platform
 * view and external texture like Android TextureView, within Lynx view tree.
 */
class LynxNativeView {
 public:
  /**
   * Method invocation results
   */
  static constexpr int kSuccess = 0;
  static constexpr int kUnknown = 1;
  static constexpr int kMethodNotFound = 3;
  static constexpr int kInvalidParameter = 4;
  static constexpr int kInvalidStateError = 7;

  LynxNativeView() : native_view_(lynx_native_view_create(this)) {
    lynx_native_view_bind_on_create(
        native_view_, [](lynx_native_view_t* self, void* user_data) -> bool {
          return static_cast<LynxNativeView*>(user_data)->OnCreate();
        });
    lynx_native_view_bind_on_attach(
        native_view_, [](lynx_native_view_t* self, void* user_data) {
          static_cast<LynxNativeView*>(user_data)->OnAttach();
        });
    lynx_native_view_bind_on_detach(
        native_view_, [](lynx_native_view_t* self, void* user_data) {
          static_cast<LynxNativeView*>(user_data)->OnDetach();
        });
    lynx_native_view_bind_on_destroy(
        native_view_, [](lynx_native_view_t* self, void* user_data) {
          static_cast<LynxNativeView*>(user_data)->OnDestroy();
        });
    lynx_native_view_bind_release(
        native_view_, [](lynx_native_view_t* self, void* user_data) {
          static_cast<LynxNativeView*>(user_data)->Release();
        });
    lynx_native_view_bind_on_layout_changed(
        native_view_,
        [](lynx_native_view_t* self, void* user_data, float left, float top,
           float width, float height, float pixel_ratio) {
          static_cast<LynxNativeView*>(user_data)->OnLayoutChanged(
              left, top, width, height, pixel_ratio);
        });
    lynx_native_view_bind_on_properties_changed(
        native_view_, [](lynx_native_view_t* self, void* user_data,
                         lynx_value attrs, lynx_value events) {
          static_cast<LynxNativeView*>(user_data)->OnPropertiesChanged(
              LynxValue(attrs), LynxValue(events));
        });
    lynx_native_view_bind_on_motion_event(
        native_view_, [](lynx_native_view_t* self, void* user_data,
                         native_view_motion_event_t* event) {
          static_cast<LynxNativeView*>(user_data)->OnMotionEvent(event);
        });
    lynx_native_view_bind_on_focus_changed(
        native_view_, [](lynx_native_view_t* self, void* user_data,
                         bool focused, bool is_leaf) {
          static_cast<LynxNativeView*>(user_data)->OnFocusChanged(focused,
                                                                  is_leaf);
        });
    lynx_native_view_bind_on_method_invoked(
        native_view_,
        [](lynx_native_view_t* self, void* user_data, const char* method,
           lynx_value attrs, lynx_native_view_callback callback,
           lynx_native_view_callback_info_t info) {
          static_cast<LynxNativeView*>(user_data)->OnMethodInvoked(
              method, LynxValue(attrs), [=](int code, LynxValue&& data) {
                lynx_value_add_reference(nullptr, data.Value(), nullptr);
                callback(info, code, data.Value());
              });
        });
    lynx_native_view_bind_is_scroll_enabled(
        native_view_, [](lynx_native_view_t* self, void* user_data) -> bool {
          return static_cast<LynxNativeView*>(user_data)->IsScrollEnabled();
          return false;
        });
    lynx_native_view_bind_is_surface_enabled(
        native_view_, [](lynx_native_view_t* self, void* user_data) -> bool {
          return static_cast<LynxNativeView*>(user_data)->IsSurfaceEnabled();
          return false;
        });
    lynx_native_view_bind_surface_buffer_mode(
        native_view_,
        [](lynx_native_view_t* self,
           void* user_data) -> lynx_surface_buffer_mode_t {
          return static_cast<LynxNativeView*>(user_data)->SurfaceBufferMode();
        });
  }
  virtual ~LynxNativeView() {}

  /**
   * @apidoc
   * @brief LynxNativeView lifecycle and callbacks
   */
  virtual bool OnCreate() { return true; }
  virtual void OnAttach() {}
  virtual void OnDetach() {}
  virtual void OnDestroy() {}
  virtual void OnLayoutChanged(float left, float top, float width, float height,
                               float pixel_ratio) {}
  virtual void OnPropertiesChanged(const LynxValue& attrs,
                                   const LynxValue& events) {}
  virtual void OnMotionEvent(native_view_motion_event_t*) {}
  virtual void OnFocusChanged(bool focused, bool is_leaf) {}
  virtual void OnMethodInvoked(const char* method, const LynxValue& attrs,
                               std::function<void(int, LynxValue&&)> callback) {
    callback(kMethodNotFound, LynxValue(LynxValue::kCreateAsNullTag));
  }
  virtual bool IsScrollEnabled() { return false; }
  virtual bool IsSurfaceEnabled() { return false; }
  virtual lynx_surface_buffer_mode_t SurfaceBufferMode() {
    return kDoubleBuffer;
  }
  virtual void Release() { delete this; }

  /**
   * @apidoc
   * @brief Create and present surface with external graphics buffer
   * @param width The width of surface
   * @param height The height of surface
   * @param transform The transformation matrix that will be applied on screen
   * @param handle The resource handle of system graphics buffer
   */
  inline bool PresentSurface(int width, int height, const float* transform,
                             lynx_surface_handle_t* handle) {
    return lynx_native_view_present_surface(native_view_, width, height,
                                            transform, handle);
  }

  /**
   * @apidoc
   * @brief Acquire the latest back buffer
   * @param width The width of surface
   * @param height The height of surface
   * @return Platform native graphics handle. On Darwin this is IOSurfaceRef.
   *         On Windows this is a D3D shared HANDLE for the back buffer,
   *         not a texture/device pointer. Import it with the graphics
   * API/device that matches the active Windows backend.
   */
  inline lynx_surface_handle_t* AcquireSurface(int width, int height) {
    return lynx_native_view_acquire_surface(native_view_, width, height);
  }

  /**
   * @apidoc
   * @brief Swap the current back buffer to pending front
   */
  inline bool SwapBack() { return lynx_native_view_swap_back(native_view_); }

  /**
   * @apidoc
   * @brief Trigger customized JS event to Lynx element
   * @param name The name of event
   * @param params The arguments be passed to JS handlers
   */
  inline void TriggerEvent(const char* name, LynxValue&& params) {
    lynx_value_add_reference(nullptr, params.Value(), nullptr);
    lynx_native_view_trigger_event(native_view_, name, params.Value());
  }

  /**
   * @apidoc
   * @brief Request focus if Lynx element need to capture input method
   */
  inline void RequestFocus() { lynx_native_view_request_focus(native_view_); }

  /**
   * @apidoc
   * @brief Mark dirty to request redrawing
   */
  inline void MarkDirty() { lynx_native_view_mark_dirty(native_view_); }

  inline lynx_native_view_t* native_view() { return native_view_; }

  LynxNativeView(const LynxNativeView&) = delete;
  LynxNativeView& operator=(const LynxNativeView&) = delete;

 private:
  lynx_native_view_t* native_view_;
};

}  // namespace pub
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_PUBLIC_LYNX_NATIVE_VIEW_H_
