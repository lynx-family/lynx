// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_LIST_LIST_LAYOUT_MANAGER_STAGGERED_GRID_H_
#define CLAY_UI_COMPONENT_LIST_LIST_LAYOUT_MANAGER_STAGGERED_GRID_H_

#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "clay/fml/logging.h"
#include "clay/ui/component/list/list_adapter_helper.h"
#include "clay/ui/component/list/list_common/layout_types.h"
#include "clay/ui/component/list/list_item_length_cache.h"
#include "clay/ui/component/list/list_item_view_holder.h"
#include "clay/ui/component/list/list_layout_manager.h"

namespace clay {

class Span;
class FullSpanItem;
class LazySpanLookup;

class ListLayoutManagerStaggeredGrid : public ListLayoutManager {
 public:
  explicit ListLayoutManagerStaggeredGrid(
      int span_count, ScrollDirection orientation = kDefaultScrollDirection);
  ~ListLayoutManagerStaggeredGrid() override;

  void OnLayoutChildren(ListRecycler& recycler,
                        const ListViewState& state) override;

  void OnLayoutCompleted(ListRecycler& recycler,
                         const ListViewState& state) override;

  float ScrollVerticallyBy(float dy, ListRecycler& recycler,
                           const ListViewState& state) override;

  float ScrollHorizontallyBy(float dx, ListRecycler& recycler,
                             const ListViewState& state) override;

  void OffsetChildren(float dx, float dy) override;

  void OnRemoveItem(ListItemViewHolder* item) override;

  void OnItemsChanged(BaseListView* list_view) override;

  void OnItemsAdded(BaseListView* list_view, int position_start,
                    int item_count) override;
  void OnItemsRemoved(BaseListView* list_view, int position_start,
                      int item_count) override;
  void OnItemsUpdated(BaseListView* list_view, int position_start,
                      int item_count) override;
  void OnItemsMoved(BaseListView* list_view, int from, int to,
                    int item_count) override;
  void InvalidateLayoutCache() override;

  int GetSpanCount() const { return span_count_; }
  void SetSpanCount(int span_count);

  bool SetOrientation(ScrollDirection orientation) override;

  void OnFocusSearchFailed(bool to_end, ListRecycler& recycler,
                           const ListViewState& state) override;

  void OnScrollStateChange(Scrollable::ScrollStatus state) override;

  void ScrollToPosition(int position, AlignTo align_to) override;
  FloatSize ScrollToRect(const FloatRect& rect, AlignTo align_to) override;

  bool HasSpaceToStart(const ListViewState& state) override;
  bool HasSpaceToEnd(const ListViewState& state) override;
  ListItemViewHolder* GetFirstInBoxChild(const ListViewState& state) override;
  ListItemViewHolder* GetLastInBoxChild(const ListViewState& state) override;

  float GetScrollOffset(const ListViewState& state) override;
  float GetTotalLength(const ListViewState& state) override;
  void CollectAdjacentPrefetchPositions(
      int delta_x, int delta_y, int max_limit,
      const ListViewState& list_view_state,
      LayoutPrefetchRegistry* layout_prefetch_registry) override;

 private:
  friend class Span;

  struct LayoutParam {
    bool full_span_;
    Span* span_;
  };

  class LayoutState {
   public:
    // We may not want to recycle children in some cases (e.g. layout)
    bool recycle_ = true;
    // Used when there is no limit in how many views can be laid out.
    bool infinite_;

    ListItemDirection item_direction_;
    ListLayoutDirection layout_direction_;

    // Number of pixels that we should fill, in the layout direction.
    float available_;

    // Pixel offset where layout should start
    float offset_;

    // Current position on the adapter to get the next item.
    int current_position_;

    // This is the target pixel closest to the start of the layout that we are
    // trying to fill
    float start_line_;
    float end_line_;
    bool HasMore(const ListViewState& state);
    ListItemViewHolder* Next(ListRecycler& recycler);
    std::string ToString() const;
  };

  class AnchorInfo {
   public:
    int position_;
    float offset_;
    bool layout_from_end_;
    bool invalidate_offsets_;
    bool valid_;
    std::vector<int> span_reference_lines_;

    static constexpr float kInvalidOffset = std::numeric_limits<float>::min();

