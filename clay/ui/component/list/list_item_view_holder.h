// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_LIST_LIST_ITEM_VIEW_HOLDER_H_
#define CLAY_UI_COMPONENT_LIST_LIST_ITEM_VIEW_HOLDER_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "clay/gfx/geometry/float_point.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/list/layout_types.h"
#include "clay/ui/component/list/list_adapter.h"

namespace clay {

enum class ListItemStickyType {
  kNone,
  kTop,
  kBottom,
};

class ListItemViewHolder {
 public:
  // Flag that indicate the view holder's state. Some of the valid is not used
  // yet but we keep it to sync with Android RecyclerView.
  // Only kFlagRemoved, kFlagUpdate, kFlagInvalid, kFlagScrapped are used yet.
  enum Flag {
    kFlagNone = 0,
    // This view holder has been bound to a position, `position_` is valid.
    kFlagBound = 1 << 0,
    // Need to be rebound data.
    kFlagUpdate = 1 << 1,
    kFlagInvalid = 1 << 2,
    // This view holder points at data that represents an item previously
    // removed from the data set. Its view may still be used for things like
    // outgoing animation.
    kFlagRemoved = 1 << 3,
    kFlagNotRecyclable = 1 << 4,
    kFlagReturnFromScrap = 1 << 5,
    kFlagIgnore = 1 << 7,
    // When the view is detached from the parent, we set this flag so that we
    // can take correct action when we need to remove it or add it back.
    kFlagTmpDetached = 1 << 8,
    kFlagAdapterPositionUnknown = 1 << 9,
    kFlagAdapterFullUpdate = 1 << 10,
    kFlagMoved = 1 << 11,
    kFlagAdapterInPreLayout = 1 << 12,
    kFlagBouncedFromHiddenList = 1 << 13,
    // This means the item was adding to `attached_scrap_items_` and not be
    // retrieved yet. This is only used during
    // `ListLayoutManager::OnLayoutChildren`.
    kFlagScrapped = 1 << 14,
    // This means the item was prefetched and maybe used in the future.
    kFlagPrefetch = 1 << 15,
  };
  static constexpr int kNoPosition = -1;

  class VisibilityObserver {
   public:
    virtual void OnVisibilityChanged(ListItemViewHolder* item,
                                     bool visible) = 0;
  };

  ListItemViewHolder();
  virtual ~ListItemViewHolder();

  // Will clean the state if the item is removed or updated. Otherwise, keep the
  // state.
  void ResetOnRecycle();

  // Clean the state in all condition.
  void Reset();

  void SetView(BaseView* view);
  BaseView* GetView() const { return view_; }

  void SetVisibilityObserver(VisibilityObserver* observer) {
    observer_ = observer;
  }

  // Each item should lay itself out with the constraint from the layout
  // manager. For example,in vertical linear layout, the width is determined by
  // the layout manager (constraint), and each item should determine its height.
  virtual MeasureResult Measure(const MeasureConstraint& constraint) = 0;

  virtual ListAdapter::TypeId GetItemViewType() const {
    return ListAdapter::kDefaultId;
  }

  // Place the view to the desired location.
  // NOTE(Xietong): instead of setting the latest location to view, the location
  // is cached to layout_location_. During layout, the
  // layout manager will get cached location instead of the actual view
  // location. The cached location will be flush to the view when the whole list
  // finishes scrolling or layout.
  // This is to workaround the behaviour of Lynx:
  // Location of children in the list should be determined by the list. When
  // updating children, however, Lynx will move child view's position and this
  // will influence the layout process. So we cache the desired location to view
  // holder and use this value during layout. Flush the desired location when
  // Lynx won't change the view location during the current layout pass.
  void Layout(const FloatPoint& location);
  // Offset the cached location.
  void LayoutWithOffset(const FloatPoint& offset);
  // Flush the cached location to the view.
  void FlushLayout();

