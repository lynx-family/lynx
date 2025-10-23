// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_LIST_LYNX_LIST_ITEM_VIEW_HOLDER_H_
#define CLAY_UI_COMPONENT_LIST_LYNX_LIST_ITEM_VIEW_HOLDER_H_

#include "clay/gfx/geometry/float_size.h"
#include "clay/ui/component/list/list_item_view_holder.h"

namespace clay {

class LynxListItemViewHolder : public ListItemViewHolder {
 public:
  explicit LynxListItemViewHolder(ListAdapter::TypeId type);
  virtual ~LynxListItemViewHolder();

  MeasureResult Measure(const MeasureConstraint& constraint) override;

  ListAdapter::TypeId GetItemViewType() const override { return type_; }

  float GetWidth() const override { return layout_size_.width(); }
  float GetHeight() const override { return layout_size_.height(); }

 private:
  // Hint what size we want item to have, won't really set to item.
  FloatSize layout_size_;

  ListAdapter::TypeId type_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_LIST_LYNX_LIST_ITEM_VIEW_HOLDER_H_
