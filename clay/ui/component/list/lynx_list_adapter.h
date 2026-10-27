// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_LIST_LYNX_LIST_ADAPTER_H_
#define CLAY_UI_COMPONENT_LIST_LYNX_LIST_ADAPTER_H_

#include <list>
#include <memory>
#include <string>
#include <unordered_map>

#include "clay/ui/component/list/list_adapter.h"
#include "clay/ui/component/list/lynx_list_data.h"

namespace clay {

class ListView;
class LynxListItemViewHolder;

struct LynxListPayload : ListAdapter::Payload {
  int update_to_position;
  explicit LynxListPayload(int position) : update_to_position(position) {
    is_lynx_payload = true;
  }
};

class LynxListAdapter : public ListAdapter {
 public:
  explicit LynxListAdapter(ListView* list_view);
  virtual ~LynxListAdapter();

  int GetItemCount() const override;
  TypeId GetItemViewType(int position) override;
  std::string GetItemTypeName(int position) const;

  void UpdateData();

  // Similar with UpdateData, but used in Lynx NoDiff
  void UpdateActions(ListNoDiffInfo* info);

  void MarkUpdatesConsumed();

  bool IsItemFullSpan(int position) const override;
  bool IsItemStickyTop(int position) const override;
  bool IsItemStickyBottom(int position) const override;
  bool IsNewArch() const override;
  void PrintViewHolders() override;

 protected:
  // Override ListAdapter
  ListItemViewHolder* OnCreateListItem(TypeId type) override;
  void OnBindListItem(ListItemViewHolder* item, int prev_position, int position,
                      bool newly_created) override;
  void OnBindListItemOnNewArch(ListItemViewHolder* item, int position);
  void OnDeleteListItem(ListItemViewHolder* view_holder) override;
  void OnRecycleItem(ListItemViewHolder* item) override;

 private:
  void UpdateTypeNames();
  ListView* GetListView();
  // Notify `observer_` data update.
  void DispatchDataUpdate();

  void FlushNoDiffActions(ListNoDiffInfo* info);

  // Note(Xietong): Don't get confused with this member and `view_type_names_`
  // in `LynxListData`. `view_type_names_` is only used to avoid copying type
  // name (a string) for each position. The same index may have different type
  // names when the list data is updated. For example:
  // When the first list_data_ is updated, there are two types: header and
  // footer. Then type name at index 1 is "footer".
  // The next time, there are three types: "header", "data" and "footer". Then
  // type name at index 1 becomes "data".
  // We should maintain our own mapping so that the same type name can have the
  // same type id.
  std::unordered_map<std::string, int> type_names_;
  std::unique_ptr<LynxListData> list_data_;
  /*
   * Truth of the source.
   */
  std::list<std::unique_ptr<LynxListItemViewHolder>> view_holders_;

  bool updates_consumed_ = true;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_LIST_LYNX_LIST_ADAPTER_H_
