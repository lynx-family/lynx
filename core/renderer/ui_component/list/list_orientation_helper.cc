// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_component/list/list_orientation_helper.h"

#include <algorithm>

#include "core/renderer/ui_component/list/list_layout_manager.h"

namespace lynx {
namespace tasm {

/**
 * VerticalOrientationHelper
 */
class VerticalOrientationHelper : public ListOrientationHelper {
 public:
  explicit VerticalOrientationHelper(ListLayoutManager* list_layout_manager)
      : ListOrientationHelper(list_layout_manager) {}

  ~VerticalOrientationHelper() override = default;

  bool IsVertical() const override { return true; }

  float GetStartAfterPadding() const override {
    return list_layout_manager_ ? list_layout_manager_->GetPaddingTop() : 0.f;
  }

  float GetEndAfterPadding() const override {
    return list_layout_manager_ ? list_layout_manager_->GetHeight() -
                                      list_layout_manager_->GetPaddingBottom()
                                : 0.f;
  }

  float GetMeasurement() const override {
    return list_layout_manager_ ? list_layout_manager_->GetHeight() : 0.f;
  }

  float GetMeasurementInOther() const override {
    return list_layout_manager_ ? list_layout_manager_->GetWidth() : 0.f;
  }

  float GetMeasurementInOtherWithoutPadding() const override {
    return list_layout_manager_->GetWidth() -
           list_layout_manager_->GetPaddingLeft() -
           list_layout_manager_->GetPaddingRight();
  }

  float GetStartAfterPaddingInOther() const override {
    return list_layout_manager_ ? list_layout_manager_->GetPaddingLeft() : 0.f;
  }

  float GetEndPadding() const override {
    return list_layout_manager_ ? list_layout_manager_->GetPaddingBottom()
                                : 0.f;
  }

  float GetDecoratedMeasurement(const ItemHolder* item_holder) const override {
    return item_holder
               ? item_holder->height() +
                     item_holder->GetMargin(list::FrameDirection::kTop) +
                     item_holder->GetMargin(list::FrameDirection::kBottom) +
                     item_holder->top_inset()
               : 0.f;
  }

  float GetDecoratedMeasurementInOther(
      const ItemHolder* item_holder) const override {
    return item_holder
               ? item_holder->width() +
                     item_holder->GetMargin(list::FrameDirection::kLeft) +
                     item_holder->GetMargin(list::FrameDirection::kRight)
               : 0.f;
  }

  float GetDecoratedStart(const ItemHolder* item_holder) const override {
    return item_holder
               ? item_holder->top() -
                     item_holder->GetMargin(list::FrameDirection::kTop) -
                     item_holder->top_inset()
               : 0.f;
  }

  float GetDecoratedEnd(const ItemHolder* item_holder) const override {
    // Note: GetHeight() includes the border-width in vertical direction
    return item_holder
               ? item_holder->top() + item_holder->height() +
                     // item_holder->GetBorder(LazyDirection::kBottom) +
                     item_holder->GetMargin(list::FrameDirection::kBottom)
               : 0.f;
  }

  float GetStart(const ItemHolder* item_holder) const override {
    return item_holder ? item_holder->top() -
                             item_holder->GetMargin(list::FrameDirection::kTop)
                       : 0.f;
  }

  float GetItemHolderCrossMargin(const ItemHolder* item_holder) const override {
    return item_holder ? item_holder->GetMargin(list::FrameDirection::kLeft)
                       : 0.f;
  }

  float GetItemHolderMainMargin(const ItemHolder* item_holder) const override {
    return item_holder ? item_holder->GetMargin(list::FrameDirection::kTop)
                       : 0.f;
  }
};

/**
 * HorizontalOrientationHelper
 */
class HorizontalOrientationHelper : public ListOrientationHelper {
 public:
  explicit HorizontalOrientationHelper(ListLayoutManager* list_layout_manager)
      : ListOrientationHelper(list_layout_manager) {}

  ~HorizontalOrientationHelper() override = default;

  bool IsVertical() const override { return false; }

  float GetStartAfterPadding() const override {
    return list_layout_manager_ ? list_layout_manager_->GetPaddingLeft() : 0.f;
  }

  float GetEndAfterPadding() const override {
    return list_layout_manager_ ? list_layout_manager_->GetWidth() -
                                      list_layout_manager_->GetPaddingRight()
                                : 0.f;
  }

  float GetStartAfterPaddingInOther() const override {
    return list_layout_manager_ ? list_layout_manager_->GetPaddingTop() : 0.f;
  }

  float GetMeasurement() const override {
    return list_layout_manager_ ? list_layout_manager_->GetWidth() : 0.f;
  }

  float GetMeasurementInOther() const override {
    return list_layout_manager_ ? list_layout_manager_->GetHeight() : 0.f;
  }

  float GetMeasurementInOtherWithoutPadding() const override {
    return list_layout_manager_->GetHeight() -
           list_layout_manager_->GetPaddingTop() -
           list_layout_manager_->GetPaddingBottom();
  }

  float GetEndPadding() const override {
    return list_layout_manager_ ? list_layout_manager_->GetPaddingRight() : 0.f;
  }

  float GetDecoratedMeasurement(const ItemHolder* item_holder) const override {
    return item_holder
               ? item_holder->width() +
                     item_holder->GetMargin(list::FrameDirection::kLeft) +
                     item_holder->GetMargin(list::FrameDirection::kRight) +
                     item_holder->top_inset()
               : 0.f;
  }

  float GetDecoratedMeasurementInOther(
      const ItemHolder* item_holder) const override {
    return item_holder
               ? item_holder->height() +
                     item_holder->GetMargin(list::FrameDirection::kTop) +
                     item_holder->GetMargin(list::FrameDirection::kBottom)
               : 0.f;
  }

  float GetDecoratedStart(const ItemHolder* item_holder) const override {
    return item_holder ? item_holder->left() -
                             item_holder->GetMargin(list::FrameDirection::kLeft)
                       : 0.f;
  }

  float GetDecoratedEnd(const ItemHolder* item_holder) const override {
    // Note: GetWidth() includes the border-width in horizontal direction
    return item_holder
               ? item_holder->left() + item_holder->width() +
                     item_holder->GetMargin(list::FrameDirection::kRight)
               : 0.f;
  }

  float GetItemHolderCrossMargin(const ItemHolder* item_holder) const override {
    return item_holder ? item_holder->GetMargin(list::FrameDirection::kTop)
                       : 0.f;
  }

  float GetItemHolderMainMargin(const ItemHolder* item_holder) const override {
    return item_holder ? item_holder->GetMargin(list::FrameDirection::kLeft)
                       : 0.f;
  }

  float GetStart(const ItemHolder* item_holder) const override {
    return item_holder ? item_holder->left() -
                             item_holder->GetMargin(list::FrameDirection::kLeft)
                       : 0.f;
  }
};

/**
 * ListOrientationHelper
 */
ListOrientationHelper::ListOrientationHelper(
    ListLayoutManager* list_layout_manager)
    : list_layout_manager_(list_layout_manager) {}

ListOrientationHelper::~ListOrientationHelper() = default;

std::unique_ptr<ListOrientationHelper>
ListOrientationHelper::CreateListOrientationHelper(
    ListLayoutManager* list_layout_manager, list::Orientation orientation) {
  if (orientation == list::Orientation::kHorizontal) {
    return std::make_unique<HorizontalOrientationHelper>(list_layout_manager);
  }
  return std::make_unique<VerticalOrientationHelper>(list_layout_manager);
}

}  // namespace tasm
}  // namespace lynx
