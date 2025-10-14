// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/list/focus_list_item_view_holder.h"

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <utility>

#include "clay/ui/component/view.h"

namespace clay {
namespace {

constexpr float kBoxSize = 50.f;
constexpr float kPadding = 5.f;
const std::array<Color, 4> kColors = {
    Color::ARGBColor(0xFF, 0xFF, 0x00, 0x00),
    Color::ARGBColor(0xFF, 0x00, 0xFF, 0x00),
    Color::ARGBColor(0xFF, 0x00, 0x00, 0xFF),
    Color::ARGBColor(0xFF, 0x00, 0x00, 0x00),
};

}  // namespace

FocusListItemViewHolder::FocusListItemViewHolder(PageView* page_view,
                                                 const IsFocusable& focusable,
                                                 int column, int row,
                                                 float default_length)
    : page_view_(page_view),
      focusable_(focusable),
      column_(column),
      row_(row),
      default_length_(default_length) {}

FocusListItemViewHolder::~FocusListItemViewHolder() = default;

BaseView* FocusListItemViewHolder::InitViews() {
  container_view_ = std::make_unique<View>(-1, page_view_);
  BordersData scroll_border_data;
  scroll_border_data.width_top_ = 5.f;
  scroll_border_data.width_bottom_ = 5.f;
  scroll_border_data.width_right_ = 5.f;
  scroll_border_data.width_left_ = 5.f;
  scroll_border_data.color_top_ = 0xFFFF0000;
  scroll_border_data.color_bottom_ = 0xFFFF0000;
  scroll_border_data.color_right_ = 0xFFFF0000;
  scroll_border_data.color_left_ = 0xFFFF0000;
  container_view_->SetBorder(scroll_border_data);
  container_view_->SetPaddings(kPadding, kPadding, kPadding, kPadding);
  boxes_.resize(column_ * row_);
  for (auto& box : boxes_) {
    box = std::make_unique<View>(-1, page_view_);
    box->SetWidth(kBoxSize);
    box->SetHeight(kBoxSize);
    box->SetFocusable(true);
    container_view_->AddChild(box.get());
  }

  SetView(container_view_.get());
  return container_view_.get();
}

void FocusListItemViewHolder::ReleaseViews() {
  SetView(nullptr);
  for (auto& box : boxes_) {
    container_view_->RemoveChild(box.get());
    box = nullptr;
  }
  container_view_ = nullptr;
}

void FocusListItemViewHolder::OnUpdatePosition() {
  for (const auto& box : boxes_) {
    box->SetBackgroundColor(kColors[GetPosition() % kColors.size()]);
    box->SetFocusable(focusable_(GetPosition()));
  }
  if (boxes_.size() == 0) {
    container_view_->SetFocusable(focusable_(GetPosition()));
  }
}

MeasureResult FocusListItemViewHolder::Measure(
    const MeasureConstraint& constraint) {
  MeasureResult res;

  {
    BaseView::LayoutIgnoreHelper helper(container_view_.get());
    if (constraint.width_mode != MeasureMode::kIndefinite &&
        constraint.width.has_value()) {
      container_view_->SetWidth(*constraint.width);
    } else if (constraint.width_mode == MeasureMode::kIndefinite) {
      container_view_->SetWidth(default_length_);
    }

    if (constraint.height_mode != MeasureMode::kIndefinite &&
        constraint.height.has_value()) {
      container_view_->SetHeight(*constraint.height);
    } else if (constraint.height_mode == MeasureMode::kIndefinite) {
      container_view_->SetHeight(default_length_);
    }
  }

  const float width = container_view_->Width();
  const float height = container_view_->Height();

  res.width = width;
  res.height = height;

  const float x_step = width / column_;
  const float y_step = height / row_;

  for (int i = 0; i < row_; ++i) {
    for (int j = 0; j < column_; ++j) {
      boxes_[i * column_ + j]->SetX(j * x_step);
      boxes_[i * column_ + j]->SetY(i * y_step);
    }
  }

  // Clear the dirty bit.
  container_view_->Layout();

  return res;
}

}  // namespace clay
