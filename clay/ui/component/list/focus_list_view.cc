// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/list/focus_list_view.h"

#include <string>
#include <utility>
#include <vector>

#include "clay/ui/component/list/focus_list_adapter.h"
#include "clay/ui/component/list/focus_list_item_view_holder.h"
#include "clay/ui/component/list/list_layout_manager_grid.h"
#include "clay/ui/component/list/list_layout_manager_linear.h"
#include "clay/ui/component/list/list_layout_manager_staggered_grid.h"

namespace clay {

BaseFocusListView::BaseFocusListView(int id, PageView* page_view)
    : BaseListView(id, "focus-list", page_view) {
  SetFocusable(true);
}

BaseFocusListView::~BaseFocusListView() { SetAdapter(nullptr); }

void BaseFocusListView::Init() {
  if (adapter_factory_) {
    focus_list_adapter_ = adapter_factory_(this);
  } else {
    focus_list_adapter_ = std::make_unique<FocusListAdapter>(this);
  }
  focus_list_adapter_->SetCount(count_);
  focus_list_adapter_->SetFocusable(focusable_rule_);
  focus_list_adapter_->SetColumnRow(column_count_, row_count_);
  focus_list_adapter_->SetItemDefaultDimension(default_length_);
  SetAdapter(focus_list_adapter_.get());
  SetLayoutManager(
      std::make_unique<ListLayoutManagerLinear>(ScrollDirection::kVertical));
}

BaseView* BaseFocusListView::HandleCreateView(ListItemViewHolder* item) {
  auto* a_item = static_cast<FocusListItemViewHolder*>(item);

  BaseView* new_view = a_item->InitViews();

  AddChild(new_view, child_count());
  return new_view;
}

void BaseFocusListView::HandleDestroyView(BaseView* to_destroy,
                                          ListItemViewHolder* item) {
  RemoveChild(to_destroy);
  auto* a_item = static_cast<FocusListItemViewHolder*>(item);

  a_item->ReleaseViews();
}

FocusListView::FocusListView(int id, PageView* page_view)
    : BaseFocusListView(id, page_view) {
  SetCount(7);
  SetFocusableRule([](int position) { return position % 4 == 0; });
  SetColumnRow(3, 3);
  Init();
}

}  // namespace clay
