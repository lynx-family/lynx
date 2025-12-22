// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BACKGROUND_LAYER_MANAGER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BACKGROUND_LAYER_MANAGER_H_

#include <native_drawing/drawing_types.h>

#include <algorithm>
#include <memory>
#include <vector>

#include "base/include/value/array.h"
#include "base/include/value/base_value.h"
#include "core/renderer/starlight/style/css_type.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/background/background_layer.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/utils/platform_length.h"

namespace lynx {
namespace tasm {
namespace harmony {
class LayerManager {
  enum class BackgroundSizeType {
    kAuto = -(1 << 5),
    kCover = -(1 << 5) - 1,
    kContain = -(1 << 5) - 2,
  };

 public:
  LayerManager(const std::weak_ptr<UIBase>& ui_base, bool is_mask)
      : ui_base_(ui_base), is_mask_(is_mask) {}
  struct Rect {
    float left;
    float right;
    float top;
    float bottom;
  };
  std::vector<std::shared_ptr<BackgroundLayer>> image_layer_list_;
  std::vector<PlatformLength> image_position_list_;
  std::vector<PlatformLength> image_size_list_;
  std::vector<starlight::BackgroundRepeatType> image_repeat_list_;
  std::vector<starlight::BackgroundOriginType> image_origin_list_;
  std::vector<starlight::BackgroundClipType> image_clip_list_;

  void Draw(OH_Drawing_Canvas* canvas, const Rect& border_rect,
            const Rect& padding_rect, const Rect& content_rect,
            OH_Drawing_Path* outer_path, OH_Drawing_Path* inner_path,
            bool has_border);
  void Reset();
  bool HasImageLayers();
  void SetLayerImage(const lepus::Value& data);
  void SetLayerPosition(const lepus::Value& data);
  void SetLayerOrigin(const lepus::Value& data);
  void SetLayerRepeat(const lepus::Value& data);
  void SetLayerClip(const lepus::Value& data);
  void SetLayerSize(const lepus::Value& data);
  std::weak_ptr<UIBase> ui_base_;
  starlight::BackgroundClipType GetLayerClip();

 private:
  float scale_density_{0.f};
  bool is_mask_{false};
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BACKGROUND_LAYER_MANAGER_H_
