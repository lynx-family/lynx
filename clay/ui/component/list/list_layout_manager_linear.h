// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_LIST_LIST_LAYOUT_MANAGER_LINEAR_H_
#define CLAY_UI_COMPONENT_LIST_LIST_LAYOUT_MANAGER_LINEAR_H_

#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "clay/ui/component/list/layout_types.h"
#include "clay/ui/component/list/list_item_view_holder.h"
#include "clay/ui/component/list/list_layout_manager.h"

namespace clay {

class ListOrientationHelper;

class ListLayoutManagerLinear : public ListLayoutManager {
 public:
  explicit ListLayoutManagerLinear(
      ScrollDirection direction = kDefaultScrollDirection);
  virtual ~ListLayoutManagerLinear();

  void OnLayoutChildren(ListRecycler& recycler,
                        const ListViewState& state) override;

  void OnLayoutCompleted(ListRecycler& recycler,
                         const ListViewState& state) override;

  float ScrollVerticallyBy(float dy, ListRecycler& recycler,
                           const ListViewState& state) override;
  float ScrollHorizontallyBy(float dx, ListRecycler& recycler,
                             const ListViewState& state) override;

  bool SetOrientation(ScrollDirection direction) override;

  void OnFocusSearchFailed(bool to_end, ListRecycler& recycler,
                           const ListViewState& state) override;
  void ScrollToPosition(int position, AlignTo align_to) override;
  FloatSize ScrollToRect(const FloatRect& rect, AlignTo align_to) override;
  std::optional<PositionAndRelativeRect> GetChildPositionByRect(
      const FloatRect& rect) override;

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
  void CollectInitialPrefetchPositions(
      int adapter_items_count, const ListViewState& list_view_state,
      LayoutPrefetchRegistry* layout_prefetch_registry) override;

  virtual void CollectPrefetchPositionForLayoutState(
      const ListViewState& list_view_state, int abs_velocity, int max_limit,
      LayoutPrefetchRegistry* layout_prefetch_registry);

 protected:
  class LayoutState {
   public:
    // Used when there is no limit in how many views can be laid out.
    bool infinite_;
    bool recycle_enabled_;

    ListItemDirection item_direction_;

    ListLayoutDirection layout_direction_;

    // Number of pixels that we should fill, in the layout direction.
    float available_;

    // Pixel offset where layout should start
    float offset_;

    // Current position on the adapter to get the next item.
    int current_position_;

    /**
     * Used when LayoutState is constructed in a scrolling state.
     * It should be set the amount of scrolling we can make without creating a
     * new view. Settings this is required for efficient view recycling.
     */
    std::optional<float> scrolling_offset_;

    /**
     * Android use this to pre-layout items that are not yet visible. But
     * currently we only use this to fix the layout issue when
     * padding-top/-bottom exists.
     * The difference with `available_` is that, when recycling, distance laid
     * out for `extra_` is not considered to avoid recycling visible children.
     */
    float extra_ = 0;

    float last_scroll_delta_;

    bool HasMore(const ListViewState& state);
    /**
     * @brief Get ListItemViewHolder instance according to `current_position_`
     * and increase/decrease `current_position_`
     *
     * @param recycler Recycler for items reusing
     * @return ListItemViewHolder*
     */
    ListItemViewHolder* Next(ListRecycler& recycler);
    std::string ToString() const;
  };

  class AnchorInfo {
   public:
    bool valid_;
    bool layout_from_end_;
    int position_;
    float coordinate_;

    static constexpr float kInvalidOffset =
        std::numeric_limits<float>::lowest();

    void Reset();
    void AssignCoordinateFromPadding(const ListOrientationHelper& helper,
                                     const LayoutState& state);
    void AssignFromChild(ListItemViewHolder* child,
                         const ListOrientationHelper& helper,
                         const LayoutState& state);
  };

  virtual void OnAnchorReady(ListRecycler& recycler, const ListViewState& state,
                             AnchorInfo& anchor_info, bool is_prime_direction) {
  }

  LayoutState layout_state_;
  AnchorInfo anchor_info_;

  int pending_scroll_position_ = ListItemViewHolder::kNoPosition;
  AlignTo pending_scroll_align_to_ = AlignTo::kNone;

 private:
  void ResetLayoutState();
  void UpdateLayoutState(ListLayoutDirection layout_direction,
                         float required_space, bool can_use_existing_space,
                         const ListViewState& state);
  void UpdateLayoutStateToFillStart(int item_pos, float offset);
  void UpdateLayoutStateToFillEnd(int item_pos, float offset);

  float ScrollBy(float delta, ListRecycler& recycler,
                 const ListViewState& state);

  void UpdateAnchorInfo(ListRecycler& recycler, const ListViewState& state);
  bool UpdateAnchorInfoFromPendingData(const ListViewState& state);
  bool UpdateAnchorInfoFromChildren(ListRecycler& recycler,
                                    const ListViewState& state);
  virtual ListItemViewHolder* FindReferenceChild(ListRecycler& recycler,
                                                 const ListViewState& state);

  float Fill(ListRecycler& recycler, const ListViewState& state);
  virtual float LayoutChunk(ListRecycler& recycler, const ListViewState& state);

  float FixLayoutStartGap(float start_offset, ListRecycler& recycler,
                          const ListViewState& state, bool can_offset_children);

  float FixLayoutEndGap(float end_offset, ListRecycler& recycler,
                        const ListViewState& state, bool can_offset_children);

  void RecycleByLayoutState(ListRecycler& recycler);
  void RecycleViewsFromEnd(ListRecycler& recycler, float dt);
  void RecycleViewsFromStart(ListRecycler& recycler, float dt);
  void RecycleChildren(ListRecycler& recycler, int start_index, int end_index);

  std::vector<int> GetVisibleItemPositions() override;
  int FindOneVisibleChildPosition(bool reverse, bool complete_visible);
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_LIST_LIST_LAYOUT_MANAGER_LINEAR_H_
