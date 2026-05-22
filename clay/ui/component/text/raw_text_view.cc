// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/text/raw_text_view.h"

#include <memory>
#include <utility>

#include "clay/ui/rendering/render_dummy.h"

namespace clay {

RawTextView::RawTextView(int id, PageView* page_view)
    : WithTypeInfo(id, "raw-text", std::make_unique<RenderDummy>(), page_view) {
}

RawTextView::~RawTextView() = default;

}  // namespace clay
