// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/scroll_wrapper.h"

#include <math.h>

#include <cmath>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "clay/ui/lynx_module/lynx_ui_method_registrar.h"
#include "clay/ui/lynx_module/type_utils.h"

namespace clay {

namespace {

// Attributes that should be forwarded to the scroll view.
const std::unordered_set<KeywordID> kProxyAttributes = {
    KeywordID::kScrollX,
    KeywordID::kScrollY,
    KeywordID::kLowerThreshold,
    KeywordID::kUpperThreshold,
    KeywordID::kEnableScroll,
    KeywordID::kEnableNestedScroll,
    KeywordID::kScrollMonitorTag,
    KeywordID::kScrollTop,
    KeywordID::kScrollLeft,
    KeywordID::kScrollToIndex,
    KeywordID::kInitialScrollOffset,
    KeywordID::kInitialScrollIndex,
    KeywordID::kScrollOrientation,
    KeywordID::kScrollForwardMode,
    KeywordID::kScrollBackwardMode,
    KeywordID::kPrevScrollable,
    KeywordID::kNextScrollable,
    KeywordID::kBounce,
    KeywordID::kBounces,
    KeywordID::kScrollToId,
    KeywordID::kScrollEventThrottle};
constexpr char kScrollWrapperTag[] = "scroll-view";

LYNX_UI_METHOD_BEGIN(ScrollWrapper) {
  LYNX_UI_METHOD(ScrollWrapper, scrollTo);
  LYNX_UI_METHOD(ScrollWrapper, autoScroll);
  LYNX_UI_METHOD(ScrollWrapper, getScrollInfo);
}
LYNX_UI_METHOD_END(ScrollWrapper);

constexpr char kArgOffset[] = "offset";
constexpr char kArgSmooth[] = "smooth";
constexpr char kArgIndex[] = "index";
constexpr char kArgStart[] = "start";
constexpr char kArgRate[] = "rate";
const std::vector<std::string> kScrollToArgs{kArgSmooth, kArgOffset, kArgIndex};
const std::vector<std::string> kAutoScrollArgs{kArgStart, kArgRate};

}  // namespace

ScrollWrapper::ScrollWrapper(int id, PageView* page_view)
    : ScrollWrapper(id, ScrollDirection::kVertical, page_view) {}

ScrollWrapper::ScrollWrapper(int id, ScrollDirection direction,
                             PageView* page_view)
    : WithTypeInfo(id, direction, kScrollWrapperTag, page_view) {
  view_ = new ScrollView(-1, id, direction, page_view);
  // Set the real element id to RenderScroll.
  view_->render_object()->SetID(id);
  view_->SetOverflow(CSSProperty::OVERFLOW_HIDDEN);
  view_->SetRepaintBoundary(true);
  GetScrollView()->SetDelegate(this);
  GetScrollView()->AddScrollListener(this);
  BaseView::AddChild(view_, 0);
}

ScrollWrapper::~ScrollWrapper() = default;

void ScrollWrapper::OnDestroy() {
  ScrollbarWrapper::OnDestroy();
  GetScrollView()->SetDelegate(nullptr);
  GetScrollView()->RemoveScrollListener(this);
}

BaseView* ScrollWrapper::GetTopViewToAcceptEvent(const FloatPoint& position,
                                                 FloatPoint* relative_position,
                                                 int platform_try_hit_id) {
  BaseView* view = BaseView::GetTopViewToAcceptEvent(
      position, relative_position, platform_try_hit_id);
  if (view == view_) {
    return this;
  }
  return view;
}

void ScrollWrapper::scrollTo(const LynxModuleValues& args) {
  bool smooth = false;
  float offset = 0;
  int index = -1;
  if (CastNamedLynxModuleArgs(kScrollToArgs, args, smooth, offset, index)) {
    if (isnan(offset) || isinf(offset)) {
      FML_DLOG(ERROR) << "Cannot scrollTo nan or infinite!";
      return;
    }
    GetScrollView()->ScrollTo(smooth, FromLogical(offset), index);
  }
}

void ScrollWrapper::autoScroll(const LynxModuleValues& args) {
  bool start = false;
  float rate = 0;
  if (CastNamedLynxModuleArgs(kAutoScrollArgs, args, start, rate)) {
    if (isnan(rate) || isinf(rate)) {
      FML_DLOG(ERROR) << "rate cannot be nan or infinite!";
      return;
    }
    GetScrollView()->AutoScroll(start, FromLogical(rate));
  }
}

void ScrollWrapper::getScrollInfo(const LynxModuleValues& args,
                                  const LynxUIMethodCallback& callback) {
  if (callback) {
    FloatPoint offset = view_->GetScrollOffset();
    FloatSize zoomed_content = page_view_->ConvertTo<kPixelTypeLogical>(
        FloatSize(view_->ContentWidth(), view_->ContentHeight()));
    FloatPoint zoomed_offset = page_view_->ConvertTo<kPixelTypeLogical>(offset);
    clay::Value::Map map;
    map.emplace("scrollTop", zoomed_offset.y());
    map.emplace("scrollLeft", zoomed_offset.x());
    map.emplace("scrollHeight", zoomed_content.height());
    map.emplace("scrollWidth", zoomed_content.width());
    map.emplace("isDragging",
                static_cast<ScrollView*>(view_)->GetScrollStatus() ==
                    ScrollView::ScrollStatus::kDragging);
    callback(LynxUIMethodResult::kSuccess, clay::Value(std::move(map)));
  }
}

void ScrollWrapper::SetAttribute(const char* attr_c, const clay::Value& value) {
  auto kw = GetKeywordID(attr_c);
  if (kProxyAttributes.find(kw) != kProxyAttributes.end()) {
    view_->SetAttribute(attr_c, value);
    if (kw == KeywordID::kScrollX || kw == KeywordID::kScrollY) {
      scrollbar_->SetScrollDirection(
          static_cast<ScrollView*>(view_)->GetScrollDirection());
    } else if (kw == KeywordID::kScrollOrientation) {
      std::string orientation;
      attribute_utils::TryGetString(value, orientation);
      if (orientation == "vertical") {
        scrollbar_->SetScrollDirection(ScrollDirection::kVertical);
      } else if (orientation == "horizontal") {
        scrollbar_->SetScrollDirection(ScrollDirection::kHorizontal);
      }
    }
  } else {
    ScrollbarWrapper::SetAttribute(attr_c, value);
  }
}

void ScrollWrapper::SetDirection(int type) {
  view_->SetDirection(type);
  scrollbar_->SetDirection(type);
}

void ScrollWrapper::SetOverflow(int overflow) {
  BaseView::SetOverflow(overflow);
  view_->SetOverflow(overflow);
}

void ScrollWrapper::WillUpdateScrollbar() { view_->OnLayoutUpdated(); }

float ScrollWrapper::GetScrollbarScrollOffset() {
  return OrientationHelper().GetLocation(view_->GetScrollOffset());
}

float ScrollWrapper::GetTotalLength() {
  return OrientationHelper().GetLength(GetScrollView()->OverflowRect());
}

void ScrollWrapper::OnScrollViewChildAdded() {
  view_->OnLayoutUpdated();
  if (scrollbar_enabled_) {
    UpdateScrollbarLengths();
    UpdateScrollbarPosition();
  }
}

void ScrollWrapper::OnScrollViewChildRemoved() {
  view_->OnLayoutUpdated();
  if (scrollbar_enabled_) {
    UpdateScrollbarLengths();
    UpdateScrollbarPosition();
  }
}

void ScrollWrapper::OnScrollViewChildUpdated() {
  view_->OnLayoutUpdated();
  if (scrollbar_enabled_) {
    UpdateScrollbarLengths();
    UpdateScrollbarPosition();
  }
}

void ScrollWrapper::OnScrollableScrolled() {
  UpdateScrollbarPosition();
  scrollbar_->NotifyScrollViewScrolled();
}

void ScrollWrapper::OnScrollbarScrolled(float old_position, float new_position,
                                        bool by_interaction, bool smooth) {
  if (by_interaction) {
    const float overflow_length = GetTotalLength();
    const float self_length = OrientationHelper().GetContentLength(*this);

    GetScrollView()->ScrollTo(smooth,
                              (overflow_length - self_length) * new_position);
  }
}

}  // namespace clay
