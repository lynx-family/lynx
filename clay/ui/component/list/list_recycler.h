// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_LIST_LIST_RECYCLER_H_
#define CLAY_UI_COMPONENT_LIST_LIST_RECYCLER_H_

#include <cstdint>
#include <functional>
#include <list>
#include <unordered_map>
#include <vector>

#include "base/include/fml/time/time_point.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/list/list_adapter.h"
#include "third_party/googletest/googletest/include/gtest/gtest_prod.h"  // nogncheck

namespace clay {

class ListItemViewHolder;

// Recycler is responsible for caching the item. Layout Manager does not acquire
// an item from List directly. Instead, a Recycler instance is passed to the
// layout manager.
// Recycler will try to find an item from cache first. If no valid cache exists,
// Recycler will ask Adapter to create one.
class ListRecycler {
 public:
  explicit ListRecycler(ListAdapter* adapter);
  ~ListRecycler();

  /**
   * Get the reused item with `position` and corresponding typeId if existing,
   * otherwise create a new one. The find process consists of following steps.
   * 1. retrieve typeId from `adapter_` using `position`
   * 2. search in 1st layer cache(i.e.`attached_scrap_items_`), return if
   *    existing
   * 3. search in 2nd layer cache(i.e.`cache_items_`), using `position`, then
   *    `kNoPosition`, then just same typeId, return if existing
   * 4. create a new one
   * @param position relative to whole list data
   * @return ListItemViewHolder* corresponding view_holder
   */
  ListItemViewHolder* GetItemForPosition(int position);

  /**
   * Set view hidden, and save into 2nd layer cache.
   *
   * @param item view_holder
   */
  void RecycleItem(ListItemViewHolder* item);

  // When an item is scrapped, it is possible that the item will be reused in
  // the same layout pass then the item does not need to bind again as long
  // as the changed flag is not set.
  void ScrapItem(ListItemViewHolder* item);
  void RecycleScrappedItems(BaseListView* list_view);

  void RemoveScrappedItemByView(BaseView* view);

  void OffsetPositionsForInsert(int position_start, int item_count);
  void OffsetPositionsForRemove(int position_start, int item_count);
  void MarkViewHoldersChanged(int position_start, int item_count);
  void OffsetPositionsForMove(int from, int to);
  void MarkKnownViewsInvalid();

  bool HasItemCached(int position) const;

  // Set the max cached item count.
  void SetCacheMaxLimit(int max_limit) { max_limit_ = max_limit; }
  int GetCacheMaxLimit() const { return max_limit_; }

  void Clear();

  void SetListPerfFlag(bool flag) { list_perf_report_flag_ = flag; }
  bool GetListPerfFlag() const { return list_perf_report_flag_; }

 private:
  FRIEND_TEST(ListLayoutManagerLinear, Scroll);
  FRIEND_TEST(ListLayoutManagerLinear, ScrollHorizontally);
  FRIEND_TEST(ListLayoutManagerLinear, ScrollDiffHeight);
  FRIEND_TEST(ListLayoutManagerLinear, RemoveData);
  FRIEND_TEST(ListLayoutManagerGrid, Scroll);
  FRIEND_TEST(ListLayoutManagerStaggeredGrid, Scroll);

  class RecycleHelperForOldArch {
   public:
    ListItemViewHolder* GetItemForPosition(int position,
                                           ListAdapter::TypeId type);
    void RecycleItem(ListItemViewHolder* item, ListAdapter::TypeId type);
    void OffsetPositionsForInsert(int position_start, int item_count);
    void OffsetPositionsForRemove(int position_start, int item_count);
    void MarkViewHoldersChanged(int position_start, int item_count);
    void OffsetPositionsForMove(int from, int to, int start, int end,
                                int in_between_offset);
    void MarkKnownViewsInvalid();
    void IterateItemsInRecyclePoolForOldArch(
        const std::function<void(ListItemViewHolder*)>& func);
    void Clear();

   private:
    std::unordered_map<ListAdapter::TypeId, std::list<ListItemViewHolder*>>
        recycle_pool_for_old_arch_;
  } old_arch_recycle_helper_;

  int max_limit_ = 0;

  // the flag will be set up, when new item is added into list
  bool list_perf_report_flag_ = false;

  // ListAdapter is owned by RenderList.
  ListAdapter* adapter_ = nullptr;
  // 2nd prefetch cache.
  std::vector<ListItemViewHolder*> cached_items_;
  // 1st layer cache.
  // Before layout: Store all items excluding flagged removed.
  // In layout: Retrieve item(Only the item with consistent type and position
  // is reused in layout.).
  // After layout: Store remaining item to 2nd layer cache and clear.
  std::list<ListItemViewHolder*> attached_scrap_items_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_LIST_LIST_RECYCLER_H_
