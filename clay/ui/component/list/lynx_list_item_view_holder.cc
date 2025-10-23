// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/list/lynx_list_item_view_holder.h"

#include "clay/ui/component/base_view.h"

namespace clay {

LynxListItemViewHolder::LynxListItemViewHolder(ListAdapter::TypeId type)
    : type_(type) {}

LynxListItemViewHolder::~LynxListItemViewHolder() = default;

MeasureResult LynxListItemViewHolder::Measure(
    const MeasureConstraint& constraint) {
  MeasureResult res;
  if (!GetView()) {
    return res;
  }

  {
    // Setting width/height should not dirty the layout bit.
    BaseView::LayoutIgnoreHelper helper(GetView());

    // The item's size is set by Lynx (through `ViewContext::SetBounds()`), so
    // we don't change the actual size here. Instead, we set the desired size to
    // `layout_size_`, which may be different from the actual size in grid
    // layout for alignment purpose.
    if (constraint.width_mode != MeasureMode::kIndefinite &&
        constraint.width.has_value()) {
      layout_size_.SetWidth(constraint.width.value());
    } else {
      layout_size_.SetWidth(GetView()->Width());
    }

    if (constraint.height_mode != MeasureMode::kIndefinite &&
        constraint.height.has_value()) {
      layout_size_.SetHeight(constraint.height.value());
    } else {
      layout_size_.SetHeight(GetView()->Height());
    }
  }
  // Call layout to clear the dirty bit.
  GetView()->Layout();

  res.width = layout_size_.width();
  res.height = layout_size_.height();
  return res;
}

}  // namespace clay
