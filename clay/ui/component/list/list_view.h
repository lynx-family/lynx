// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_LIST_LIST_VIEW_H_
#define CLAY_UI_COMPONENT_LIST_LIST_VIEW_H_

#include <memory>
#include <optional>
#include <string>

#include "clay/ui/component/list/base_list_view.h"
#include "clay/ui/component/list/lynx_list_data.h"

namespace clay {

class LynxListAdapter;

class ListView : public BaseListView {
 public:
  ListView(int id, PageView* page_view);
  ListView(int id, int callback_id, PageView* page_view);
  ~ListView() override;

  void AddChild(BaseView* child, int index) override;

  bool CanChildrenEscape() const override { return false; }

  void SetAttribute(const char* attr_c, const clay::Value& value) override;
  void DidUpdateAttributes() override;

  //---------- Called by LynxListAdapter START ----------
  std::unique_ptr<LynxListData> GetListData();
  void UpdateChildPosition(ListItemViewHolder* item, int position);
  BaseView* ObtainChild(ListItemViewHolder* item, int position);
  void RecycleChild(BaseView* child);
  //---------- Called by LynxListAdapter END ------------

 protected:
  BaseView* HandleCreateView(ListItemViewHolder* item) override;
  void HandleDestroyView(BaseView* to_destroy,
                         ListItemViewHolder* item) override {}
  void ProcessAdapterUpdates() override;
  void OnLayoutComplete() override;

 private:
  void InitAdapter();
  int64_t GenerateOperationId();
  bool SetNoDiffInfo(const clay::Value& value);
  std::unique_ptr<LynxListAdapter> lynx_adapter_;
  std::optional<ListItemViewHolder*> waiting_for_child_;
  uint64_t op_counter_ = 0;
  std::unique_ptr<ListNoDiffInfo> no_diff_info_;
  bool is_no_diff_mode_ = false;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_LIST_LIST_VIEW_H_
