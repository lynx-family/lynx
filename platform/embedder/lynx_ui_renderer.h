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

// Abstract class for UI Renderer.
class LynxUIRenderer {
 public:
  static std::unique_ptr<LynxUIRenderer> CreateWithBuilder(
      lynx_view_builder_t* builder);

  LynxUIRenderer() = default;
  virtual ~LynxUIRenderer() = default;

  LynxUIRenderer(const LynxUIRenderer&) = delete;
  LynxUIRenderer& operator=(const LynxUIRenderer&) = delete;

  virtual void SetParent(NativeWindow parent) = 0;

  virtual NativeWindow GetNativeWindow() = 0;

  virtual void SetFrame(float x, float y, float width, float height) = 0;

  virtual void OnEnterForeground() = 0;

  virtual void OnEnterBackground() = 0;

  virtual void InjectBubbleEvent(const char* params) {}

  virtual void RegisterNativeView(const char* name,
                                  lynx_native_view_creator creator,
                                  void* opaque) {}

  virtual lynx::tasm::UIDelegate* GetUIDelegate() = 0;
  // TODO: Add more methods.
};
}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_LYNX_UI_RENDERER_H_
