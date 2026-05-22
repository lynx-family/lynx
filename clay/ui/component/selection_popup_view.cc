// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/selection_popup_view.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "clay/gfx/style/color.h"
#include "clay/gfx/style/shadow.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/text/internal_text_view.h"
#include "clay/ui/component/text/raw_text_view.h"
#include "clay/ui/component/text/text_view.h"
#include "clay/ui/rendering/render_container.h"
#include "clay/ui/shadow/raw_text_shadow_node.h"

namespace clay {

namespace {

constexpr double kDefaultBorderWidth = 0.0;
constexpr double kDefaultRadius = 12.0;
constexpr double kDefaultLeftPadding = 10.0;
constexpr double kDefaultMidPadding = 6.0;
constexpr double kDefaultRightPadding = 10.0;
constexpr double kDefaultTopPadding = 6.0;
constexpr double kDefaultBottomPadding = 6.0;
constexpr double kDefaultFontSize = 14.0;
constexpr double kDefaultLineHeight = 22.0;
constexpr double kPopupContentDistance = 8.0;

const Size kDefaultMenuItemSize = Size(28, 22);

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
  FloatSize radius(FromLogical(kDefaultRadius), FromLogical(kDefaultRadius));
  this->SetBorderRadius(radius, radius, radius, radius);
  this->SetBorderColor({Side::kAll}, {Color::kBlack()});
  float border_width = kDefaultBorderWidth;
  this->SetBorderWidth({Side::kAll}, {border_width});
  this->SetBorderStyle({Side::kAll}, {BorderStyleType::kSolid});
  this->SetBackgroundColor(Color::kWhite());
  float menu_width = 0;
  float menu_height = 0;
  for (auto type : types) {
    if (type == ActionType::kCopy) {
      auto text_view = CreateTextViewByText("\xE5\xA4\x8D\xE5\x88\xB6", 0, 0,
                                            MenuIndex::kLeft);
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
      auto text_view = CreateTextViewByText("\xE5\x85\xA8\xE9\x80\x89",
                                            menu_width, 0, MenuIndex::kRight);
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
                          FloatSize(menu_width, menu_height));
  this->SetX(offset.x());
  this->SetY(offset.y());
  origin_top_ = Top();
  origin_left_ = Left();
  this->SetContentWidth(menu_width);
  this->SetContentHeight(menu_height);
  auto shadows = CreateShadow();
  render_object()->SetShadows(std::move(shadows));
}

std::vector<Shadow> SelectionPopupView::CreateShadow() {
  std::vector<Shadow> shadows;
  Shadow shadow1;
  shadow1.inset = false;
  shadow1.offset_x = 0;
  shadow1.offset_y = 4;
  shadow1.blur_radius = 20;
  shadow1.spread_radius = 0;
  shadow1.color = Color::ARGBColor(round(0.08 * 255), 0, 0, 0);
  shadows.emplace_back(shadow1);
  Shadow shadow2;
  shadow2.inset = false;
  shadow2.offset_x = 0;
  shadow2.offset_y = 2;
  shadow2.blur_radius = 4;
  shadow2.spread_radius = 0;
  shadow2.color = Color::ARGBColor(round(0.06 * 255), 0, 0, 0);
  shadows.emplace_back(shadow2);
  return shadows;
}

InternalTextView* SelectionPopupView::CreateTextViewByText(
    const std::string& text, int left, int top, MenuIndex index) const {
  auto text_view = new InternalTextView(
      -1, "text_select_menu_item", std::make_unique<RenderText>(), page_view());
  text_view->SetX(left);
  text_view->SetY(top);
  text_view->SetText(text);
  text_view->SetFontSize(FromLogical(kDefaultFontSize));
  text_view->SetLineHeight(FromLogical(kDefaultLineHeight));
  if (index == MenuIndex::kLeft) {
    text_view->SetPaddings(
        FromLogical(kDefaultLeftPadding), FromLogical(kDefaultTopPadding),
        FromLogical(kDefaultMidPadding), FromLogical(kDefaultBottomPadding));
  } else if (index == MenuIndex::kMid) {
    text_view->SetPaddings(
        FromLogical(kDefaultMidPadding), FromLogical(kDefaultTopPadding),
        FromLogical(kDefaultMidPadding), FromLogical(kDefaultBottomPadding));
  } else if (index == MenuIndex::kRight) {
    text_view->SetPaddings(
        FromLogical(kDefaultMidPadding), FromLogical(kDefaultTopPadding),
        FromLogical(kDefaultRightPadding), FromLogical(kDefaultBottomPadding));
  }
  MeasureResult result;
  text_view->Measure(
      {FromLogical(kDefaultMenuItemSize.width()), TextMeasureMode::kAtMost,
       FromLogical(kDefaultMenuItemSize.height()), TextMeasureMode::kAtMost},
      result);
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
                      bounding_rect.top() - Height() -
                          FromLogical(kPopupContentDistance))
           : std::min(scroll_offset.y() + origin_top_, bounding_rect.bottom()));
  SetX(scroll_offset.x() + origin_left_);
}

}  // namespace clay