    void Reset();
    void AssignFromChild(ListItemViewHolder* child,
                         const ListOrientationHelper& helper,
                         const LayoutState& state);
    void SaveSpanReferenceLines(std::vector<std::unique_ptr<Span>>& spans);

    std::string ToString() const;
  };

  float ScrollBy(float delta, ListRecycler& recycler,
                 const ListViewState& state);

  void OnLayoutChildrenInternal(ListRecycler& recycler,
                                const ListViewState& state,
                                bool should_check_for_gaps);

  float Fill(ListRecycler& recycler, const ListViewState& state);

  void FixLayoutStartGap(ListRecycler& recycler, const ListViewState& state,
                         bool can_offset_children);
  void FixLayoutEndGap(ListRecycler& recycler, const ListViewState& state,
                       bool can_offset_children);
  bool CheckForGaps();
  ListItemViewHolder* HasGapsToFix();

  void SetLayoutStateDirection(ListLayoutDirection direction);
  std::unique_ptr<FullSpanItem> CreateFullSpanItemFromEnd(int new_item_top);
  std::unique_ptr<FullSpanItem> CreateFullSpanItemFromStart(
      int new_item_bottom);

  void UpdateAnchorInfo(const ListViewState& state);
  bool UpdateAnchorInfoFromChildren(const ListViewState& state);
  bool UpdateAnchorInfoFromPendingData(const ListViewState& state);
  int FindReferenceChildPosition(int item_count, bool reversed);

  ListLayoutDirection CalculateScrollDirectionForPosition(int position);
  void UpdateMeasureSpecs(float total_space);
  void PrepareLayoutStateForDelta(float delta, const ListViewState& state);
  void UpdateLayoutState(int anchor_position, const ListViewState& state);

  LayoutParam& GetLayoutParam(ListItemViewHolder* item);

  void AttachItemToSpans(ListItemViewHolder* item, LayoutParam& layout_param,
                         bool laying_out_in_primary_direction);
  void AppendItemToAllSpans(ListItemViewHolder* item);
  void PrependItemToAllSpans(ListItemViewHolder* item);

  int GetLastChildPosition();
  int GetFirstChildPosition();
  float GetMinStart(float default_line);
  float GetMaxStart(float default_line);
  float GetMinEnd(float default_line);
  float GetMaxEnd(float default_line);
  Span* GetNextSpan();

  std::vector<int> GetVisibleItemPositions() override;

  bool CheckAllEndsEqual();
  bool CheckAllStartsEqual();

  float CalculateAccumulatedLength(int to_position);

  // return the number of full spans in specified layout_dir and target_line
  int CalculateNumberOfUnavailableSpans(ListLayoutDirection layout_dir,
                                        float target_line);
  int UpdateRemainingSpans(Span* span, ListLayoutDirection layout_dir,
                           float target_line);

  void HandleUpdate(int position_start, int item_count_or_to_position,
                    ListAdapterHelper::Type cmd);

  void Recycle(ListRecycler& recycler);
  void RecycleFromStart(ListRecycler& recycler, float line);
  void RecycleFromEnd(ListRecycler& recycler, float line);

  void ResolveShouldLayoutReverse();

  LayoutState layout_state_;

  int span_count_ = 0;

  float size_per_span_;
  float full_size_spec_;

  bool last_layout_from_end_ = false;

  bool laid_out_invalid_full_span_ = false;

  bool should_reverse_layout_ = false;

  // When LayoutManager needs to scroll to a position, it sets this variable and
  // requests a layout which will check this variable and re-layout accordingly.
  int pending_scroll_position_ = ListItemViewHolder::kNoPosition;
  AlignTo pending_scroll_align_to_ = AlignTo::kNone;

  std::unique_ptr<LazySpanLookup> lazy_span_lookup_;
  AnchorInfo anchor_info_;

  // Temporary variable used during fill method to check which spans needs to
  // be filled.
  std::vector<bool> remaining_spans_;

  std::vector<std::unique_ptr<Span>> spans_;

  // local cache for [key=view_id:value=layout_param]
  std::unordered_map<int, LayoutParam> layout_params_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_LIST_LIST_LAYOUT_MANAGER_STAGGERED_GRID_H_
