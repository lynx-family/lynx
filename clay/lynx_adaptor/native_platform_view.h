// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_LYNX_ADAPTOR_NATIVE_PLATFORM_VIEW_H_
#define CLAY_LYNX_ADAPTOR_NATIVE_PLATFORM_VIEW_H_

#include <functional>
#include <memory>
#include <string>
#include <utility>

#include "base/include/value/lynx_value_api.h"
#include "clay/public/clay.h"

struct native_view_motion_event;

namespace lynx::pub {
class LynxValue;
}

namespace clay {

class CLAY_EXPORT NativePlatformView {
 public:
  using Creator = std::function<NativePlatformView*()>;

  // should sync with LynxUIMethodConstants.java in Lynx.
  static constexpr int kSuccess = 0;
  static constexpr int kUnknown = 1;
  static constexpr int kMethodNotFound = 3;
  static constexpr int kInvalidParameter = 4;
  static constexpr int kInvalidStateError = 7;

  virtual ~NativePlatformView();
  virtual bool OnCreate();
  virtual void OnAttach();
  virtual void OnDetach();
  virtual void OnDestroy();
  virtual void OnLayoutChanged(float left, float top, float width, float height,
                               float pixel_ratio);
  virtual void OnPropertiesChanged(lynx_value attrs, lynx_value events);
  virtual void OnMouseClickEvent(int x, int y, int buttons, bool mouse_up);
  virtual void OnMouseMoveEvent(int x, int y, int modifiers, bool mouse_leave);
  virtual void OnMouseWheelEvent(int x, int y, int modifiers, double delta_x,
                                 double delta_y);
  virtual void OnFocusChanged(bool focused, bool is_leaf);
  virtual void OnMethodInvoked(
      const std::string& method, lynx_value attrs,
      std::function<void(int code, lynx_value data)> callback);
  virtual bool SupportScrolling() const { return false; }
  virtual bool NeedSharedImageSink() const { return false; }
  virtual bool HandleMotionEvent(native_view_motion_event* event) {
    return false;
  }

  virtual ClaySharedImageSinkBufferMode buffer_mode() const {
    return kClaySharedImageSinkBufferModeDoubleBuffer;
  }
  virtual ClaySharedImageBackingType backing_type() const {
#if defined(OS_WIN)
    return kClaySharedImageBackingTypeD3DTexture;
#else
    return kClaySharedImageBackingTypeIOSurface;
#endif
  }
  virtual ClaySharedImageBackingPixelFormat pixel_format() const {
    return kClaySharedImageBackingPixelFormatNative8888;
  }

  bool PresentSurface(int width, int height,
                      const ClayTransformation* transform,
                      ClaySharedImageNativeHandle gfx_handle);
  ClaySharedImageNativeHandle AcquireSurface(int width, int height);
  bool SwapBack();
  ClaySharedImageSinkRef GetSharedImageSink();

  void TriggerEvent(const char* name, lynx::pub::LynxValue&& params);
  void TriggerEvent(const char* name, lynx_value params);
  void RequestFocus();
  void MarkDirty();

 private:
  virtual void Release() = 0;

  friend class NativeViewServiceDesktop;
  friend class NativeViewPluginDesktop;
  class NativeView* native_view_ = nullptr;
  ClaySharedImageSinkRef sink_ref_ = nullptr;
};

}  // namespace clay

#endif  // CLAY_LYNX_ADAPTOR_NATIVE_PLATFORM_VIEW_H_
