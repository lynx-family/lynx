// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BACKGROUND_BACKGROUND_LAYER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BACKGROUND_BACKGROUND_LAYER_H_

#include <native_drawing/drawing_canvas.h>

#include <memory>

namespace lynx {
namespace tasm {
namespace harmony {
class BackgroundLayer : public std::enable_shared_from_this<BackgroundLayer> {
 public:
  virtual bool IsReady();
  virtual float GetWidth() { return width_; };
  virtual float GetHeight() { return height_; };
  virtual void OnSizeChange(float width, float height, float scale_density);
  virtual void Draw(OH_Drawing_Canvas* canvas, OH_Drawing_Path* path);
  virtual bool IsGradient() { return false; };
  virtual ~BackgroundLayer() = default;

 protected:
  float width_{0};
  float height_{0};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BACKGROUND_BACKGROUND_LAYER_H_
