// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/list/focus_list_adapter.h"

#include <cstdlib>
#include <ctime>
#include <memory>
#include <utility>

#include "clay/fml/logging.h"
#include "clay/ui/common/macros.h"
#include "clay/ui/component/list/base_list_view.h"
#include "clay/ui/component/list/focus_list_item_view_holder.h"
#include "clay/ui/component/list/macros.h"

namespace clay {

FocusListAdapter::FocusListAdapter(BaseListView* list_view)
    : ListAdapter(list_view) {}
FocusListAdapter::~FocusListAdapter() = default;

ListItemViewHolder* FocusListAdapter::OnCreateListItem(TypeId type) {
  auto item = std::make_unique<FocusListItemViewHolder>(
      list_view_->page_view(), focusable_, column_count_, row_count_,
      default_length_);
  items_.emplace_front(std::move(item));
  return items_.front().get();
}

void FocusListAdapter::OnBindListItem(ListItemViewHolder* item,
                                      int prev_position, int position,
                                      bool newly_created) {
  FML_DCHECK(position < count_);
  static_cast<FocusListItemViewHolder*>(item)->OnUpdatePosition();
}

void FocusListAdapter::OnDeleteListItem(ListItemViewHolder* view_holder) {
  auto before = items_.before_begin();
  auto itr = items_.begin();
  for (; itr != items_.end();) {
    if (itr->get() == view_holder) {
      items_.erase_after(before);
      break;
    }
    before = itr;
    ++itr;
  }
}

int FocusListAdapter::GetItemCount() const { return count_; }

}  // namespace clay