  // Get the dimension of the view. If there's cached location, the cached
  // location is returned.
  float GetTop() const;
  float GetLeft() const;
  virtual float GetWidth() const;
  virtual float GetHeight() const;
  float GetRight() const { return GetLeft() + GetWidth(); }
  float GetBottom() const { return GetTop() + GetHeight(); }
  FloatPoint GetLayoutOrigin() const;

  void SetPosition(int position);
  int GetPosition() const { return position_; }
  int GetLastValidPosition() const { return last_valid_position_; }
  int GetLayoutPosition() const { return GetPosition(); }
  void OffsetPosition(int offset, bool apply_to_pre_layout = false);
  void FlagRemovedAndOffsetPosition(int new_position, int offset,
                                    bool apply_to_pre_layout = false);
  void SetFlags(Flag flags, Flag mask);
  void AddFlags(Flag flags);
  void RemoveFlags(Flag flags);

  bool IsRemoved() const { return flags_ & kFlagRemoved; }
  bool IsInvalid() const { return flags_ & kFlagInvalid; }
  bool IsBound() const { return flags_ & kFlagBound; }
  bool IsScrapped() const { return flags_ & kFlagScrapped; }
  bool IsPrefetch() const { return flags_ & kFlagPrefetch; }
  bool NeedsUpdate() const { return flags_ & kFlagUpdate; }
  bool HasAnyOfFlags(Flag flags) const { return (flags_ & flags) != 0; }

  // If item's view is attached, it is attached to the page view tree.
  // If it is not attached, it means the item has been recycled.
  bool GetViewAttached() const {
    if (view_) {
      return view_->Parent();
    }
    return false;
  }

  // If item's view is visible, it must be attached. However, an attached view
  // is not necessarily visible.
  // For example, when we search for focusable node, we may attach items out of
  // bounds. In this case, the item is attached but not visible.
  // Currently, there are 3 chances for an item to change its visibility:
  //  1. When an item is recycled, recycler will notify adapter and adapter
  //     marks item as invisible.
  //  2. When an item is bound to a new position, adapter will also mark it as
  //     invisible so that Lynx can receive notification that the item in the
  //     original position disappears.
  //  3. In the end of layout or scrolling, layout manager will iterate all the
  //     attached items and mark the in bound items visible.
  void SetViewVisible(bool visible);
  bool GetViewVisible() const {
    if (view_) {
      return view_->Visible();
    }
    return false;
  }

  // adding nullptr once means fully update.
  void AddChangePayload(std::unique_ptr<ListAdapter::Payload> payload);
  void ClearPayloads();
  const std::vector<std::unique_ptr<ListAdapter::Payload>>& GetPayloads() const;

  // When full span is enabled, this item will ignore list view's padding in the
  // secondary direction.
  void SetFullSpan(bool enabled) { full_span_ = enabled; }
  bool FullSpan() const { return full_span_; }

  void SetStickyType(ListItemStickyType type) { sticky_type_ = type; }
  ListItemStickyType StickyType() const { return sticky_type_; }

  void SetAdjustedLocation(float value) { adjusted_location_ = value; }
  void ClearAdjustedLocation() { adjusted_location_.reset(); }

  std::string ToString() const;

 private:
  BaseView* view_ = nullptr;

  int position_ = kNoPosition;
  // Used for node disappearing when the item is removed.
  int last_valid_position_ = kNoPosition;

  Flag flags_ = kFlagNone;

  // The location in list. It may be overwritten by the `adjusted_location_` if
  // it is a sticky item.
  std::optional<FloatPoint> layout_location_;

  // The header/footer items will be displayed at the top or bottom of list,
  // instead of the `layout_location_`.
  std::optional<float> adjusted_location_;

  ListAdapter::Payloads payloads_;

  bool full_span_ = false;
  ListItemStickyType sticky_type_ = ListItemStickyType::kNone;

  VisibilityObserver* observer_ = nullptr;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_LIST_LIST_ITEM_VIEW_HOLDER_H_
