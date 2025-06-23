// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/background/background_layer.h"

#include <native_drawing/drawing_path.h>

namespace lynx {
namespace tasm {
namespace harmony {

bool BackgroundLayer::IsReady() { return false; }

void BackgroundLayer::OnSizeChange(float width, float height,
                                   float scale_density) {
  width_ = width;
  height_ = height;
}

void BackgroundLayer::Draw(OH_Drawing_Canvas* canvas, OH_Drawing_Path* path) {}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
