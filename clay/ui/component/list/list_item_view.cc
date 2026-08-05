// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/list/list_item_view.h"

#include <memory>

#include "clay/ui/component/component.h"
#ifndef LYNX_ENABLE_CLAY_NATIVE_LIST
#include "clay/ui/component/list/base_list_view.h"
#endif

namespace clay {

ListItemView::ListItemView(int32_t id, PageView* page_view)
    : WithTypeInfo(id, page_view) {
  tag_ = "ListItemView";
}

void ListItemView::OnContentSizeChanged(const FloatRect& old_rect,
                                        const FloatRect& new_rect) {
  Component::OnContentSizeChanged(old_rect, new_rect);
#ifndef LYNX_ENABLE_CLAY_NATIVE_LIST
  for (auto* ancestor = Parent(); ancestor; ancestor = ancestor->Parent()) {
    if (ancestor->Is<BaseListView>()) {
      static_cast<BaseListView*>(ancestor)->OnListItemSizeChanged();
      break;
    }
  }
#endif
}

}  // namespace clay
