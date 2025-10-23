// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_LIST_LIST_ORIENTATION_HELPER_H_
#define CLAY_UI_COMPONENT_LIST_LIST_ORIENTATION_HELPER_H_

#include <limits>
#include <memory>

#include "clay/gfx/geometry/float_rect.h"
#include "clay/gfx/scroll_direction.h"

namespace clay {

class ListLayoutManager;
class ListItemViewHolder;

class ListOrientationHelper {
 public:
  virtual ~ListOrientationHelper();

  virtual float GetEndAfterPadding() const = 0;
  virtual float GetEnd() const = 0;
  virtual float GetEndPadding() const = 0;
  virtual float GetDecoratedEnd(const ListItemViewHolder* child) const = 0;

  virtual float GetStartAfterPadding() const = 0;
  virtual float GetDecoratedStart(const ListItemViewHolder* child) const = 0;
  virtual float GetSecondaryStartAfterPadding() const = 0;

  virtual float GetRectStart(const FloatRect& rect) const = 0;
  virtual float GetRectEnd(const FloatRect& rect) const = 0;

  virtual float GetDecoratedMeasure(const ListItemViewHolder* child) const = 0;
  virtual float GetSecondaryDecoratedMeasure(
      const ListItemViewHolder* child) const = 0;

  virtual void OffsetChildren(float amount) = 0;

  virtual float GetTotalSpace() const = 0;
  float GetTotalSpaceChange() const;
  virtual float GetSecondaryTotalSpace() const = 0;

  virtual FloatPoint CalculateFullSpanLocation(
      const FloatPoint& old_location,
      const ListItemViewHolder* child) const = 0;

  static std::unique_ptr<ListOrientationHelper> CreateOrientationHelper(
      ListLayoutManager* manager, ScrollDirection orientation);

  /**
   * Call this method after OnLayoutChildren method is complete. This method
   * records information like layout bounds that might be useful in the next
   * layout calculations.
   */
  void OnLayoutCompleted();

 protected:
  explicit ListOrientationHelper(ListLayoutManager* manager);
  // Not owned. Instead, it is the layout manager owning the helper.
  ListLayoutManager* manager_;

  static constexpr float kInvalidSize = std::numeric_limits<float>::lowest();

  float last_total_space_ = kInvalidSize;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_LIST_LIST_ORIENTATION_HELPER_H_
