// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_LIST_LIST_ITEM_VIEW_H_
#define CLAY_UI_COMPONENT_LIST_LIST_ITEM_VIEW_H_

#include "clay/ui/component/component.h"

namespace clay {

// In RL2, the child in a listContainer may not be a `list-item`, and in RL3 the
// child must be a `list-item`, so the current logic of list-item should be
// placed in component.cc.
// TODO(dongjiajian): separate ListItem with Component.
class ListItemView : public WithTypeInfo<ListItemView, Component> {
 public:
  ListItemView(int32_t id, PageView* page_view);

  void OnContentSizeChanged(const FloatRect& old_rect,
                            const FloatRect& new_rect) override;
};
}  // namespace clay

#endif  // CLAY_UI_COMPONENT_LIST_LIST_ITEM_VIEW_H_
