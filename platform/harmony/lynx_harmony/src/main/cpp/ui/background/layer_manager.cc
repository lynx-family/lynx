// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/background/layer_manager.h"

#include <native_drawing/drawing_rect.h>

#include <memory>

#include "base/include/float_comparison.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/background/background_image_layer.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/background/background_linear_gradient_layer.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/background/background_none_layer.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/background/background_radial_gradient_layer.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"

namespace lynx {
namespace tasm {
namespace harmony {

void LayerManager::Draw(OH_Drawing_Canvas* canvas, const Rect& border_rect,
                        const Rect& padding_rect, const Rect& content_rect,
                        OH_Drawing_Path* outer_path,
                        OH_Drawing_Path* inner_path, bool has_border) {
  if (image_layer_list_.empty()) {
    return;
  }
  auto ui_base_self = ui_base_.lock();
  if (!ui_base_self) {
    return;
  }
  scale_density_ = ui_base_self->GetContext()->ScaledDensity();
  for (int32_t index = image_layer_list_.size() - 1; index >= 0; --index) {
    if (!image_layer_list_.at(index)->IsReady()) {
      continue;
    }
    auto painting_box = padding_rect;
    // 1.origin
    if (!image_origin_list_.empty()) {
      auto used_index = index % image_origin_list_.size();
      switch (image_origin_list_.at(used_index)) {
        case starlight::BackgroundOriginType::kBorderBox:
          painting_box = border_rect;
          break;
        case starlight::BackgroundOriginType::kPaddingBox:
          painting_box = padding_rect;
          break;
        case starlight::BackgroundOriginType::kContentBox:
          painting_box = content_rect;
          break;
        default:
          break;
      }
    }

    // 2.clip
    auto clip_path = outer_path;
    auto clip_box = border_rect;
    if (!image_clip_list_.empty()) {
      auto used_index = index % image_clip_list_.size();
      switch (image_clip_list_.at(used_index)) {
        case starlight::BackgroundClipType::kBorderBox:
          clip_box = border_rect;
          clip_path = outer_path;
          break;
        case starlight::BackgroundClipType::kPaddingBox:
          clip_box = padding_rect;
          clip_path = inner_path;
          break;
        case starlight::BackgroundClipType::kContentBox:
          clip_box = content_rect;
          clip_path = inner_path;
          break;
        default:
          break;
      }
    }
    float width = 0;
    float height = 0;
    float image_width = 0;
    float image_height = 0;
    auto layer_drawable = image_layer_list_.at(index).get();
    bool is_gradient = layer_drawable->IsGradient();
    if (is_gradient) {
      image_width = painting_box.right - painting_box.left;
      image_height = painting_box.bottom - painting_box.top;
      width = image_width;
      height = image_height;
    } else {
      image_width = layer_drawable->GetWidth();
      image_height = layer_drawable->GetHeight();
      width = scale_density_ * image_width;
      height = scale_density_ * image_height;
    }

    // 3.size
    if (!image_size_list_.empty() && image_size_list_.size() >= 2) {
      PlatformLength size_x;
      PlatformLength size_y;
      if (index * 2 >= image_size_list_.size()) {
        size_x = image_size_list_.at(image_size_list_.size() - 2);
        size_y = image_size_list_.at(image_size_list_.size() - 1);
      } else {
        size_x = image_size_list_.at(index * 2);
        size_y = image_size_list_.at(index * 2 + 1);
      }
      float self_width = painting_box.right - painting_box.left;
      float self_height = painting_box.bottom - painting_box.top;
      float aspect = width / height;
      auto size_x_type = static_cast<BackgroundSizeType>(size_x.AsNumber());
      auto size_y_type = static_cast<BackgroundSizeType>(size_y.AsNumber());
      if (size_x_type == BackgroundSizeType::kCover) {  // apply cover
        width = self_width;
        height = width / aspect;
        if (height < self_height) {
          height = self_height;
          width = aspect * height;
        }
      } else if (size_x_type ==
                 BackgroundSizeType::kContain) {  // apply contain
        width = self_width;
        height = width / aspect;

        if (height > self_height) {
          height = self_height;
          width = aspect * height;
        }
      } else {
        width =
            size_x_type == BackgroundSizeType::kAuto
                ? width
                : size_x.GetValue(self_width / scale_density_) * scale_density_;
        height = size_y_type == BackgroundSizeType::kAuto
                     ? height
                     : size_y.GetValue(self_height / scale_density_) *
                           scale_density_;

        if (size_x_type == BackgroundSizeType::kAuto) {
          if (is_gradient) {
            // gradient has no aspect
            width = painting_box.right - painting_box.left;
          } else {
            width = aspect * height;
          }
        }

        if (size_y_type == BackgroundSizeType::kAuto) {
          if (is_gradient) {
            // gradient has no aspect
            height = painting_box.bottom - painting_box.top;
          } else {
            height = width / aspect;
          }
        }
      }
    }

    if (std::isnan(height) || std::isnan(width) ||
        base::FloatsLarger(1, width) || base::FloatsLargerOrEqual(1, height)) {
      continue;
    }

    // 4.position
    float offset_x = painting_box.left;
    float offset_y = painting_box.top;
    if (image_position_list_.size() >= 2) {
      auto used_pos_index = index % (image_position_list_.size() / 2);

      float delta_width = (painting_box.right - painting_box.left) - width;
      float delta_height = (painting_box.bottom - painting_box.top) - height;

      PlatformLength pos_x = image_position_list_.at(used_pos_index * 2);
      PlatformLength pos_y = image_position_list_.at(used_pos_index * 2 + 1);
      offset_x += pos_x.GetValue(delta_width / scale_density_) * scale_density_;
      offset_y +=
          pos_y.GetValue(delta_height / scale_density_) * scale_density_;
    }

    // 5.repeat
    auto repeat_x_type = starlight::BackgroundRepeatType::kRepeat;
    auto repeat_y_type = starlight::BackgroundRepeatType::kRepeat;

    if (image_repeat_list_.size() >= 2) {
      auto used_repeat_index = index % (image_repeat_list_.size() / 2);
      repeat_x_type = image_repeat_list_.at(used_repeat_index * 2);
      repeat_y_type = image_repeat_list_.at(used_repeat_index * 2 + 1);
    }
    layer_drawable->OnSizeChange(width, height, scale_density_);

    OH_Drawing_Path* draw_path = nullptr;
    if (outer_path && is_gradient && !has_border) {
      draw_path = outer_path;
    }

    OH_Drawing_CanvasSave(canvas);
    //    auto saveCount = OH_Drawing_CanvasGetSaveCount(canvas);
    // need to make sure all background content is clipped inside view's bounds
    // if clipPath is empty, need to clip at view's bounding rect
    if (clip_path && has_border) {
      // if has border&radius, do not use PathEffect to draw gradient, just clip
      OH_Drawing_CanvasClipPath(canvas, clip_path, INTERSECT, true);
    } else {
      auto rect = OH_Drawing_RectCreate(clip_box.left, clip_box.top,
                                        clip_box.right, clip_box.bottom);
      OH_Drawing_CanvasClipRect(canvas, rect, INTERSECT, true);
      OH_Drawing_RectDestroy(rect);
    }

    if (repeat_x_type == starlight::BackgroundRepeatType::kNoRepeat &&
        repeat_y_type == starlight::BackgroundRepeatType::kNoRepeat) {
      // No repeat, paint the drawable directly
      OH_Drawing_CanvasSave(canvas);
      OH_Drawing_CanvasTranslate(canvas, offset_x, offset_y);
      layer_drawable->Draw(canvas, draw_path);
      OH_Drawing_CanvasRestore(canvas);
    } else {
      float end_x = std::max(painting_box.right, clip_box.right);
      float end_y = std::max(painting_box.bottom, clip_box.bottom);
      float start_x = offset_x, start_y = offset_y;

      if (repeat_x_type == starlight::BackgroundRepeatType::kRepeat ||
          repeat_x_type == starlight::BackgroundRepeatType::kRepeatX) {
        start_x = start_x - std::ceil(start_x / width) * width;
      }

      if (repeat_y_type == starlight::BackgroundRepeatType::kRepeat ||
          repeat_y_type == starlight::BackgroundRepeatType::kRepeatY) {
        start_y = start_y - std::ceil(start_y / height) * height;
      }

      OH_Drawing_CanvasSave(canvas);
      auto rect = OH_Drawing_RectCreate(clip_box.left, clip_box.top,
                                        clip_box.right, clip_box.bottom);
      OH_Drawing_CanvasClipRect(canvas, rect, INTERSECT, true);
      OH_Drawing_RectDestroy(rect);
      for (float x = start_x; x < end_x; x += width) {
        for (float y = start_y; y < end_y; y += height) {
          OH_Drawing_CanvasSave(canvas);
          OH_Drawing_CanvasTranslate(canvas, x, y);
          layer_drawable->Draw(canvas, draw_path);
          OH_Drawing_CanvasRestore(canvas);
          if (repeat_y_type == starlight::BackgroundRepeatType::kNoRepeat) {
            break;
          }
        }

        if (repeat_x_type == starlight::BackgroundRepeatType::kNoRepeat) {
          break;
        }
      }
      OH_Drawing_CanvasRestore(canvas);
    }
    OH_Drawing_CanvasRestore(canvas);
  }
}

void LayerManager::Reset() {
  image_layer_list_.clear();
  image_position_list_.clear();
  image_size_list_.clear();
  image_origin_list_.clear();
  image_repeat_list_.clear();
  image_clip_list_.clear();
}

bool LayerManager::HasImageLayers() { return !image_layer_list_.empty(); }

void LayerManager::SetLayerImage(const lepus::Value& data) {
  image_layer_list_.clear();
  if (!data.IsArray()) {
    return;
  }
  auto items = data.Array();
  auto length = items->size();
  for (size_t i = 0; i < length; ++i) {
    auto type =
        static_cast<starlight::BackgroundImageType>(items->get(i).Number());
    i++;
    if (type == starlight::BackgroundImageType::kUrl) {
      auto layer =
          std::make_shared<BackgroundImageLayer>(items->get(i), ui_base_);
      layer->LoadImage();
      image_layer_list_.emplace_back(layer);
    } else if (type == starlight::BackgroundImageType::kLinearGradient) {
      image_layer_list_.emplace_back(
          std::make_unique<BackgroundLinearGradientLayer>(items->get(i)));
    } else if (type == starlight::BackgroundImageType::kRadialGradient) {
      image_layer_list_.emplace_back(
          std::make_unique<BackgroundRadialGradientLayer>(items->get(i)));
    } else if (type == starlight::BackgroundImageType::kNone) {
      image_layer_list_.emplace_back(std::make_unique<BackgroundNoneLayer>());
    }
  }
}

void LayerManager::SetLayerPosition(const lepus::Value& data) {
  image_position_list_.clear();
  if (!data.IsArray()) {
    return;
  }
  auto items = data.Array();
  for (size_t i = 0; i < items->size(); i += 2) {
    auto type = static_cast<PlatformLengthType>(items->get(i + 1).Number());
    image_position_list_.emplace_back(items->get(i), type);
  }
}

void LayerManager::SetLayerOrigin(const lepus::Value& data) {
  image_origin_list_.clear();
  if (!data.IsArray()) {
    return;
  }
  auto items = data.Array();
  for (size_t i = 0; i < items->size(); ++i) {
    image_origin_list_.emplace_back(
        static_cast<starlight::BackgroundOriginType>(items->get(i).Number()));
  }
}

void LayerManager::SetLayerRepeat(const lepus::Value& data) {
  image_repeat_list_.clear();
  if (!data.IsArray()) {
    return;
  }
  auto items = data.Array();
  for (size_t i = 0; i < items->size(); ++i) {
    image_repeat_list_.emplace_back(
        static_cast<starlight::BackgroundRepeatType>(items->get(i).Number()));
  }
}

void LayerManager::SetLayerClip(const lepus::Value& data) {
  image_clip_list_.clear();
  if (!data.IsArray()) {
    return;
  }
  auto items = data.Array();
  for (size_t i = 0; i < items->size(); ++i) {
    image_clip_list_.emplace_back(
        static_cast<starlight::BackgroundClipType>(items->get(i).Number()));
  }
}

starlight::BackgroundClipType LayerManager::GetLayerClip() {
  if (image_clip_list_.empty()) {
    return starlight::BackgroundClipType::kBorderBox;
  }
  return image_clip_list_.back();
}

void LayerManager::SetLayerSize(const lepus::Value& data) {
  image_size_list_.clear();

  if (!data.IsArray()) {
    return;
  }
  auto items = data.Array();
  for (size_t i = 0; i < items->size(); i += 2) {
    auto type = static_cast<PlatformLengthType>(items->get(i + 1).Number());
    image_size_list_.emplace_back(items->get(i), type);
  }
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
