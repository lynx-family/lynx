// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/selection_popup_view.h"

#include <algorithm>
#include <memory>

#include "clay/ui/component/base_view.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/text/internal_text_view.h"
#include "clay/ui/component/text/raw_text_view.h"
#include "clay/ui/component/text/text_view.h"
#include "clay/ui/rendering/render_container.h"
#include "clay/ui/shadow/raw_text_shadow_node.h"

namespace clay {

namespace {

constexpr double kDefaultBorderWidth = 1.0;
constexpr double kDefaultRadius = 10.0;
constexpr double kDefaultEndPadding = 14.5;
constexpr double kDefaultMidPadding = 9.5;
constexpr double kDefaultAbovePadding = 20.0;
constexpr double kDefaultFontSize = 15.0;
constexpr double kPopupContentDistance = 8.0;

const Size kDefaultMenuItemSize = Size(110, 25);

}  // namespace

SelectionPopupView::SelectionPopupView(PageView* page_view)
    : WithTypeInfo(-1, "popup_view", std::make_unique<RenderContainer>(),
                   page_view) {}

SelectionPopupView::~SelectionPopupView() {
  for (auto* child : children_) {
    delete child;
  }
}

void SelectionPopupView::BuildSelectionPopup(
    const std::vector<ActionType>& types) {
  FML_DCHECK(this->child_count() == 0);
  this->SetBorderRadius(FromLogical(kDefaultRadius));
  this->SetBorderColor(Color::kBlack(), Color::kBlack(), Color::kBlack(),
                       Color::kBlack());
  auto border_width = FromLogical(kDefaultBorderWidth);
  this->SetBorderWidth(border_width, border_width, border_width, border_width);
  this->SetBorderStyle(BorderStyleType::kSolid, BorderStyleType::kSolid,
                       BorderStyleType::kSolid, BorderStyleType::kSolid);
  this->SetBackgroundColor(Color::kWhite());
  float menu_width = 0;
  float menu_height = 0;
  for (auto type : types) {
    if (type == ActionType::kCopy) {
      auto text_view = CreateTextViewByText("Copy", 0, 0, MenuIndex::kLeft);
      menu_width += text_view->Width();
      menu_height = std::max(menu_height, text_view->Height());
      text_view->AddTapUpListener([this](const PointerEvent&) {
        if (handle_copy_) {
          handle_copy_();
        }
      });
      this->AddChild(text_view);
    } else if (type == ActionType::kPaste) {
      // TODO(wangyanyi) text componet dont need paste and cut function, but
      // these function needed by editable view
    } else if (type == ActionType::kCut) {
    } else if (type == ActionType::kSelectAll) {
      auto text_view =
          CreateTextViewByText("SelectAll", menu_width, 0, MenuIndex::kRight);
      menu_width += text_view->Width();
      menu_height = std::max(menu_height, text_view->Height());
      text_view->AddTapUpListener([this](const PointerEvent&) {
        if (select_all_) {
          select_all_();
        }
      });
      this->AddChild(text_view);
    }
  }
  FloatPoint offset =
      GetPositionForChild(FloatSize(bounds_width_, bounds_height_),
                          FloatSize(menu_width + kPopupContentDistance,
                                    menu_height + kPopupContentDistance));
  this->SetX(offset.x());
  this->SetY(offset.y());
  origin_top_ = Top();
  origin_left_ = Left();
  this->SetContentWidth(menu_width);
  this->SetContentHeight(menu_height);
}

InternalTextView* SelectionPopupView::CreateTextViewByText(
    const std::string& text, int left, int top, MenuIndex index) const {
  auto text_view = new InternalTextView(
      -1, "text_select_menu_item", std::make_unique<RenderText>(), page_view());
  text_view->SetX(left);
  text_view->SetY(top);
  text_view->SetText(text);
  text_view->SetFontSize(FromLogical(kDefaultFontSize));
  MeasureResult result;
  text_view->Measure(
      {FromLogical(kDefaultMenuItemSize.width()), TextMeasureMode::kAtMost,
       FromLogical(kDefaultMenuItemSize.height()), TextMeasureMode::kAtMost},
      result);
  if (index == MenuIndex::kLeft) {
    text_view->SetPaddings(kDefaultEndPadding, kDefaultAbovePadding,
                           kDefaultMidPadding, kDefaultAbovePadding);
  } else if (index == MenuIndex::kMid) {
    text_view->SetPaddings(kDefaultMidPadding, kDefaultAbovePadding,
                           kDefaultMidPadding, kDefaultAbovePadding);
  } else if (index == MenuIndex::kRight) {
    text_view->SetPaddings(kDefaultMidPadding, kDefaultAbovePadding,
                           kDefaultEndPadding, kDefaultAbovePadding);
  }
  text_view->SetContentWidth(result.width);
  text_view->SetContentHeight(result.height);
  return text_view;
}

FloatPoint SelectionPopupView::GetPositionForChild(FloatSize size,
                                                   FloatSize child_size) {
  if (anchor_offset_.size() == 0) {
    return FloatPoint(0, 0);
  }
  FloatPoint anchor_above = anchor_offset_[0];
  FloatPoint anchor_below = anchor_offset_[1];
  bool fits_above =
      anchor_above.y() >= child_size.height() + kPopupContentDistance;
  bool fits_below = anchor_below.y() <=
                    size.height() - child_size.height() - kPopupContentDistance;
  FloatPoint anchor;
  if (!fits_above && !fits_below) {
    anchor = FloatPoint(size.width() / 2, size.height() / 2);
  } else {
    anchor = fits_above ? anchor_above : anchor_below;
  }
  return FloatPoint(
      CenterOn(anchor.x(), child_size.width(), size.width()),
      fits_above ? std::max<float>(0.0, anchor.y() - child_size.height() -
                                            FromLogical(kPopupContentDistance))
                 : anchor.y() + FromLogical(kPopupContentDistance));
}

double SelectionPopupView::CenterOn(double position, double width, double max) {
  if (position - width / 2.0 < 0.0) {
    return 0.0;
  }
  if (position + width / 2.0 > max) {
    return max - width;
  }
  return position - width / 2.0;
}

void SelectionPopupView::UpdatePosWithScroll(FloatPoint scroll_offset,
                                             FloatRect bounding_rect) {
  SetY(scroll_offset.y() < 0
           ? std::max(scroll_offset.y() + origin_top_,
                      bounding_rect.top() - height_ -
                          FromLogical(kPopupContentDistance))
           : std::min(scroll_offset.y() + origin_top_, bounding_rect.bottom()));
  SetX(scroll_offset.x() + origin_left_);
}

}  // namespace clay
