// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_LIST_FOCUS_LIST_ADAPTER_H_
#define CLAY_UI_COMPONENT_LIST_FOCUS_LIST_ADAPTER_H_

#include <forward_list>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "clay/ui/component/list/list_adapter.h"

namespace clay {

class BaseListView;

// List Adapter used to test the focus behaviour of list view.
// Each item will have 9 sub views in the same color. An item may have sub views
// with different colors from other items and there are 4 colors in total. Only
// the red ones are focusable.
class FocusListAdapter : public ListAdapter {
 public:
  using IsFocusable = std::function<bool(int position)>;

  explicit FocusListAdapter(BaseListView* list_view);
  virtual ~FocusListAdapter();

  void SetCount(int count) { count_ = count; }

  void SetFocusable(const IsFocusable& focusable) { focusable_ = focusable; }

  void SetColumnRow(int column, int row) {
    column_count_ = column;
    row_count_ = row;
  }

  void SetItemDefaultDimension(float length) { default_length_ = length; }

  int GetItemCount() const override;

 protected:
  // Override ListAdapter
  ListItemViewHolder* OnCreateListItem(TypeId type) override;
  void OnBindListItem(ListItemViewHolder* item, int prev_position, int position,
                      bool newly_created) override;
  void OnDeleteListItem(ListItemViewHolder* view_holder) override;

 private:
  int count_ = 0;
  std::forward_list<std::unique_ptr<ListItemViewHolder>> items_;
  IsFocusable focusable_;
  int column_count_ = 3;
  int row_count_ = 3;
  float default_length_ = 0.f;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_LIST_FOCUS_LIST_ADAPTER_H_
