// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_LIST_LIST_LAYOUT_MANAGER_H_
#define CLAY_UI_COMPONENT_LIST_LIST_LAYOUT_MANAGER_H_

#include <list>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/geometry/float_rect.h"
#include "clay/gfx/scroll_direction.h"
#include "clay/ui/component/list/layout_types.h"
#include "clay/ui/component/list/list_item_length_cache.h"
#include "clay/ui/component/view_callback/list_event_callback_manager.h"

namespace clay {

class BaseListView;
class LayoutPrefetchRegistry;
class ListAdapter;
class ListChildrenHelper;
class ListItemLengthCache;
class ListItemViewHolder;
class ListOrientationHelper;
class ListRecycler;
struct ListViewState;

class ListLayoutManager {
 public:
  explicit ListLayoutManager(
      ScrollDirection orientation = kDefaultScrollDirection);
  virtual ~ListLayoutManager();

  static std::unique_ptr<ListLayoutManager> Create(const std::string& type,
                                                   int span_count);

  void SetListView(BaseListView* list_view);
  void SetChildrenHelper(ListChildrenHelper* children_helper);

  virtual void OnLayoutChildren(ListRecycler& recycler,
                                const ListViewState& state) = 0;

  // Subclass should call this method.
  virtual void OnLayoutCompleted(ListRecycler& recycler,
                                 const ListViewState& state);

  virtual float ScrollHorizontallyBy(float dx, ListRecycler& recycler,
                                     const ListViewState& state) {
    return 0.f;
  }

  virtual float ScrollVerticallyBy(float dy, ListRecycler& recycler,
                                   const ListViewState& state) {
    return 0.f;
  }

  void ScrapFloatingItems(ListRecycler* recycler);
  void LayoutSticky(ListRecycler* recycler, ListAdapter* adapter);

  void SetStickyOffset(double offset);

  bool CanScrollHorizontally() const {
    return orientation_ == ScrollDirection::kHorizontal;
  }
  bool CanScrollVertically() const {
    return orientation_ == ScrollDirection::kVertical;
  }

  float GetWidth();
  float GetHeight();
  float GetPaddingLeft();
  float GetPaddingRight();
  float GetPaddingTop();
  float GetPaddingBottom();

  virtual void OffsetChildren(float x, float y);

  // Delete item's layout param.
  virtual void OnRemoveItem(ListItemViewHolder* item) {}

  ListOrientationHelper* GetOrientationHelper() const {
    return orientation_helper_.get();
  }
  ScrollDirection GetOrientation() const { return orientation_; }
  // return whether orientation changes
  virtual bool SetOrientation(ScrollDirection orientation);

  // Layout the list with an area larger than the visible part.
  virtual void OnFocusSearchFailed(bool to_end, ListRecycler& recycler,
                                   const ListViewState& state) {
    // Should implement by layout manager subclass.
  }

  /**
   * Called in response to a call to `ListAdapter::NotifyDataSetChanged()` and
   * signals that the the entire data set has changed.
   *
   * Used by staggered grid layout manager.
   */
  virtual void OnItemsChanged(BaseListView* list_view) {}

  // A range of callback to handle item change from adapter.
  virtual void OnItemsAdded(BaseListView* list_view, int position_start,
                            int item_count) {
    length_cache_->OnItemsAdded(position_start, item_count);
  }
  virtual void OnItemsRemoved(BaseListView* list_view, int position_start,
                              int item_count) {
    length_cache_->OnItemsRemoved(position_start, item_count);
  }
  virtual void OnItemsUpdated(BaseListView* list_view, int position_start,
                              int item_count) {
    length_cache_->OnItemsUpdated(position_start, item_count);
  }
  virtual void OnItemsMoved(BaseListView* list_view, int from, int to,
                            int item_count) {
    length_cache_->OnItemsMoved(from, to, item_count);
  }

  // scroll state changed notification
  virtual void OnScrollStateChange(Scrollable::ScrollStatus state) {}

  // The actual scroll will be delayed until the next `OnLayoutChildren()` call.
  virtual void ScrollToPosition(int position,
                                AlignTo align_to = AlignTo::kNone) {
    // Should implement by layout manager subclass.
  }

