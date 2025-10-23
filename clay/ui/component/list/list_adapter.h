// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_LIST_LIST_ADAPTER_H_
#define CLAY_UI_COMPONENT_LIST_LIST_ADAPTER_H_

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "base/include/fml/time/time_delta.h"
#include "base/include/fml/time/time_point.h"

namespace clay {

class BaseListView;
class ListItemViewHolder;

class ListAdapter {
 public:
  using TypeId = int;
  struct Payload {
    bool is_lynx_payload = false;
    Payload() = default;
    virtual ~Payload() = default;
  };
  using Payloads = std::vector<std::unique_ptr<Payload>>;

  class Observer {
   public:
    Observer();
    virtual ~Observer();

    virtual void OnItemMoved(int from_position, int to_position) {}
    virtual void OnItemRangeInserted(int position_start, int item_count) {}
    virtual void OnItemRangeRemoved(int position_start, int item_count) {}
    virtual void OnItemRangeChanged(int position_start, int item_count,
                                    std::unique_ptr<Payload> payload) {}
    virtual void OnChanged() {}
  };

  static constexpr TypeId kDefaultId = -1;

  explicit ListAdapter(BaseListView* list_view);
  virtual ~ListAdapter();

  ListItemViewHolder* CreateListItem(TypeId type);
  void BindListItem(ListItemViewHolder* item, int index);
  // The list item is not recycled and should by deleted.
  void DeleteListItem(ListItemViewHolder* view_holder);

  void UpdateBindTime(TypeId type, fml::TimeDelta duration);
  fml::TimeDelta GetAverageBindTime(TypeId type);

  virtual void OnRecycleItem(ListItemViewHolder* item);

  virtual int GetItemCount() const = 0;
  virtual TypeId GetItemViewType(int position) { return kDefaultId; }

  void SetObserver(Observer* observer) { observer_ = observer; }

  // Called after modifying the data.
  void NotifyItemMoved(int from_position, int to_position);
  void NotifyItemRangeInserted(int position_start, int item_count);
  void NotifyItemRangeRemoved(int position_start, int item_count);
  void NotifyItemRangeChanged(int position_start, int item_count,
                              std::unique_ptr<Payload> payload = nullptr);
  void NotifyDataSetChanged();

  int FindPreviousFullSpan(int position) const;
  int FindNextFullSpan(int position) const;

  virtual bool IsItemFullSpan(int position) const { return false; }
  virtual bool IsItemStickyTop(int position) const { return false; }
  virtual bool IsItemStickyBottom(int position) const { return false; }
  virtual bool IsNewArch() const { return false; }
  virtual void PrintViewHolders() {}

  struct PerformData {
    int64_t average_create_time_ns;
    int64_t average_bind_time_ns;
  };

 protected:
  virtual ListItemViewHolder* OnCreateListItem(TypeId type) = 0;
  virtual void OnBindListItem(ListItemViewHolder* item, int prev_position,
                              int position, bool newly_created) = 0;
  virtual void OnDeleteListItem(ListItemViewHolder* view_holder) = 0;

  BaseListView* list_view_ = nullptr;
  std::unordered_map<ListAdapter::TypeId, PerformData> id_to_perform_data_;

 private:
  // Not owned.
  Observer* observer_ = nullptr;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_LIST_LIST_ADAPTER_H_
