// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_LYNX_UI_RENDERER_CLAY_H_
#define PLATFORM_EMBEDDER_LYNX_UI_RENDERER_CLAY_H_

#include "platform/embedder/lynx_ui_renderer.h"

struct ClayPointerEvent;

namespace lynx::embedder {

// Shared input implementation for Clay backends. Engine and its mock renderer
// depend only on LynxUIRenderer, without referencing this implementation.
class LynxUIRendererClay : public LynxUIRenderer {
 public:
  explicit LynxUIRendererClay(const RendererConfig& config)
      : LynxUIRenderer(config) {}

  void EmulateMouseSyntheticEvent(const char* event_type, float x, float y,
                                  const char* button, float delta_x,
                                  float delta_y, int modifiers,
                                  int click_count) override;

  void EmulateTouchSyntheticEvent(const char* event_type, float x, float y,
                                  const char* button, float delta_x,
                                  float delta_y, int modifiers,
                                  int click_count) override;

 protected:
  virtual void SendPointerEvent(const ClayPointerEvent& event) = 0;

 private:
  void DispatchSyntheticPointerEvent(ClayPointerEvent event);
};

}  // namespace lynx::embedder

#endif  // PLATFORM_EMBEDDER_LYNX_UI_RENDERER_CLAY_H_
