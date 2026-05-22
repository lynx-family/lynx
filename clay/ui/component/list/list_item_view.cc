// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/list/list_item_view.h"

#include <memory>

#include "clay/ui/component/component.h"

namespace clay {

ListItemView::ListItemView(int32_t id, PageView* page_view)
    : WithTypeInfo(id, page_view) {
  tag_ = "ListItemView";
}

}  // namespace clay