  // `rect` is a location currently relative to the list view which may be in
  // the invisible area of the list view (out of bounds). Call the method to
  // scroll the list so that the rect will be in the visible area (in box).
  // Only called by `BaseListView` to handle the focus logic.
  // Returns the scroll distance in either horizontal or vertical direction and
  // the actual scroll is done by `BaseListView`.
  virtual FloatSize ScrollToRect(const FloatRect& rect, AlignTo align_to) {
    // Should implement by layout manager subclass.
    return {0.f, 0.f};
  }

  using PositionAndRelativeRect = std::pair<int, FloatRect>;
  virtual std::optional<PositionAndRelativeRect> GetChildPositionByRect(
      const FloatRect& rect) {
    return std::nullopt;
  }

  // Can still scroll to the start direction.
  virtual bool HasSpaceToStart(const ListViewState& state) { return false; }
  // Can still scroll to the end direction.
  virtual bool HasSpaceToEnd(const ListViewState& state) { return false; }

  // Get first child that has start lower than list view's start. No such child
  // is found, the first is returned.
  virtual ListItemViewHolder* GetFirstInBoxChild(const ListViewState& state) {
    return nullptr;
  }
  // Get last child that has end above list view's end. No such child is found,
  // the last is returned.
  virtual ListItemViewHolder* GetLastInBoxChild(const ListViewState& state) {
    return nullptr;
  }

  bool IsItemFullSpan(int position) const;

  // Get infomation of currently visible items in list
  size_t GetVisibleItemsInfo(std::vector<float>& top_array,
                             std::vector<float>& bottom_array,
                             std::vector<float>& left_array,
                             std::vector<float>& right_array,
                             std::vector<int>& position,
                             std::vector<std::string>& id_array);

  void UpdateItemsVisibility();

  void RemoveAndRecycleAllViews(ListRecycler& recycler);

  virtual float GetScrollOffset(const ListViewState& state) { return 0.f; }
  virtual float GetTotalLength(const ListViewState& state) { return 0.f; }

  void SetItemPrefetchEnabled(bool enabled);
  bool IsItemPrefetchEnabled() { return prefetch_enabled_; }

  virtual void CollectAdjacentPrefetchPositions(
      int delta_x, int delta_y, int max_limit,
      const ListViewState& list_view_state,
      LayoutPrefetchRegistry* layout_prefetch_registry) {}
  virtual void CollectInitialPrefetchPositions(
      int adapter_items_count, const ListViewState& list_view_state,
      LayoutPrefetchRegistry* layout_prefetch_registry) {}

  void SetCrossAxisGap(float cross_axis_gap);
  void SetMainAxisGap(float main_axis_gap);

 protected:
  void RequestLayout();
  // Add item to RenderList.
  // However, in Clay, the render object tree is not operated directly.
  // Instead it is driven by the view tree.
  // When index is -1, it means add the item as the last child.
  void AddItem(ListItemViewHolder* item_view, int index);

  MeasureResult MeasureItem(ListItemViewHolder* item,
                            const MeasureConstraint& constraint);

  void LayoutItem(ListItemViewHolder* item, const FloatPoint& top_left);

  // Child (view holders) manipulation
  int GetChildCount() const;
  ListItemViewHolder* GetChildAt(int index);
  ListItemViewHolder* GetFirstChild() { return GetChildAt(0); }
  ListItemViewHolder* GetLastChild() { return GetChildAt(GetChildCount() - 1); }

  ListItemViewHolder* FindChildByPosition(int position);

  void RemoveAndRecycleViewAt(ListRecycler& recycler, int index);

  void RemoveAndScrapChildren(ListRecycler& recycler);
  void RemoveAndScrapChild(ListRecycler& recycler, int index);

  virtual std::vector<int> GetVisibleItemPositions() { return {}; }

  MeasureConstraint FullSpanConstraint();

 protected:
  float cross_axis_gap_ = 0;
  float main_axis_gap_ = 0;
  // Not owned. It is borrowed from BaseListView.
  ListChildrenHelper* children_helper_ = nullptr;

  std::unique_ptr<ListOrientationHelper> orientation_helper_;
  ScrollDirection orientation_ = ScrollDirection::kNone;

  std::unique_ptr<ListItemLengthCache> length_cache_;

 private:
  // Make list_view_ private. The subclass should not use it directly.
  BaseListView* list_view_ = nullptr;
  bool prefetch_enabled_ = true;
  double sticky_offset_ = 0.0;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_LIST_LIST_LAYOUT_MANAGER_H_
