// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BACKGROUND_BOX_SHADOW_LAYER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BACKGROUND_BOX_SHADOW_LAYER_H_

#include <native_drawing/drawing_types.h>

#include <memory>
#include <utility>
#include <vector>

#include "core/renderer/starlight/style/css_type.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/border_radius.h"

namespace lynx {
namespace tasm {
namespace harmony {
struct Shadow {
  OH_Drawing_Path* shadow_path{nullptr};
  OH_Drawing_MaskFilter* mask_filter{nullptr};
  uint32_t color{0};
  float offset_x{0.f};
  float offset_y{0.f};
  float blur_radius{0.f};
  float spread_radius{0.f};
  bool is_inset{false};
  starlight::ShadowOption option{starlight::ShadowOption::kNone};
};

class BoxShadowLayer {
 public:
  explicit BoxShadowLayer() = default;
  void DrawInsetLayer(OH_Drawing_Canvas* canvas);
  void DrawOutSetLayer(OH_Drawing_Canvas* canvas);
  void Draw(Shadow* shadow, OH_Drawing_Canvas* canvas);
  void UpdateShadowData(const lepus::Value& data);
  void UpdateShadowDrawer(float left, float top, float width, float height,
                          BorderRadius* radius, float scale_density);
  bool HasShadow();
  void ClearShadowDrawStruct();
  ~BoxShadowLayer();

 private:
  std::vector<std::unique_ptr<Shadow>> shadow_list_{};
  OH_Drawing_Brush* brush_{nullptr};
  OH_Drawing_Filter* filter_{nullptr};
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BACKGROUND_BOX_SHADOW_LAYER_H_
