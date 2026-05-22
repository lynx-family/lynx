// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_LIST_LIST_LAYOUT_MANAGER_GRID_H_
#define CLAY_UI_COMPONENT_LIST_LIST_LAYOUT_MANAGER_GRID_H_

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "clay/ui/component/list/list_layout_manager_linear.h"

namespace clay {

// TODO(hanhaoshen): do we need prelayout?
class ListLayoutManagerGrid : public ListLayoutManagerLinear {
 public:
  explicit ListLayoutManagerGrid(
      int span_count, ScrollDirection orientation = kDefaultScrollDirection);
  ~ListLayoutManagerGrid() override;

  // A helper class to provide the number of spans each item occupies.
  // Default implementation sets each item to occupy exactly 1 span.
  class SpanSizeLookup {
   public:
    explicit SpanSizeLookup(bool enable_cache);
    virtual ~SpanSizeLookup() = default;
    // Returns the number of span occupied by the item at position.
    virtual int GetSpanSize(int position) const = 0;
    // Returns the final span index of the provided position.
    int GetCachedSpanIndex(int position, int span_count);
    // Returns the index of the group this position belongs.
    int GetCachedSpanGroupIndex(int adapter_position, int span_count);

    void InvalidCaches();
    void SetEnableCache(bool enable);

   private:
    virtual int GetSpanIndex(int position, int span_count);
    virtual int GetSpanGroupIndex(int adapter_position, int span_count);

    bool enable_cache_;
    std::unordered_map<int, int> span_index_cache_;
    std::unordered_map<int, int> span_group_index_cache_;
  };

  int GetSpanSize(ListRecycler& recycler, const ListViewState& state,
                  int pos) const;
  int GetSpanIndex(ListRecycler& recycler, const ListViewState& state,
                   int pos) const;
  int GetSpanGroupIndex(ListRecycler& recycler, const ListViewState& state,
                        int pos) const;

  float ScrollVerticallyBy(float dy, ListRecycler& recycler,
                           const ListViewState& state) override;
  float ScrollHorizontallyBy(float dx, ListRecycler& recycler,
                             const ListViewState& state) override;

  void OnItemsChanged(BaseListView* list_view) override;

  void OnItemsAdded(BaseListView* list_view, int position_start,
                    int item_count) override;
  void OnItemsRemoved(BaseListView* list_view, int position_start,
                      int item_count) override;
  void OnItemsUpdated(BaseListView* list_view, int position_start,
                      int item_count) override;
  void OnItemsMoved(BaseListView* list_view, int from, int to,
                    int item_count) override;

  int GetSpanCount() const { return span_count_; }
  void SetSpanCount(int span_count);
  void SetSpanSizeLookup(std::unique_ptr<SpanSizeLookup> span_size_lookup) {
    span_size_lookup_ = std::move(span_size_lookup);
  }

  float GetScrollOffset(const ListViewState& state) override;
  float GetTotalLength(const ListViewState& state) override;

  void CollectPrefetchPositionForLayoutState(
      const ListViewState& list_view_state, int abs_delta, int max_limit,
      LayoutPrefetchRegistry* layout_prefetch_registry) override;

 private:
  struct LayoutParam {
    // the indices of item in listview
    int span_index_;

    int span_size_;
  };

  float LayoutChunk(ListRecycler& recycler,
                    const ListViewState& state) override;

  void UpdateMeasurements();
  void OnAnchorReady(ListRecycler& recycler, const ListViewState& state,
                     AnchorInfo& anchor_info, bool is_prime_direction) override;
  void EnsureAnchorSpanPosition(ListRecycler& recycler,
                                const ListViewState& state,
                                AnchorInfo& anchor_info,
                                bool is_prime_direction) const;
  ListItemViewHolder* FindReferenceChild(ListRecycler& recycler,
                                         const ListViewState& state) override;
  void AssignSpan(ListRecycler& recycler, const ListViewState& state, int count,
                  bool laying_out_in_primary_direction,
                  const std::vector<ListItemViewHolder*>& temp_items);

  float GetSpaceForSpanRange(int span_index, int span_size) const;
  const LayoutParam& GetLayoutParam(ListItemViewHolder* item) const;

  // the number of columns in the grid
  int span_count_;

  std::vector<float> cached_borders_;
  std::unique_ptr<SpanSizeLookup> span_size_lookup_;
  // local cache for [key=view_id:value=layout_param]
  std::unordered_map<int, LayoutParam> layout_params_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_LIST_LIST_LAYOUT_MANAGER_GRID_H_
