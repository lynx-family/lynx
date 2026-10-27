// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_LIST_FOCUS_LIST_VIEW_H_
#define CLAY_UI_COMPONENT_LIST_FOCUS_LIST_VIEW_H_

#include <functional>
#include <memory>

#include "clay/ui/component/list/base_list_view.h"

namespace clay {

class FocusListAdapter;

class BaseFocusListView : public BaseListView {
 public:
  using IsFocusable = std::function<bool(int position)>;
  using AdapterFactory =
      std::function<std::unique_ptr<FocusListAdapter>(BaseFocusListView*)>;

  BaseFocusListView(int id, PageView* page_view);
  ~BaseFocusListView() override;

  void SetCount(int count) { count_ = count; }
  void SetFocusableRule(const IsFocusable& focusable_rule) {
    focusable_rule_ = focusable_rule;
  }

  void SetAdapterFactory(const AdapterFactory& factory) {
    adapter_factory_ = factory;
  }

  void SetColumnRow(int column, int row) {
    column_count_ = column;
    row_count_ = row;
  }

  void SetItemDefaultDimension(float length) { default_length_ = length; }

  void Init();

 protected:
  BaseView* HandleCreateView(ListItemViewHolder* item) override;
  void HandleDestroyView(BaseView* to_destroy,
                         ListItemViewHolder* item) override;

 private:
  FRIEND_TEST(FocusListViewTest, Focus);
  FRIEND_TEST(FocusListViewTest, EventFindFocus);

  int count_ = 0;
  IsFocusable focusable_rule_;
  AdapterFactory adapter_factory_;
  int column_count_ = 3;
  int row_count_ = 3;
  float default_length_ = 300.f;

  std::unique_ptr<FocusListAdapter> focus_list_adapter_;
};

class FocusListView : public BaseFocusListView {
 public:
  FocusListView(int id, PageView* page_view);
  ~FocusListView() override = default;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_LIST_FOCUS_LIST_VIEW_H_
