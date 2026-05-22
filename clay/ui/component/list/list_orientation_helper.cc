// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/list/list_orientation_helper.h"

#include "clay/fml/logging.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/list/list_item_view_holder.h"
#include "clay/ui/component/list/list_layout_manager.h"

namespace clay {

void ListOrientationHelper::OnLayoutCompleted() {
  last_total_space_ = GetTotalSpace();
}

float ListOrientationHelper::GetTotalSpaceChange() const {
  return last_total_space_ == kInvalidSize
             ? 0.f
             : GetTotalSpace() - last_total_space_;
}

class ListOrientationHelperVertical : public ListOrientationHelper {
 public:
  explicit ListOrientationHelperVertical(ListLayoutManager* manager)
      : ListOrientationHelper(manager) {}
  ~ListOrientationHelperVertical() override = default;

  float GetEndAfterPadding() const override {
    return manager_->GetHeight() - manager_->GetPaddingBottom();
  }

  float GetEnd() const override { return manager_->GetHeight(); }

  float GetEndPadding() const override { return manager_->GetPaddingBottom(); }

  float GetDecoratedEnd(const ListItemViewHolder* child) const override {
    FML_DCHECK(child);
    return child->GetBottom();
  }

  float GetStartAfterPadding() const override {
    return manager_->GetPaddingTop();
  }

  float GetDecoratedStart(const ListItemViewHolder* child) const override {
    FML_DCHECK(child);
    return child->GetTop();
  }

  float GetSecondaryStartAfterPadding() const override {
    return manager_->GetPaddingLeft();
  }

  float GetRectStart(const FloatRect& rect) const override { return rect.y(); }

  float GetRectEnd(const FloatRect& rect) const override { return rect.MaxY(); }

  float GetDecoratedMeasure(const ListItemViewHolder* child) const override {
    FML_DCHECK(child);
    return child->GetBottom() - child->GetTop();
  }

  float GetSecondaryDecoratedMeasure(
      const ListItemViewHolder* child) const override {
    FML_DCHECK(child);
    return child->GetRight() - child->GetLeft();
  }

  void OffsetChildren(float amount) override {
    manager_->OffsetChildren(0.f, amount);
  }

  float GetTotalSpace() const override {
    float space = manager_->GetHeight() - manager_->GetPaddingTop() -
                  manager_->GetPaddingBottom();
    if (space < 0.f) {
      return 0.f;
    }
    return space;
  }

  float GetSecondaryTotalSpace() const override {
    float space = manager_->GetWidth() - manager_->GetPaddingLeft() -
                  manager_->GetPaddingRight();
    if (space < 0.f) {
      return 0.f;
    }
    return space;
  }

  FloatPoint CalculateFullSpanLocation(
      const FloatPoint& old_location,
      const ListItemViewHolder* child) const override {
    FML_DCHECK(child);
    float remaining_space = manager_->GetWidth() - child->GetView()->Width();
    float left = 0.f;
    if (remaining_space > 0.f) {
      float total_padding =
          manager_->GetPaddingLeft() + manager_->GetPaddingRight();
      float front_padding = manager_->GetPaddingLeft();
      // if remained space is larger than total padding, use space - total
      // padding else use scaled left padding in total space.
      remaining_space -= total_padding;
      if (remaining_space >= 0.f) {
        left = front_padding;
      } else {
        float ratio = front_padding / total_padding;
        left = front_padding + (remaining_space * ratio);
      }
    }
    return FloatPoint(left, old_location.y());
  }
};

class ListOrientationHelperHorizontal : public ListOrientationHelper {
 public:
  explicit ListOrientationHelperHorizontal(ListLayoutManager* manager)
      : ListOrientationHelper(manager) {}
  ~ListOrientationHelperHorizontal() override = default;

  float GetEndAfterPadding() const override {
    return manager_->GetWidth() - manager_->GetPaddingRight();
  }

  float GetEnd() const override { return manager_->GetWidth(); }

  float GetEndPadding() const override { return manager_->GetPaddingRight(); }

  float GetDecoratedEnd(const ListItemViewHolder* child) const override {
    FML_DCHECK(child);
    return child->GetRight();
  }

  float GetStartAfterPadding() const override {
    return manager_->GetPaddingLeft();
  }

  float GetDecoratedStart(const ListItemViewHolder* child) const override {
    FML_DCHECK(child);
    return child->GetLeft();
  }

  float GetSecondaryStartAfterPadding() const override {
    return manager_->GetPaddingTop();
  }

  float GetRectStart(const FloatRect& rect) const override { return rect.x(); }

  float GetRectEnd(const FloatRect& rect) const override { return rect.MaxX(); }

  float GetDecoratedMeasure(const ListItemViewHolder* child) const override {
    FML_DCHECK(child);
    return child->GetRight() - child->GetLeft();
  }

  float GetSecondaryDecoratedMeasure(
      const ListItemViewHolder* child) const override {
    FML_DCHECK(child);
    return child->GetBottom() - child->GetTop();
  }

  void OffsetChildren(float amount) override {
    manager_->OffsetChildren(amount, 0.f);
  }

  float GetTotalSpace() const override {
    float space = manager_->GetWidth() - manager_->GetPaddingLeft() -
                  manager_->GetPaddingRight();
    if (space < 0.f) {
      return 0.f;
    }
    return space;
  }

  float GetSecondaryTotalSpace() const override {
    float space = manager_->GetHeight() - manager_->GetPaddingTop() -
                  manager_->GetPaddingBottom();
    if (space < 0.f) {
      return 0.f;
    }
    return space;
  }

  FloatPoint CalculateFullSpanLocation(
      const FloatPoint& old_location,
      const ListItemViewHolder* child) const override {
    FML_DCHECK(child);
    float remaining_space = manager_->GetHeight() - child->GetView()->Height();
    float top = 0.f;
    if (remaining_space > 0.f) {
      float total_padding =
          manager_->GetPaddingTop() + manager_->GetPaddingBottom();
      float front_padding = manager_->GetPaddingTop();
      // if remained space is larger than total padding, use space - total
      // padding else use scaled left padding in total space.
      remaining_space -= total_padding;
      if (remaining_space >= 0.f) {
        top = front_padding;
      } else {
        float ratio = front_padding / total_padding;
        top = front_padding + (remaining_space * ratio);
      }
    }
    return FloatPoint(old_location.x(), top);
  }
};

ListOrientationHelper::ListOrientationHelper(ListLayoutManager* manager)
    : manager_(manager) {
  FML_DCHECK(manager_);
}
ListOrientationHelper::~ListOrientationHelper() = default;

// static
std::unique_ptr<ListOrientationHelper>
ListOrientationHelper::CreateOrientationHelper(ListLayoutManager* manager,
                                               ScrollDirection orientation) {
  if (orientation == ScrollDirection::kVertical) {
    return std::make_unique<ListOrientationHelperVertical>(manager);
  } else if (orientation == ScrollDirection::kHorizontal) {
    return std::make_unique<ListOrientationHelperHorizontal>(manager);
  }
  FML_CHECK(0) << "Invalid ListLayoutOrientation "
               << static_cast<int>(orientation);
  return nullptr;
}

}  // namespace clay
