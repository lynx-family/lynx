// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "clay/ui/component/extend_views.h"

#include "build/build_config.h"
#include "clay/ui/component/view_registry.h"

#if defined(ENABLE_SVG)
#include "clay/ui/component/svg_image_view.h"
#endif

namespace clay {

void keepExtendElements() {}

#if defined(ENABLE_SVG)
REGISTER_CLAY_ELEMENT("svg", SVGImageView, void);
#endif

}  // namespace clay
