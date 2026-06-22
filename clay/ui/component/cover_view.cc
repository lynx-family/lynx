// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/cover_view.h"

#include <memory>

#include "clay/ui/rendering/render_external_view.h"

namespace clay {

CoverView::CoverView(int id, PageView* page_view)
    : WithTypeInfo(id, "cover-view", std::make_unique<RenderExternalView>(),
                   page_view) {}

void CoverView::SetBound(float left, float top, float width, float height) {
  BaseView::SetBound(left, top, width, height);
  static_cast<RenderExternalView*>(render_object_.get())
      ->SetBackingSize(skity::Vec2(width, height));
}

}  // namespace clay
