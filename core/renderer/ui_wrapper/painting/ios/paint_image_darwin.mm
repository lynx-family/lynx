// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/ios/paint_image_darwin.h"

#import <Lynx/LynxRendererContext.h>

namespace lynx::tasm {

PaintImageDarwin::PaintImageDarwin(int32_t image_key, LynxRendererContext* context)
    : PaintImage(image_key), context_(context) {}

PaintImageDarwin::~PaintImageDarwin() {
  if (context_) {
    [context_ destroyImage:image_key_];
  }
}

}  // namespace lynx::tasm
