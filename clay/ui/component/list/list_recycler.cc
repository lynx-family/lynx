// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/list/list_recycler.h"

#include <cstdint>
#include <string>

#include "base/include/fml/macros.h"
#include "base/include/fml/time/time_delta.h"
#include "base/include/fml/time/time_point.h"
#include "base/trace/native/trace_event.h"
#include "clay/fml/logging.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/list/base_list_view.h"
#include "clay/ui/component/list/list_item_view_holder.h"
#include "clay/ui/component/list/macros.h"

namespace clay {

ListRecycler::ListRecycler(ListAdapter* adapter) : adapter_(adapter) {
  FML_DCHECK(adapter_ != nullptr);
}

ListRecycler::~ListRecycler() = default;

ListItemViewHolder* ListRecycler::GetItemForPosition(int position) {
  TRACE_EVENT("LIST", "ListRecycler::GetItemForPosition");

  if (position >= adapter_->GetItemCount()) {
    // TODO(liuguoliang): This may happen when prefetching items. We add
    // protection here to avoid crashes for now, but further investigation is
    // required.
    FML_DCHECK(false) << "position out of range (" << position << " vs "
                      << adapter_->GetItemCount() << ")";
    return nullptr;
  }

  const ListAdapter::TypeId type = adapter_->GetItemViewType(position);

  {
    TRACE_EVENT("LIST", "Search attached scrap items");
    // 0) If there is a scrapped item, return it.
    for (auto itr = attached_scrap_items_.begin();
         itr != attached_scrap_items_.end(); ++itr) {
      ListItemViewHolder* view_holder = *itr;
      if (view_holder->GetLayoutPosition() == position &&
          view_holder->GetItemViewType() == type && !view_holder->IsRemoved()) {
        attached_scrap_items_.erase(itr);
        if (view_holder->NeedsUpdate() || !view_holder->IsBound()) {
          // view_holder which need reload rebind data here.
          adapter_->BindListItem(view_holder, position);
          // Unset the update flag.
          view_holder->RemoveFlags(ListItemViewHolder::kFlagUpdate);
        }
        return view_holder;
      }
    }
  }
  ListItemViewHolder* item = nullptr;
  std::vector<ListItemViewHolder*>& cached_list = cached_items_;
  {
    // 1) Find in prefetch cache.
    if (!cached_list.empty()) {
      TRACE_EVENT("LIST", "Search in prefetch cache");
      // find the item with identical pos
      auto finder = [&cached_list, type](int pos) {
        ListItemViewHolder* res = nullptr;
        auto itr = std::find_if(cached_list.begin(), cached_list.end(),
                                [pos, type](ListItemViewHolder* item) {
                                  return item->GetPosition() == pos &&
                                         item->GetItemViewType() == type;
                                });
        if (itr != cached_list.end()) {
          res = *itr;
          cached_list.erase(itr);
          res->RemoveFlags(ListItemViewHolder::kFlagPrefetch);
        }
        return res;
      };
      // Find the one using `position`
      item = finder(position);
      if (item && item->IsBound() && !item->NeedsUpdate()) {
        return item;
      }
    }
  }

  {
    // 2) Check if in recycle pool in old arch.
    if (!adapter_->IsNewArch()) {
      item = old_arch_recycle_helper_.GetItemForPosition(position, type);
    }
  }

  fml::TimePoint start = fml::TimePoint::Now();
  {
    TRACE_EVENT("LIST", "Create new item");
    // 3) Create new item.
    if (item == nullptr) {
      item = adapter_->CreateListItem(type);
    }
  }
  {
    TRACE_EVENT("LIST", "Bind item");
    // Finally, bind item.
    adapter_->BindListItem(item, position);
  }
  adapter_->UpdateBindTime(type, fml::TimePoint::Now() - start);
  list_perf_report_flag_ = true;
  return item;
}

bool ListRecycler::HasItemCached(int position) const {
  for (auto item : cached_items_) {
    if (item->GetPosition() == position) {
      return true;
    }
  }
  return false;
}

void ListRecycler::RecycleItem(ListItemViewHolder* item) {
  auto type = item->GetItemViewType();
  adapter_->OnRecycleItem(item);
  if (item->IsScrapped()) {
    for (auto itr = attached_scrap_items_.begin();
         itr != attached_scrap_items_.end(); ++itr) {
      if (*itr == item) {
        attached_scrap_items_.erase(itr);
        break;
      }
    }
    item->RemoveFlags(ListItemViewHolder::Flag::kFlagScrapped);
  }
  item->ResetOnRecycle();
  if (item->IsPrefetch() && max_limit_ > 0) {
    int current_size = cached_items_.size();
    while (current_size >= max_limit_) {
      auto to_delete = *cached_items_.begin();
      cached_items_.erase(cached_items_.begin());
      to_delete->RemoveFlags(ListItemViewHolder::Flag::kFlagPrefetch);
      to_delete->AddFlags(ListItemViewHolder::Flag::kFlagUpdate);
      // Put it back to recycle pool in new arch.
      adapter_->OnRecycleItem(to_delete);
      --current_size;
    }
    cached_items_.push_back(item);
  } else if (!adapter_->IsNewArch()) {
    old_arch_recycle_helper_.RecycleItem(item, type);
  }
}

void ListRecycler::ScrapItem(ListItemViewHolder* item) {
  item->AddFlags(ListItemViewHolder::kFlagScrapped);
  attached_scrap_items_.push_back(item);
}

void ListRecycler::RecycleScrappedItems(BaseListView* list_view) {
  if (attached_scrap_items_.empty()) {
    return;
  }
  for (ListItemViewHolder* item : attached_scrap_items_) {
    FML_DCHECK(item->HasAnyOfFlags(ListItemViewHolder::kFlagScrapped));
    item->RemoveFlags(ListItemViewHolder::kFlagScrapped);
    list_view->OnRemoveItem(item);
    RecycleItem(item);
  }
  attached_scrap_items_.clear();
}

void ListRecycler::RemoveScrappedItemByView(BaseView* view) {
  for (auto itr = attached_scrap_items_.begin();
       itr != attached_scrap_items_.end(); ++itr) {
    ListItemViewHolder* view_holder = *itr;
    if (view_holder->GetView() == view) {
      LIST_LOG << "Remove the scrapped item with target view:"
               << view_holder->ToString();
      view_holder->SetView(nullptr);
      attached_scrap_items_.erase(itr);
      break;
    }
  }
}

void ListRecycler::OffsetPositionsForInsert(int position_start,
                                            int item_count) {
  FML_DCHECK(attached_scrap_items_.empty());
  for (const auto& item : cached_items_) {
    const int pos = item->GetPosition();
    if (pos != ListItemViewHolder::kNoPosition && pos >= position_start) {
      LIST_LOG << "Insert. offset pos:" << pos << " count:" << item_count;
      item->OffsetPosition(item_count);
    }
  }
  if (!adapter_->IsNewArch()) {
    old_arch_recycle_helper_.OffsetPositionsForInsert(position_start,
                                                      item_count);
  }
}

void ListRecycler::OffsetPositionsForRemove(int position_start,
                                            int item_count) {
  FML_DCHECK(attached_scrap_items_.empty());
  FML_DCHECK(position_start >= 0 && item_count >= 0);
  const int position_end = position_start + item_count;
  for (auto itr = cached_items_.begin(); itr != cached_items_.end();) {
    auto item = *itr;
    const int pos = item->GetPosition();
    if (pos >= position_end) {
      LIST_LOG << "Remove. offset pos:" << pos << " count:" << -item_count;
      item->OffsetPosition(-item_count, true);
    } else if (pos >= position_start) {
      LIST_LOG << "Remove. Reset pos:" << pos;
      item->Reset();
      adapter_->OnRecycleItem(item);
      itr = cached_items_.erase(itr);
      continue;
    }
    itr++;
  }
  if (!adapter_->IsNewArch()) {
    old_arch_recycle_helper_.OffsetPositionsForRemove(position_start,
                                                      item_count);
  }
}

void ListRecycler::MarkViewHoldersChanged(int position_start, int item_count) {
  FML_DCHECK(attached_scrap_items_.empty());
  FML_DCHECK(position_start >= 0 && item_count >= 0);
  const int position_end = position_start + item_count;
  for (auto itr = cached_items_.begin(); itr != cached_items_.end();) {
    auto item = *itr;
    const int pos = item->GetPosition();
    if (pos >= position_start && pos < position_end) {
      LIST_LOG << "Changed. Reset pos:" << pos;
      item->Reset();
      adapter_->OnRecycleItem(item);
      itr = cached_items_.erase(itr);
      continue;
    }
    itr++;
  }

  if (!adapter_->IsNewArch()) {
    old_arch_recycle_helper_.MarkViewHoldersChanged(position_start, item_count);
  }
}

void ListRecycler::OffsetPositionsForMove(int from, int to) {
  FML_DCHECK(attached_scrap_items_.empty());

  int start, end, in_between_offset;
  if (from < to) {
    start = from;
    end = to;
    in_between_offset = -1;
  } else {
    start = to;
    end = from;
    in_between_offset = 1;
  }
  for (const auto& item : cached_items_) {
    const int pos = item->GetPosition();
    if (pos == ListItemViewHolder::kNoPosition) {
      return;
    }
    if (pos < start || pos > end) {
      return;
    }
    if (pos == from) {
      LIST_LOG << "Moved. Offset pos:" << pos << " count:" << to - from;
      item->OffsetPosition(to - from, false);
    } else {
      LIST_LOG << "Moved. Offset pos:" << pos << " count:" << in_between_offset;
      item->OffsetPosition(in_between_offset, false);
    }
  }
  if (!adapter_->IsNewArch()) {
    old_arch_recycle_helper_.OffsetPositionsForMove(from, to, start, end,
                                                    in_between_offset);
  }
}

void ListRecycler::MarkKnownViewsInvalid() {
  for (const auto& item : cached_items_) {
    item->Reset();
    adapter_->OnRecycleItem(item);
  }
  cached_items_.clear();
  if (!adapter_->IsNewArch()) {
    old_arch_recycle_helper_.MarkKnownViewsInvalid();
  }
}

void ListRecycler::Clear() {
  LIST_LOG << __PRETTY_FUNCTION__;
  cached_items_.clear();
  attached_scrap_items_.clear();
  old_arch_recycle_helper_.Clear();
}

ListItemViewHolder* ListRecycler::RecycleHelperForOldArch::GetItemForPosition(
    int position, ListAdapter::TypeId type) {
  std::list<ListItemViewHolder*>& data_list = recycle_pool_for_old_arch_[type];
  ListItemViewHolder* item = nullptr;
  if (!data_list.empty()) {
    // find the item with identical pos
    auto finder = [&data_list, type](int pos) {
      ListItemViewHolder* res = nullptr;
      auto itr = std::find_if(data_list.begin(), data_list.end(),
                              [pos, type](ListItemViewHolder* item) {
                                return item->GetPosition() == pos &&
                                       item->GetItemViewType() == type;
                              });
      if (itr != data_list.end()) {
        res = *itr;
        data_list.erase(itr);
      }
      return res;
    };
    // Step1: Find the one using `position`
    item = finder(position);
    bool item_need_rebind = false;
    // Step2: Find the one using `kNoPosition`
    if (item == nullptr) {
      item = finder(ListItemViewHolder::kNoPosition);
      item_need_rebind |= (item != nullptr);
    }

    // Step3: Just retrieve item with identical

    if (item == nullptr) {
      // only non-dryrun support
      item = data_list.front();
      data_list.pop_front();
      item_need_rebind |= (item != nullptr);
    }

    if (item_need_rebind && item->IsBound()) {
      item->RemoveFlags(ListItemViewHolder::kFlagBound);
    }
  }
  return item;
}

void ListRecycler::RecycleHelperForOldArch::IterateItemsInRecyclePoolForOldArch(
    const std::function<void(ListItemViewHolder*)>& func) {
  for (auto& id_to_list : recycle_pool_for_old_arch_) {
    std::list<ListItemViewHolder*>& cached_list = id_to_list.second;
    for (ListItemViewHolder* item : cached_list) {
      func(item);
    }
  }
}

void ListRecycler::RecycleHelperForOldArch::RecycleItem(
    ListItemViewHolder* item, ListAdapter::TypeId type) {
  if (recycle_pool_for_old_arch_.find(type) ==
      recycle_pool_for_old_arch_.end()) {
    recycle_pool_for_old_arch_[type] = {};
  }
  recycle_pool_for_old_arch_[type].push_front(item);
}

void ListRecycler::RecycleHelperForOldArch::OffsetPositionsForInsert(
    int position_start, int item_count) {
  IterateItemsInRecyclePoolForOldArch(
      [position_start, item_count](ListItemViewHolder* item) {
        const int pos = item->GetPosition();
        if (pos != ListItemViewHolder::kNoPosition && pos >= position_start) {
          LIST_LOG << "Old Arch insert. offset pos:" << pos
                   << " count:" << item_count;
          item->OffsetPosition(item_count);
        }
      });
}

void ListRecycler::RecycleHelperForOldArch::OffsetPositionsForRemove(
    int position_start, int item_count) {
  const int position_end = position_start + item_count;
  IterateItemsInRecyclePoolForOldArch(
      [position_start, item_count, position_end](ListItemViewHolder* item) {
        const int pos = item->GetPosition();
        if (pos == ListItemViewHolder::kNoPosition) {
          return;
        }
        if (pos >= position_end) {
          LIST_LOG << "Old arch remove. offset pos:" << pos
                   << " count:" << -item_count;
          item->OffsetPosition(-item_count, true);
        } else if (pos >= position_start) {
          LIST_LOG << "Old arch remove. Reset pos:" << pos;
          item->Reset();
        }
      });
}

void ListRecycler::RecycleHelperForOldArch::MarkViewHoldersChanged(
    int position_start, int item_count) {
  const int position_end = position_start + item_count;
  IterateItemsInRecyclePoolForOldArch(
      [position_start, position_end](ListItemViewHolder* item) {
        const int pos = item->GetPosition();
        if (pos == ListItemViewHolder::kNoPosition) {
          return;
        }
        if (pos >= position_start && pos < position_end) {
          LIST_LOG << "Old arch changed. Reset pos:" << pos;
          item->Reset();
        }
      });
}

void ListRecycler::RecycleHelperForOldArch::OffsetPositionsForMove(
    int from, int to, int start, int end, int in_between_offset) {
  IterateItemsInRecyclePoolForOldArch(
      [from, to, start, end, in_between_offset](ListItemViewHolder* item) {
        const int pos = item->GetPosition();
        if (pos == ListItemViewHolder::kNoPosition) {
          return;
        }
        if (pos < start || pos > end) {
          return;
        }
        if (pos == from) {
          LIST_LOG << "Old arch moved. Offset pos:" << pos
                   << " count:" << to - from;
          item->OffsetPosition(to - from, false);
        } else {
          LIST_LOG << "Old arch moved. Offset pos:" << pos
                   << " count:" << in_between_offset;
          item->OffsetPosition(in_between_offset, false);
        }
      });
}

void ListRecycler::RecycleHelperForOldArch::MarkKnownViewsInvalid() {
  IterateItemsInRecyclePoolForOldArch(
      [](ListItemViewHolder* item) { item->Reset(); });  // NOLINT
}

void ListRecycler::RecycleHelperForOldArch::Clear() {
  recycle_pool_for_old_arch_.clear();
}

}  // namespace clay
