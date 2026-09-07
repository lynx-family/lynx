// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_LYNX_IMAGE_MANAGER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_LYNX_IMAGE_MANAGER_H_

#include <native_drawing/drawing_canvas.h>

#include <cstdint>
#include <memory>
#include <string>

#include "core/renderer/ui_wrapper/painting/paint_image.h"

namespace lynx {
namespace tasm {
namespace harmony {

class ImageData;
class ImageDrawable;
class LynxContext;
class UIBase;

class LynxImageManager : public std::enable_shared_from_this<LynxImageManager> {
 public:
  explicit LynxImageManager(std::weak_ptr<LynxContext> context);
  ~LynxImageManager();

  void UpdatePaintInfo(const ImagePaintInfo& paint_info);
  void RequestImage(int32_t sign, std::string src, float width, float height,
                    int32_t event_mask);
  void SetTarget(const std::weak_ptr<UIBase>& target);
  void UpdateBounds(float width, float height, float scale_density);
  void Draw(OH_Drawing_Canvas* canvas);

 private:
  void ApplyPaintInfo();
  void ApplyImage();
  void OnImageLoadSuccess(float image_width, float image_height,
                          float view_width, float view_height);
  void AutoSizeIfNeeded(float image_width, float image_height, float view_width,
                        float view_height);
  void OnImageLoadFailure(int32_t error_code, const std::string& error_message);

  std::weak_ptr<LynxContext> context_;
  int32_t sign_{-1};
  int32_t event_mask_{0};
  ImagePaintInfo paint_info_;
  std::shared_ptr<ImageData> image_;
  std::unique_ptr<ImageDrawable> drawable_;
  float width_{0.f};
  float height_{0.f};
  bool has_bounds_{false};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_LYNX_IMAGE_MANAGER_H_
