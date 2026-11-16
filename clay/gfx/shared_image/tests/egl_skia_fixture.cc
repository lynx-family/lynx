// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/egl_image_backing.h"

namespace clay {
fml::RefPtr<SkiaImageRepresentation> EGLImageBacking::CreateSkiaRepresentation(
    GrDirectContext*) {
  return nullptr;
}
}  // namespace clay
