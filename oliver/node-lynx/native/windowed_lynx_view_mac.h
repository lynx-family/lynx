// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef OLIVER_NODE_LYNX_NATIVE_WINDOWED_LYNX_VIEW_MAC_H_
#define OLIVER_NODE_LYNX_NATIVE_WINDOWED_LYNX_VIEW_MAC_H_

#include <uv.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "platform/embedder/public/capi/lynx_types.h"

namespace lynx {
namespace node {

struct WindowedLynxViewOptions {
  std::string title;
  double width = 0;
  double height = 0;
  double device_pixel_ratio = 1;
  bool resizable = true;
  bool visible = true;
};

struct WindowedKeyEvent {
  lynx_key_event_t event;
  std::string character;
};

class NodeLynxWindowHost {
 public:
  using PointerCallback = std::function<void(const lynx_pointer_event_t&)>;
  using KeyCallback = std::function<void(const WindowedKeyEvent&)>;
  using ResizeCallback =
      std::function<void(double width, double height, double pixel_ratio)>;
  using ClosedCallback = std::function<void()>;

  virtual ~NodeLynxWindowHost() = default;

  virtual bool Show() = 0;
  virtual void Close() = 0;
  virtual bool IsClosed() const = 0;
  virtual bool PresentFrame(const lynx_accelerated_paint_info_t& info) = 0;
  virtual bool PresentPixels(const uint8_t* rgba, int width, int height) = 0;
  virtual void Click(double x, double y) = 0;
  virtual void TypeText(const std::string& text) = 0;
  virtual void PressKey(const std::string& key) = 0;
  virtual void SetTextInputActive(bool active) = 0;
  virtual void UpdateCaretPosition(float x, float y, float width,
                                   float height) = 0;
  virtual void SetMarkedTextRect(float x, float y, float width,
                                 float height) = 0;

  static bool SupportsAcceleratedRenderer();
  static std::unique_ptr<NodeLynxWindowHost> Create(
      uv_loop_t* loop, WindowedLynxViewOptions options,
      PointerCallback pointer_callback, KeyCallback key_callback,
      ResizeCallback resize_callback, ClosedCallback closed_callback);
};

#if !defined(OS_MACOSX) && !defined(__APPLE__)
inline bool NodeLynxWindowHost::SupportsAcceleratedRenderer() { return false; }

inline std::unique_ptr<NodeLynxWindowHost> NodeLynxWindowHost::Create(
    uv_loop_t*, WindowedLynxViewOptions, PointerCallback, KeyCallback,
    ResizeCallback, ClosedCallback) {
  return nullptr;
}
#endif

}  // namespace node
}  // namespace lynx

#endif  // OLIVER_NODE_LYNX_NATIVE_WINDOWED_LYNX_VIEW_MAC_H_
