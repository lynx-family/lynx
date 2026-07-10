// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef LYNX_CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PAINT_IMAGE_DARWIN_H_
#define LYNX_CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PAINT_IMAGE_DARWIN_H_

#include "core/renderer/ui_wrapper/painting/paint_image.h"

@class LynxRendererContext;

namespace lynx::tasm {

class PaintImageDarwin : public PaintImage {
 public:
  PaintImageDarwin(int32_t image_key, LynxRendererContext* context);
  ~PaintImageDarwin() override;

 private:
  __weak LynxRendererContext* context_;
};
}  // namespace lynx::tasm

#endif  // LYNX_CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PAINT_IMAGE_DARWIN_H_
