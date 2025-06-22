// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BACKGROUND_BACKGROUND_IMAGE_LAYER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BACKGROUND_BACKGROUND_IMAGE_LAYER_H_

#include <multimedia/image_framework/image/image_source_native.h>

#include <memory>
#include <string>
#include <utility>

#include "base/include/value/base_value.h"
#include "core/public/lynx_resource_loader.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/background/background_layer.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/lynx_image_helper.h"

namespace lynx {
namespace tasm {
namespace harmony {
class BackgroundImageLayer : public BackgroundLayer {
 public:
  explicit BackgroundImageLayer(const lepus::Value& data,
                                const std::weak_ptr<UIBase>& ui_base)
      : url_(data.ToString()), ui_base_(ui_base){};
  void OnSizeChange(float width, float height, float scale_density) override;
  void Draw(OH_Drawing_Canvas* canvas, OH_Drawing_Path* path) override;
  void DecodeImageData(const std::string& url, bool is_base64);
  void HandleImageData(LynxImageHelper::ImageResponse& response);
  float GetWidth() override;
  float GetHeight() override;
  bool IsReady() override;
  void LoadImage();
  ~BackgroundImageLayer() override;
  void DestroyDrawStruct();

 private:
  std::string url_;
  std::unique_ptr<LynxBaseImage> pixel_map_{nullptr};
  std::weak_ptr<UIBase> ui_base_;
  OH_Drawing_Rect* src_rect_{nullptr};
  OH_Drawing_Rect* dest_rect_{nullptr};
  OH_Drawing_SamplingOptions* sample_{nullptr};
  uint32_t image_width_{0};
  uint32_t image_height_{0};
};
}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_BACKGROUND_BACKGROUND_IMAGE_LAYER_H_
