// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PAINT_IMAGE_ANDROID_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PAINT_IMAGE_ANDROID_H_

#include <memory>
#include <utility>

#include "core/renderer/ui_wrapper/painting/native_painting_context_platform_ref.h"
#include "core/renderer/ui_wrapper/painting/paint_image.h"

namespace lynx::tasm {
class PaintImageAndroid : public PaintImage {
 public:
  PaintImageAndroid(int32_t image_key,
                    std::weak_ptr<NativePaintingCtxPlatformRef> platform_ref)
      : PaintImage(image_key), platform_ref_(std::move(platform_ref)) {}
  ~PaintImageAndroid() override {
    if (auto platform_ref = platform_ref_.lock()) {
      platform_ref->ScheduleDestroyImage(image_key_);
    }
  }

 private:
  std::weak_ptr<NativePaintingCtxPlatformRef> platform_ref_;
};
}  // namespace lynx::tasm
#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_ANDROID_PAINT_IMAGE_ANDROID_H_
