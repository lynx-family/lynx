// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_LYNX_UI_RENDERER_H_
#define PLATFORM_EMBEDDER_LYNX_UI_RENDERER_H_

#include <memory>

#include "core/public/ui_delegate.h"
#include "platform/embedder/lynx_view_builder_priv.h"

namespace lynx {
namespace embedder {

class LynxViewClients;

// Abstract class for UI Renderer.
class LynxUIRenderer {
 public:
  static std::unique_ptr<LynxUIRenderer> CreateWithBuilder(
      lynx_view_builder_t* builder);

  static std::unique_ptr<LynxUIRenderer> CreateWindowlessUIRenderer(
      lynx_view_builder_t* builder);

  explicit LynxUIRenderer(lynx_view_builder_t* builder)
      : width_(builder->frame.width),
        height_(builder->frame.height),
        pixel_ratio_(builder->screen_size.pixel_ratio) {}
  virtual ~LynxUIRenderer() = default;

  LynxUIRenderer(const LynxUIRenderer&) = delete;
  LynxUIRenderer& operator=(const LynxUIRenderer&) = delete;

  virtual void SetParent(NativeWindow parent) = 0;

  virtual NativeWindow GetNativeWindow() = 0;

  virtual void SetPixelRatio(float pixel_ratio) { pixel_ratio_ = pixel_ratio; }
  virtual void SetFrame(float x, float y, float width, float height) {
    width_ = width;
    height_ = height;
  }

  float GetWidth() const { return width_; }

  float GetHeight() const { return height_; }

  float GetPixelRatio() const { return pixel_ratio_; }

  virtual void Reset() {}

  virtual void OnEnterForeground() = 0;

  virtual void OnEnterBackground() = 0;

  virtual void EmulateMouseEvent(const char* event_name, float x, float y,
                                 float delta_x, float delta_y) {}

  virtual void InjectBubbleEvent(const char* params) {}

  virtual void RegisterNativeView(const char* name,
                                  lynx_native_view_creator creator,
                                  void* opaque) {}

  virtual lynx::tasm::UIDelegate* GetUIDelegate() = 0;

  virtual void RegisterIMEHandler(void* handler, void* opaque) = 0;

  virtual void AddClient(LynxViewClients* client) {}

  // TODO: Add more methods.

 protected:
  float width_ = 0;
  float height_ = 0;
  float pixel_ratio_ = 1;
};
}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_LYNX_UI_RENDERER_H_
