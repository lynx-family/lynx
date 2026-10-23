// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_INTERFACES_NATIVE_VIEW_H_
#define PLATFORM_EMBEDDER_INTERFACES_NATIVE_VIEW_H_

#include <functional>
#include <memory>
#include <string>

#include "core/public/pub_value.h"
#include "platform/embedder/public/capi/lynx_native_view_capi.h"

namespace lynx::embedder {
// Renderer-owned surface and event capabilities. No engine implementation
// symbols are needed by the renderer. Both DSOs must come from the same SDK.
class NativeViewRenderer {
 public:
  virtual ~NativeViewRenderer() = default;
  virtual lynx_surface_handle_t* AcquireSurface(int width, int height) = 0;
  virtual lynx_surface_handle_t* AcquireSurface(int width, int height,
                                                const float* transform) = 0;
  virtual bool PresentSurface(int width, int height, const float* transform,
                              lynx_surface_handle_t* handle) = 0;
  virtual bool SwapBack() = 0;
  virtual void TriggerEvent(const char* name, const pub::Value& params) = 0;
  virtual void RequestFocus() = 0;
  virtual void MarkDirty() = 0;
};
// Engine-owned callback state. A native view owns its renderer peer; the peer
// borrows this object, avoiding a reference cycle. Attribute values are
// borrowed for the call; asynchronous work must copy their contents.
class NativeView {
 public:
  using MethodCallback = std::function<void(int, const pub::Value&)>;
  virtual void ReleaseRef() = 0;
  virtual void SetRenderer(std::shared_ptr<NativeViewRenderer> renderer) = 0;
  virtual bool OnCreate() = 0;
  virtual void OnAttach() = 0;
  virtual void OnDetach() = 0;
  virtual void OnDestroy() = 0;
  virtual void OnLayoutChanged(float left, float top, float width, float height,
                               float pixel_ratio) = 0;
  virtual void OnPropertiesChanged(const pub::Value& attrs,
                                   const pub::Value& events) = 0;
  virtual void OnMethodInvoked(const std::string& method,
                               const pub::Value& attrs,
                               MethodCallback callback) = 0;
  virtual bool HandleMotionEvent(native_view_motion_event_t* event) = 0;
  virtual void OnFocusChanged(bool focused, bool is_leaf) = 0;
  virtual bool SupportScrolling() const = 0;
  virtual bool NeedSharedImageSink() const = 0;
  virtual lynx_surface_buffer_mode_t BufferMode() const = 0;

 protected:
  virtual ~NativeView() = default;
};
}  // namespace lynx::embedder

#endif  // PLATFORM_EMBEDDER_INTERFACES_NATIVE_VIEW_H_
