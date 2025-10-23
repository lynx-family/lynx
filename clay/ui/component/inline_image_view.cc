// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/inline_image_view.h"

#include <memory>

#include "clay/ui/component/text/layout_context.h"
#include "clay/ui/rendering/render_image.h"

namespace clay {

InlineImageView::InlineImageView(int id, PageView* page_view)
    : WithTypeInfo(id, "inline-image", std::make_unique<RenderImage>(),
                   page_view) {}

}  // namespace clay
