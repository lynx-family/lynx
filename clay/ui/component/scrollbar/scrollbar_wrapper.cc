// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/scrollbar/scrollbar_wrapper.h"

#include <limits>
#include <string>
#include <unordered_set>
#include <utility>

#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/component/css_property.h"
#include "clay/ui/rendering/render_container.h"

namespace clay {

const double kDefaultScrollbarWidth = 12;  // in logical pixels

namespace {

const std::unordered_set<KeywordID> kProxyAttributes = {
    KeywordID::kScrollBarThumbWidth,       KeywordID::kScrollBarThumbMinLength,
    KeywordID::kScrollBarThumbRadius,      KeywordID::kScrollBarThumbColor,
    KeywordID::kScrollBarThumbActiveColor, KeywordID::kScrollBarThumbHoverColor,
    KeywordID::kScrollBarTrackColor,       KeywordID::kScrollBarAutoHide,
    KeywordID::kScrollBarAutoHideDelay,
};

inline bool NearlyZeroFloat(float value) {
  return std::abs(value) < std::numeric_limits<float>::epsilon();
}

inline bool NearlyEqual(float left, float right) {
  return NearlyZeroFloat(left - right);
}

}  // namespace

ScrollbarWrapper::ScrollbarWrapper(int id, ScrollDirection direction,
                                   std::string tag, PageView* page_view)
    : BaseView(id, std::move(tag), std::make_unique<RenderContainer>(),
               page_view),
      scrollbar_width_(FromLogical(kDefaultScrollbarWidth)) {
  SetOverflow(CSSProperty::OVERFLOW_HIDDEN);
  SetRepaintBoundary(true);
  scrollbar_ = new ScrollbarView(page_view);
  scrollbar_->SetDelegate(this);
  scrollbar_->SetScrollDirection(direction);
}

ScrollbarWrapper::~ScrollbarWrapper() = default;

void ScrollbarWrapper::SetScrollbarEnabled(bool enabled) {
  if (scrollbar_enabled_ != enabled) {
    scrollbar_enabled_ = enabled;
    if (scrollbar_enabled_) {
      UpdateScrollbarLengths();
    }
    UpdateScrollbarBounds();
  }
}

void ScrollbarWrapper::AddChild(BaseView* child) {
  view_->BaseView::AddChild(child);
}

void ScrollbarWrapper::AddChild(BaseView* child, int index) {
  view_->AddChild(child, index);
}

void ScrollbarWrapper::RemoveChild(BaseView* child) {
  view_->RemoveChild(child);
}

void ScrollbarWrapper::OnDestroy() {
  if (scrollbar_ && !scrollbar_->Parent()) {
    DestroyChildrenRecursively(scrollbar_);
  }
  scrollbar_ = nullptr;
}

void ScrollbarWrapper::SetAttribute(const char* attr_c,
                                    const clay::Value& value) {
  auto kw = GetKeywordID(attr_c);
  if (kProxyAttributes.find(kw) != kProxyAttributes.end()) {
    scrollbar_->SetAttribute(attr_c, value);
  } else if (kw == KeywordID::kScrollBarWidth) {
    scrollbar_width_ = FromLogical(attribute_utils::GetNum(value));
    UpdateScrollbarBounds();
  } else if (kw == KeywordID::kScrollBarEnable ||
             kw == KeywordID::kEnableScrollbar) {
    // keyword 'scroll-bar-enable' is deprecated. use 'enable-scrollbar'
    // instead.
    SetScrollbarEnabled(attribute_utils::GetBool(value));
  } else {
    BaseView::SetAttribute(attr_c, value);
  }
}

void ScrollbarWrapper::SetWidth(float width) {
  BaseView::SetWidth(width);
  view_->SetWidth(width);
}
void ScrollbarWrapper::SetHeight(float height) {
  BaseView::SetHeight(height);
  view_->SetHeight(height);
}
void ScrollbarWrapper::SetBound(float left, float top, float width,
                                float height) {
  BaseView::SetBound(left, top, width, height);
  view_->SetBound(0, 0, width, height);
}

void ScrollbarWrapper::SetPaddings(float padding_left, float padding_top,
                                   float padding_right, float padding_bottom) {
  view_->SetPaddings(padding_left, padding_top, padding_right, padding_bottom);
}

void ScrollbarWrapper::SetBorder(const BordersData& data) {
  view_->SetBorder(data);
}

void ScrollbarWrapper::SetBorderStyle(BorderStyleType left, BorderStyleType top,
                                      BorderStyleType right,
                                      BorderStyleType bottom) {
  view_->SetBorderStyle(left, top, right, bottom);
}

void ScrollbarWrapper::SetBorderStyle(Side side, int style) {
  view_->SetBorderStyle(side, style);
}

void ScrollbarWrapper::SetBorderWidth(float left_width, float top_width,
                                      float right_width, float bottom_width) {
  view_->SetBorderWidth(left_width, top_width, right_width, bottom_width);
}

void ScrollbarWrapper::SetBorderWidth(Side side, float width) {
  view_->SetBorderWidth(side, width);
}

void ScrollbarWrapper::SetBorderColor(unsigned int left_color,
                                      unsigned int top_color,
                                      unsigned int right_color,
                                      unsigned int bottom_color) {
  view_->SetBorderColor(left_color, top_color, right_color, bottom_color);
}

void ScrollbarWrapper::SetBorderColor(Side side, uint32_t color) {
  view_->SetBorderColor(side, color);
}

void ScrollbarWrapper::SetBorderRadius(const FloatSize& left_top,
                                       const FloatSize& right_top,
                                       const FloatSize& right_bottom,
                                       const FloatSize& left_bottom) {
  view_->SetBorderRadius(left_top, right_top, right_bottom, left_bottom);
}

void ScrollbarWrapper::SetBorderRadius(float radius_all) {
  view_->SetBorderRadius(radius_all);
}

void ScrollbarWrapper::SetBackground(const BackgroundData& background) {
  view_->SetBackground(background);
}
void ScrollbarWrapper::SetBackgroundColor(const Color& color) {
  view_->SetBackgroundColor(color);
}

void ScrollbarWrapper::SetBackgroundImage(const clay::Value::Array& array) {
  view_->SetBackgroundImage(array);
}

void ScrollbarWrapper::SetBackgroundClip(const clay::Value::Array& array) {
  view_->SetBackgroundClip(array);
}

void ScrollbarWrapper::SetBackgroundOrigin(const clay::Value::Array& array) {
  view_->SetBackgroundOrigin(array);
}

void ScrollbarWrapper::SetBackgroundPosition(
    const std::vector<BackgroundPosition>& positions) {
  view_->SetBackgroundPosition(positions);
}

void ScrollbarWrapper::SetBackgroundRepeat(const clay::Value::Array& array) {
  view_->SetBackgroundRepeat(array);
}

void ScrollbarWrapper::SetBackgroundSize(
    const std::vector<BackgroundSize>& sizes) {
  view_->SetBackgroundSize(sizes);
}

void ScrollbarWrapper::AddEventCallback(const char* event) {
  view_->AddEventCallback(event);
}

bool ScrollbarWrapper::HasEvent(const std::string& event) const {
  return view_->HasEvent(event);
}

void ScrollbarWrapper::OnContentSizeChanged(const FloatRect& old_rect,
                                            const FloatRect& new_rect) {
  BaseView::OnContentSizeChanged(old_rect, new_rect);
  view_->SetBound(0, 0, new_rect.width(), new_rect.height());
  UpdateScrollbarBounds();
}

void ScrollbarWrapper::OnViewportMetricsUpdated(int physical_width,
                                                int physical_height,
                                                float device_pixel_ratio) {
  UpdateScrollbarBounds();
}

void ScrollbarWrapper::UpdateScrollbarBounds() {
  if (!scrollbar_enabled_) {
    SetScrollbarVisible(false);
    return;
  }
  const float secondary_length =
      OrientationHelper().GetSecondaryContentLength(*this);
  const float self_length = OrientationHelper().GetContentLength(*this);

  if (scrollbar_width_ >= secondary_length) {
    SetScrollbarVisible(false);
    return;
  }

  if (scrollbar_visible_) {
    OrientationHelper().SetBound(scrollbar_, 0.f,
                                 secondary_length - scrollbar_width_,
                                 self_length, scrollbar_width_);
    WillUpdateScrollbar();
    UpdateScrollbarLengths();
    UpdateScrollbarPosition();
  }
}

void ScrollbarWrapper::UpdateScrollbarLengths() {
  FML_DCHECK(scrollbar_enabled_);
  const float self_length = OrientationHelper().GetContentLength(*this);
  const float overflow_length = GetTotalLength();
  scrollbar_->SetVisibleLength(self_length);
  scrollbar_->SetTotalLength(overflow_length);
  SetScrollbarVisible(overflow_length > self_length);
}

void ScrollbarWrapper::UpdateScrollbarPosition() {
  const float overflow_length = GetTotalLength();
  const float scroll_offset = GetScrollbarScrollOffset();
  const float self_length = OrientationHelper().GetContentLength(*this);

  if (NearlyZeroFloat(overflow_length) || NearlyZeroFloat(self_length) ||
      NearlyEqual(overflow_length, self_length)) {
    scrollbar_->SetPosition(0.f);
  } else {
    scrollbar_->SetPosition(scroll_offset / (overflow_length - self_length));
  }
}

void ScrollbarWrapper::SetScrollbarVisible(bool visible) {
  if (scrollbar_visible_ == visible) {
    return;
  }

  scrollbar_visible_ = visible;
  if (visible && !scrollbar_->Parent()) {
    BaseView::AddChild(scrollbar_, 1);
    UpdateScrollbarBounds();
  } else if (!visible && scrollbar_->Parent()) {
    BaseView::RemoveChild(scrollbar_);
  }
}

#ifndef NDEBUG
std::string ScrollbarWrapper::ToString() const {
  std::stringstream ss;
  ss << BaseView::ToString();
  if (scrollbar_enabled_) {
    ss << " scrollbar_width_=" << static_cast<uint8_t>(scrollbar_width_);
  }
  return ss.str();
}
#endif

}  // namespace clay
