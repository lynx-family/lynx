// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_LIST_FOCUS_LIST_ITEM_VIEW_HOLDER_H_
#define CLAY_UI_COMPONENT_LIST_FOCUS_LIST_ITEM_VIEW_HOLDER_H_

#include <functional>
#include <memory>
#include <vector>

#include "clay/ui/component/list/list_item_view_holder.h"

namespace clay {

class PageView;
class View;

class FocusListItemViewHolder : public ListItemViewHolder {
 public:
  using IsFocusable = std::function<bool(int position)>;

  explicit FocusListItemViewHolder(PageView* page_view,
                                   const IsFocusable& focusable, int column,
                                   int row, float default_length);
  virtual ~FocusListItemViewHolder();

  BaseView* InitViews();
  void ReleaseViews();

  void OnUpdatePosition();

  MeasureResult Measure(const MeasureConstraint& constraint) override;

 private:
  std::unique_ptr<View> container_view_;
  std::vector<std::unique_ptr<View>> boxes_;
  PageView* page_view_ = nullptr;
  IsFocusable focusable_;
  int column_;
  int row_;
  float default_length_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_LIST_FOCUS_LIST_ITEM_VIEW_HOLDER_H_
