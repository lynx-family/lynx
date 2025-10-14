// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/view.h"

#include <memory>

#include "clay/ui/rendering/render_container.h"

namespace clay {

View::View(int id, PageView* page_view)
    : WithTypeInfo(id, "view", std::make_unique<RenderContainer>(), page_view) {
}

View::~View() = default;

}  // namespace clay
