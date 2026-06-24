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

class ImageDrawable;
class ImageData;
class LynxContext;
class UIBase;

class LynxImageManager : public std::enable_shared_from_this<LynxImageManager> {
 public:
  explicit LynxImageManager(LynxContext* context);
  ~LynxImageManager();

  void UpdatePaintInfo(const ImagePaintInfo& paint_info);
  void RequestImage(int32_t sign, std::string src, float width, float height,
                    int32_t event_mask);
  void SetTarget(const std::weak_ptr<UIBase>& target);
  void UpdateBounds(float width, float height, float scale_density);
  void Draw(OH_Drawing_Canvas* canvas);
  void Reset();
  const std::shared_ptr<ImageData>& Image() const { return image_; }

 private:
  void ApplyPaintInfo();
  void ApplyImage();
  void OnImageLoadSuccess(uint64_t request_id, float width, float height);
  void OnImageLoadFailure(uint64_t request_id, int32_t error_code,
                          const std::string& error_message);
  void SendLoadEvent(float width, float height) const;
  void SendErrorEvent(int32_t error_code,
                      const std::string& error_message) const;
  bool IsCurrentRequest(uint64_t request_id) const;

  LynxContext* context_{nullptr};
  int32_t sign_{-1};
  int32_t event_mask_{0};
  std::string src_;
  uint64_t request_id_{0};
  ImagePaintInfo paint_info_;
  std::shared_ptr<ImageData> image_;
  std::weak_ptr<UIBase> target_;
  std::unique_ptr<ImageDrawable> drawable_;
  float width_{0.f};
  float height_{0.f};
  float scale_density_{0.f};
  bool has_bounds_{false};
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_LYNX_IMAGE_MANAGER_H_
