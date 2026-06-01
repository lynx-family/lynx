// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/scroll_view.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "clay/fml/logging.h"
#include "clay/gfx/animation/viscous_fluid_interpolator.h"
#include "clay/gfx/geometry/float_rect.h"
#include "clay/gfx/geometry/float_size.h"
#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/common/utils/floating_comparison.h"
#include "clay/ui/component/base_image_view.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/component_constants.h"
#include "clay/ui/component/css_property.h"
#include "clay/ui/component/nested_scroll/nested_scrollable.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/view_callback/scroll_event_callback_manager.h"
#include "clay/ui/component/view_context.h"
#include "clay/ui/lynx_module/lynx_ui_method_registrar.h"
#include "clay/ui/rendering/render_box.h"
#include "clay/ui/rendering/render_scroll.h"

namespace clay {
namespace {

constexpr char kScrollViewTag[] = "scroll-view-impl";

constexpr char kArgOffset[] = "offset";
constexpr char kArgSmooth[] = "smooth";
const std::vector<std::string> kScrollToArgs{kArgSmooth, kArgOffset};

NestedScrollMode ParseNestedScrollMode(const std::string& value) {
  if (value == "self-only") {
    return NestedScrollMode::kSelfOnly;
  } else if (value == "self-first") {
    return NestedScrollMode::kSelfFirst;
  } else if (value == "parent-first") {
    return NestedScrollMode::kParentFirst;
  } else {
    FML_DLOG(ERROR) << "invalid nested scroll mode: " << value;
    return NestedScrollMode::kSelfOnly;
  }
}

inline ScrollDirection ParseScrollDirection(const std::string& value) {
  if (value == "horizontal") {
    return ScrollDirection::kHorizontal;
  } else {
    return ScrollDirection::kVertical;
  }
}

inline ScrollEventCallbackManager::ScrollState ConvertScrollState(
    Scrollable::ScrollStatus status) {
  if (status == Scrollable::ScrollStatus::kAnimating ||
      status == Scrollable::ScrollStatus::kFling ||
      status == Scrollable::ScrollStatus::kBounce) {
    return ScrollEventCallbackManager::ScrollState::kAnimating;
  } else if (status == Scrollable::ScrollStatus::kIdle) {
    return ScrollEventCallbackManager::ScrollState::kIdle;
  } else {
    return ScrollEventCallbackManager::ScrollState::kDragging;
  }
}

}  // namespace

ScrollView::ScrollView(int32_t id, PageView* page_view)
    : ScrollView(id, ScrollDirection::kVertical, page_view) {}

ScrollView::ScrollView(int32_t id, ScrollDirection direction,
                       PageView* page_view,
                       std::unique_ptr<RenderScroll> render_scroll)
    : ScrollView(id, id, direction, page_view, std::move(render_scroll)) {}

ScrollView::ScrollView(int32_t id, int32_t callback_id,
                       ScrollDirection direction, PageView* page_view,
                       std::unique_ptr<RenderScroll> render_scroll)
    : WithTypeInfo(id, kScrollViewTag, std::move(render_scroll), page_view,
                   direction, true),
      callback_manager_(
          std::make_unique<ScrollEventCallbackManager>(this, callback_id)),
      callback_id_(callback_id),
      weak_factory_(this) {
  InitScrollView();
}

ScrollView::ScrollView(
    int32_t id, int32_t callback_id, ScrollDirection direction,
    PageView* page_view,
    std::unique_ptr<ScrollEventCallbackManager> callback_manager,
    std::unique_ptr<RenderScroll> render_scroll)
    : WithTypeInfo(id, kScrollViewTag, std::move(render_scroll), page_view,
                   direction, true),
      callback_manager_(std::move(callback_manager)),
      callback_id_(callback_id),
      weak_factory_(this) {
  InitScrollView();
}

void ScrollView::InitScrollView() {
  scroller_ =
      std::make_unique<Scroller>(this, GetAnimationHandler(),
                                 std::make_unique<ViscousFluidInterpolator>());
  SetIsFocusScope();
  GetFocusManager()->SetFirstFocusedNodeExpandRatio(1.f / 3.f);
#if defined(OS_IOS) || defined(OS_OSX)
  // Enable bounce effect by default on iOS and macOS.
  SetOverscrollEnabled(true);
#endif
}

ScrollView::~ScrollView() {}

void ScrollView::AddChild(BaseView* child, int index) {
  if (child->Is<BounceView>()) {
    bounce_view_ = static_cast<BounceView*>(child);
    MarkNeedsLayout();
  }
  BaseView::AddChild(child, index);
  if (delegate_) {
    delegate_->OnScrollViewChildAdded();
  }
  CorrectScrollOffset();
  if (!page_view()->ImageDecodeWithPriority()) {
    auto func = [](BaseView* v) {
      if (v && v->Is<BaseImageView>()) {
        static_cast<BaseImageView*>(v)->TryDecodeImmediately();
      }
    };
    child->VisitChildren(func);
  }
}

void ScrollView::RemoveChild(BaseView* child) {
  if (child->Is<BounceView>()) {
    bounce_view_ = nullptr;
    MarkNeedsLayout();
  }
  BaseView::RemoveChild(child);
  if (delegate_) {
    delegate_->OnScrollViewChildRemoved();
  }
  CorrectScrollOffset();
}

void ScrollView::OnLayout(LayoutContext* context) {
  BaseView::OnLayout(context);
  if (content_size_ != old_content_size_) {
    UpdateScrollOffsetForRTL();
    NotifyContentSizeChanged(content_size_);
    old_content_size_ = content_size_;
  }
  if (pending_scroll_offset_) {
    bool is_vertical = scroll_direction_ == ScrollDirection::kVertical;
    auto render_scroll = static_cast<RenderScroll*>(render_object());
    if ((is_vertical &&
         *pending_scroll_offset_ <= render_scroll->MaxScrollHeight()) ||
        (!is_vertical &&
         *pending_scroll_offset_ <= render_scroll->MaxScrollWidth())) {
      OnScrollUpdate(*pending_scroll_offset_);
      pending_scroll_offset_ = std::nullopt;
    }
  }
  if (pending_scroll_index_ > -1) {
    SetScrollToIndex(pending_scroll_index_);
    pending_scroll_index_ = -1;
  }
  if (!bounce_view_) {
    return;
  }
  auto bounce_direction = bounce_view_->GetDirection();
  if (bounce_direction == Direction::kTop) {
    bounce_view_->SetY(Top() - bounce_view_->Height() -
                       bounce_view_->MarginBottom());
  } else if (bounce_direction == Direction::kLeft) {
    bounce_view_->SetX(Left() - bounce_view_->Width() -
                       bounce_view_->MarginRight());
  } else if (bounce_direction == Direction::kBottom) {
    auto render_box = static_cast<RenderBox*>(render_object());
    auto height = render_box->OverflowRect().MaxY();
    bounce_view_->SetY(height + bounce_view_->MarginTop());
  } else if (bounce_direction == Direction::kRight) {
    auto render_box = static_cast<RenderBox*>(render_object());
    auto width = render_box->OverflowRect().MaxX();
    bounce_view_->SetX(width + bounce_view_->MarginLeft());
  }
}

void ScrollView::CalculateOverFlow() {
  GetRenderScroll()->AddOverflowFromChildren();
  GetRenderScroll()->MarkNeedsPaint();

  content_size_ =
      FloatSize(std::max<float>(GetRenderScroll()->OverflowRect().MaxX(),
                                BaseView::ContentWidth()),
                std::max<float>(GetRenderScroll()->OverflowRect().MaxY(),
                                BaseView::ContentHeight()));
}

void ScrollView::OnLayoutUpdated() {
  CalculateOverFlow();
  CorrectScrollOffset();
}

#ifdef ENABLE_ACCESSIBILITY
int32_t ScrollView::GetA11yScrollChildren() const {
  int32_t valid_count = 0;
  for (auto child : children_) {
    if (child->IsAccessibilityElement()) {
      ++valid_count;
    }
  }
  return valid_count;
}

int32_t ScrollView::GetSemanticsActions() const {
  int32_t actions = BaseView::GetSemanticsActions();
  if (CanScrollY()) {
    actions |=
        static_cast<int32_t>(SemanticsNode::SemanticsAction::kScrollUp) |
        static_cast<int32_t>(SemanticsNode::SemanticsAction::kScrollDown);
  } else if (CanScrollX()) {
    actions |=
        static_cast<int32_t>(SemanticsNode::SemanticsAction::kScrollLeft) |
        static_cast<int32_t>(SemanticsNode::SemanticsAction::kScrollRight);
  } else {
    FML_DLOG(ERROR) << "scrollview cannot scroll, direction: "
                    << static_cast<int32_t>(scroll_direction_);
  }
  return actions;
}

int32_t ScrollView::GetSemanticsFlags() const {
  int32_t flags = BaseView::GetSemanticsFlags();
  flags |=
      static_cast<int32_t>(SemanticsNode::SemanticsFlag::kHasImplicitScrolling);
  return flags;
}
#endif

bool ScrollView::OnScrollToMiddle(BaseView* target_view) {
  if (!target_view) {
    FML_DLOG(ERROR) << "DispatchA11yShowOnScreenEvent but view is nullptr";
    return false;
  }
  FloatRect rect = target_view->GetBounds();
  rect.Move(PaddingLeft(), PaddingTop());
  ScrollChildViewToMiddle(rect);
  return true;
}

void ScrollView::DoScrollFromRaster(float scroll_offset, bool ignore_repaint) {
  auto delta = scroll_direction_ == ScrollDirection::kVertical
                   ? FloatPoint{0, scroll_offset - scroll_offset_.y()}
                   : FloatPoint{scroll_offset - scroll_offset_.x(), 0};
  DoScroll(delta, false, ignore_repaint);
}

void ScrollView::OnScrollUpdate(float scroll_offset) {
  auto delta = scroll_direction_ == ScrollDirection::kVertical
                   ? FloatPoint{0, scroll_offset - scroll_offset_.y()}
                   : FloatPoint{scroll_offset - scroll_offset_.x(), 0};
  DoScroll(delta, false);
}

bool ScrollView::OnScrollToVisible() {
  FloatRect focused_view_rect = GetFocusManager()->GetFocusedNodeRect();
  if (!focused_view_rect.IsEmpty()) {
    ScrollChildViewToVisible(focused_view_rect);
  }
  return true;
}

void ScrollView::SetAttribute(const char* attr_c, const clay::Value& value) {
  auto kw = GetKeywordID(attr_c);
  if (kw == KeywordID::kScrollX) {
    bool scroll_x = attribute_utils::GetBool(value);
    SetScrollDirection(scroll_x ? ScrollDirection::kHorizontal
                                : ScrollDirection::kVertical);
  } else if (kw == KeywordID::kScrollEventThrottle) {
    int throttle_ms = attribute_utils::GetInt(value);
    if (throttle_ms >= 0) {
      callback_manager_->SetScrollEventThrottle(
          fml::TimeDelta::FromMilliseconds(throttle_ms));
    }
  } else if (kw == KeywordID::kScrollY) {
    bool scroll_y = attribute_utils::GetBool(value);
    SetScrollDirection(scroll_y ? ScrollDirection::kVertical
                                : ScrollDirection::kHorizontal);
  } else if (kw == KeywordID::kLowerThreshold) {
    lower_threshold_ = FromLogical(attribute_utils::GetNum(value));
  } else if (kw == KeywordID::kUpperThreshold) {
    upper_threshold_ = FromLogical(attribute_utils::GetNum(value));
  } else if (kw == KeywordID::kScrollTop) {
    // keyword: scroll-top is deprecated, use initial-scroll-offset and ui
    // method instead.
    float offset = FromLogical(attribute_utils::GetNum(value));
    if (offset <= GetRenderScroll()->MaxScrollHeight()) {
      pending_scroll_offset_ = std::nullopt;
      OnScrollUpdate(offset);
    } else {
      pending_scroll_offset_ = offset;
    }
  } else if (kw == KeywordID::kScrollLeft) {
    // keyword: scroll-left is deprecated, use initial-scroll-offset and ui
    // method instead.
    float offset = FromLogical(attribute_utils::GetNum(value));
    if (offset <= GetRenderScroll()->MaxScrollWidth()) {
      pending_scroll_offset_ = std::nullopt;
      OnScrollUpdate(offset);
    } else {
      pending_scroll_offset_ = offset;
    }
  } else if (kw == KeywordID::kScrollToIndex) {
    // keyword: scroll-to-index is deprecated, use ui method and
    // initial-scroll-index instead.
    size_t index = attribute_utils::GetInt(value);
    if (index < child_count() && CanInvokeScrollImmediately()) {
      pending_scroll_index_ = -1;
      SetScrollToIndex(index);
    } else {
      pending_scroll_index_ = attribute_utils::GetNum(value);
    }
  } else if (kw == KeywordID::kScrollToId) {
    std::string result;
    if (attribute_utils::TryGetString(value, result)) {
      SetScrollToId(result);
    }
  } else if (kw == KeywordID::kEnableNestedScroll) {
    bool enable_nested_scroll = attribute_utils::GetBool(value);
    SetEnableNestedScroll(enable_nested_scroll);
  } else if (kw == KeywordID::kEnableScroll) {
    bool enabled = attribute_utils::GetBool(value, true);
    SetScrollEnabled(enabled);
  } else if (kw == KeywordID::kScrollMonitorTag) {
    std::string tag;
    if (attribute_utils::TryGetString(value, tag)) {
      SetScrollMonitorTag(tag);
    }
  } else if (kw == KeywordID::kScrollOrientation) {
    std::string orientation;
    attribute_utils::TryGetString(value, orientation);
    auto direction = ParseScrollDirection(orientation);
    if (GetScrollDirection() == direction) {
      return;
    }
    OnScrollUpdate(0);
    SetScrollDirection(direction);
  } else if (kw == KeywordID::kInitialScrollIndex) {
    size_t index = attribute_utils::GetInt(value);
    if (!initial_scroll_index_set_ && index >= 0) {
      initial_scroll_index_set_ = true;
      if (CanInvokeScrollImmediately()) {
        pending_scroll_index_ = -1;
        ScrollTo(false, 0, index);
      } else {
        pending_scroll_index_ = index;
      }
    }
  } else if (kw == KeywordID::kInitialScrollOffset) {
    int offset = attribute_utils::GetNum(value);
    if (!initial_scroll_offset_set_ && offset >= 0) {
      initial_scroll_offset_set_ = true;
      if (CanInvokeScrollImmediately()) {
        pending_scroll_offset_ = std::nullopt;
        ScrollTo(false, offset);
      } else {
        pending_scroll_offset_ = offset;
      }
    }
  } else if (kw == KeywordID::kBounce || kw == KeywordID::kBounces) {
    // Align with Android system-rendering behavior, disable bounce effect on
    // Android platform.
#if !defined(OS_ANDROID)
    auto enable_bounce = attribute_utils::GetBool(value);
    SetOverscrollEnabled(enable_bounce);
#endif
  } else if (kw == KeywordID::kScrollForwardMode) {
    scroll_forward_mode_ =
        ParseNestedScrollMode(attribute_utils::GetCString(value));
  } else if (kw == KeywordID::kScrollBackwardMode) {
    scroll_backward_mode_ =
        ParseNestedScrollMode(attribute_utils::GetCString(value));
  } else if (kw == KeywordID::kPrevScrollable) {
    pre_scrollable_ = attribute_utils::GetCString(value);
  } else if (kw == KeywordID::kNextScrollable) {
    next_scrollable_ = attribute_utils::GetCString(value);
  } else {
    BaseView::SetAttribute(attr_c, value);
  }
}

void ScrollView::SetDirection(int type) {
  float new_offset = 0;
  if (scroll_direction_ == ScrollDirection::kHorizontal &&
      (type == static_cast<int>(DirectionType::kLynxRtl) ||
       type == static_cast<int>(DirectionType::kRtl))) {
    new_offset = GetRenderScroll()->MaxScrollWidth();
  }
  OnScrollUpdate(new_offset);
  GetRenderScroll()->SetLayoutDirection(static_cast<DirectionType>(type));
}

void ScrollView::UpdateScrollOffsetForRTL() {
  if (scroll_direction_ == ScrollDirection::kHorizontal && IsRtlDirection()) {
    float previous_offset_x =
        old_content_size_.width() - scroll_offset_.x() - Width();
    GetScroller()->ScrollToImmediately(GetRenderScroll()->MaxScrollWidth() -
                                       previous_offset_x);
  }
}

void ScrollView::AddEventCallback(const char* event_c) {
  BaseView::AddEventCallback(event_c);
  std::string event(event_c);
  if (callback_manager_->AddEventCallback(event)) {
    return;
  }
  if (event.compare(event_attr::kEventContentSizeChanged) == 0) {
    enable_content_size_changed_event_ = true;
  }
}

float ScrollView::ContentWidth() const {
  if (content_size_.width() != 0.f) {
    return content_size_.width();
  }
  return BaseView::ContentWidth();
}

float ScrollView::ContentHeight() const {
  if (content_size_.height() != 0.f) {
    return content_size_.height();
  }
  return BaseView::ContentHeight();
}

const FloatRect& ScrollView::OverflowRect() const {
  RenderBox* box = static_cast<RenderBox*>(render_object_.get());
  return box->OverflowRect();
}

void ScrollView::OnScrollBegin() {
  // confirm the scroll area
  CalculateOverFlow();
  StopAnimation();
}

void ScrollView::OnScrollStatusChange(ScrollView::ScrollStatus old_status) {
  NestedScrollable::OnScrollStatusChange(old_status);
  if (status_ == Scrollable::ScrollStatus::kDragging) {
    OnScrollBegin();
  } else if (status_ == Scrollable::ScrollStatus::kBounce) {
    float offset = scroll_direction_ == ScrollDirection::kVertical
                       ? ClampedOverscrollOffset().y()
                       : ClampedOverscrollOffset().x();
    OnBounceStart(offset);
  } else if (status_ == Scrollable::ScrollStatus::kIdle) {
    callback_manager_->NotifyScrollEnd(scroll_offset_);
  }

  auto current_callback_scroll_status = ConvertScrollState(status_);
  if (last_callback_scroll_state_ == current_callback_scroll_status) {
    return;
  }
  auto old_callback_scroll_status = last_callback_scroll_state_;
  last_callback_scroll_state_ = current_callback_scroll_status;
  float velocity = 0;
  if (current_callback_scroll_status ==
      ScrollEventCallbackManager::ScrollState::kAnimating) {
    velocity = page_view()->nested_scroll_manager()->GetFlingVelocity();
    if (velocity == 0) {
      // in some cases, the first fling will be handled by rasterfling animator,
      // so we use drag end velocity instead.
      velocity = last_drag_end_velocity_;
    }
    // logicalpx/ms
    velocity = page_view()->ConvertTo<kPixelTypeLogical>(velocity) / 1000.0f;
  }
  callback_manager_->NotifyScrollStateChange(
      old_callback_scroll_status, current_callback_scroll_status, velocity,
      GetScrollStatus() != ScrollStatus::kAnimating);
}

void ScrollView::NotifyContentSizeChanged(const FloatSize& new_size) {
  if (enable_content_size_changed_event_) {
    auto event_name = "contentsizechanged";
    if (HasEvent(event_name)) {
      page_view()->SendEvent(
          callback_id_, event_name, {"scrollWidth", "scrollHeight"},
          ToLogical(new_size.width()), ToLogical(new_size.height()));
    }
  }
}

void ScrollView::OnChildSizeChanged(BaseView* child) {
  if (delegate_) {
    delegate_->OnScrollViewChildUpdated();
  }

  CalculateOverFlow();

  CorrectScrollOffset();
  MarkNeedsLayout();
}

void ScrollView::OnContentSizeChanged(const FloatRect& old_rect,
                                      const FloatRect& new_rect) {
  BaseView::OnContentSizeChanged(old_rect, new_rect);
  content_size_.SetWidth(std::max(new_rect.width(), content_size_.width()));
  content_size_.SetHeight(std::max(new_rect.height(), content_size_.height()));

  CorrectScrollOffset();
}

bool ScrollView::CanScroll() { return CanScrollX() || CanScrollY(); }

void ScrollView::StopAnimation() {
  NestedScrollable::StopAnimation();
  if (scroller_ && scroller_->IsScrolling()) {
    scroller_->AbortAnimation();
  }
}

void ScrollView::ScrollChildViewToVisible(const FloatRect& rect) {
  // A fake scroll-begin call to:
  // 1. Generate the lazy-create items which are not in the visible rect.
  // 2. Stop previous animation.
  OnScrollBegin();
  FloatRect scroll_view_rect = GetBounds();
  scroll_view_rect.SetLocation(FloatPoint(0, 0));

  FloatRect focused_view_rect = rect;
  focused_view_rect.MoveBy(-scroll_offset_);

  if (scroll_view_rect.Contains(focused_view_rect)) {
    return;
  }

  float offset = 0.f;
  if (CanScrollX()) {
    offset = rect.x() + rect.width() - scroll_view_rect.width();
    if (focused_view_rect.x() < scroll_view_rect.x()) {
      offset = rect.x();
    }
  }
  if (CanScrollY()) {
    offset = rect.y() + rect.height() - scroll_view_rect.height();
    if (focused_view_rect.y() < scroll_view_rect.y()) {
      offset = rect.y();
    }
  }

  OnScrollUpdate(offset);
}

void ScrollView::ScrollChildViewToMiddle(const FloatRect& rect) {
  OnScrollBegin();
  FloatRect scroll_view_rect = GetBounds();
  scroll_view_rect.SetLocation(FloatPoint(0, 0));

  FloatRect view_rect = rect;
  view_rect.MoveBy(-scroll_offset_);

  if (scroll_view_rect.Contains(view_rect)) {
    return;
  }

  float offset = 0.f;
  if (CanScrollX()) {
    offset = rect.x() + rect.width() / 2.f - scroll_view_rect.width() / 2.f;
    if (view_rect.x() < scroll_view_rect.x()) {
      offset = rect.x() - scroll_view_rect.width() / 2.f + rect.width() / 2.f;
    }
  }
  if (CanScrollY()) {
    offset = rect.y() + rect.height() / 2.f - scroll_view_rect.height() / 2.f;
    if (view_rect.y() < scroll_view_rect.y()) {
      offset = rect.y() - scroll_view_rect.height() / 2.f + rect.height() / 2.f;
    }
  }

  ScrollTo(true, offset);
}

void ScrollView::Invalidate() {
  FML_DCHECK(render_object());
  // ScrollView always fully loads all children. During scrolling, only
  // ClipRect needs to be updated without re-recording.
  render_object()->MarkNeedsEffect();
}

void ScrollView::ScrollWithDelta(bool smooth, float delta) {
  if (RoughlyEqualToZero(delta)) {
    return;
  }
  float start_offset = scroll_direction_ == ScrollDirection::kVertical
                           ? scroll_offset_.y()
                           : scroll_offset_.x();
  if (smooth) {
    GetScroller()->StartScroll(start_offset, delta);
  } else {
    GetScroller()->ScrollToImmediately(start_offset + delta);
  }
}

void ScrollView::ScrollTo(bool smooth, float offset, int index) {
  FML_DCHECK(scroll_direction_ != ScrollDirection::kNone);
  float start_offset = scroll_direction_ == ScrollDirection::kVertical
                           ? scroll_offset_.y()
                           : scroll_offset_.x();
  float end_offset = 0;
  auto& children = GetChildren();
  float child_offset = 0.f;
  if (index >= 0 && static_cast<unsigned int>(index) < children.size()) {
    if (scroll_direction_ == ScrollDirection::kVertical) {
      child_offset = children[index]->Top();
    } else {
      if (IsRtlDirection()) {
        child_offset =
            children[index]->Left() + children[index]->Width() - Width();
        offset = -offset;
      } else {
        child_offset = children[index]->Left();
      }
    }
  }
  end_offset = child_offset + offset;
  if (smooth) {
    GetScroller()->StartScroll(start_offset, end_offset - start_offset);
  } else {
    GetScroller()->ScrollToImmediately(end_offset);
  }
}

void ScrollView::OnBounceStart(float start_offset) {
  bool trigger_event = false;
  if (start_offset < 0) {
    trigger_event =
        bounce_view_ && (bounce_view_->GetDirection() == Direction::kLeft ||
                         bounce_view_->GetDirection() == Direction::kTop);
  } else if (start_offset > 0) {
    trigger_event =
        bounce_view_ && (bounce_view_->GetDirection() == Direction::kRight ||
                         bounce_view_->GetDirection() == Direction::kBottom);
  }
  if (trigger_event) {
    auto bounce_threshold = scroll_direction_ == ScrollDirection::kVertical
                                ? bounce_view_->ContentHeight()
                                : bounce_view_->ContentWidth();
    if (HasEvent(event_attr::kEventScrollToBounce) &&
        std::abs(start_offset) > bounce_threshold * 0.8 && trigger_event) {
      page_view()->SendEvent(callback_id_, event_attr::kEventScrollToBounce,
                             {});
    }
  }
}

void ScrollView::DidScroll() {
  NestedScrollable::DidScroll();

  // Trigger ScrollWrapper and descendants to be updated in a11y system.
#ifdef ENABLE_ACCESSIBILITY
  if (Parent() && Parent()->IsAccessibilityElement()) {
    if (SemanticsOwner* semantics_owner = GetSemanticsOwner()) {
      semantics_owner->AddDirtySemanticsForDescendants(this);
    }
  }
#endif

  auto delta = TotalScrollOffset() - last_scroll_offset_;
  last_scroll_offset_ = TotalScrollOffset();
  FML_DCHECK(!delta.IsOrigin()) << "delta should not be empty";
  RenderScroll* scroll = GetRenderScroll();
  if (enable_sticky_) {
    HandleSticky();
  }
  bool is_upper, is_lower;
  if (static_cast<int>(scroll_direction_) &
      static_cast<int>(ScrollDirection::kVertical)) {
    // vertical
    is_lower =
        scroll_offset_.y() >= scroll->MaxScrollHeight() - lower_threshold_;
    is_upper = scroll_offset_.y() <= upper_threshold_;
  } else {
    // horizontal
    if (IsRtlDirection()) {
      is_upper =
          scroll_offset_.x() >= scroll->MaxScrollWidth() - lower_threshold_;
      is_lower = scroll_offset_.x() <= upper_threshold_;
    } else {
      is_lower =
          scroll_offset_.x() >= scroll->MaxScrollWidth() - lower_threshold_;
      is_upper = scroll_offset_.x() <= upper_threshold_;
    }
  }
  ScrollEventCallbackManager::BorderStatus border_status;
  border_status.SetUpper(is_upper);
  border_status.SetLower(is_lower);
  callback_manager_->NotifyScrolled(
      delta, scroll_offset_, border_status,
      GetScrollStatus() == ScrollStatus::kDragging);

  // Trigger ScrollWrapper and descendants to be updated in a11y system.
#ifdef ENABLE_ACCESSIBILITY
  if (Parent() && Parent()->IsAccessibilityElement()) {
    if (SemanticsOwner* semantics_owner = GetSemanticsOwner()) {
      semantics_owner->AddDirtySemanticsForDescendants(Parent());
    }
  }
#endif
}

FloatPoint ScrollView::TotalScrollOffset() {
  return GetRenderScroll()->ScrollOffset();
}

void ScrollView::SetScrollToId(const std::string& id_str, bool smooth) {
  for (BaseView* child : GetChildren()) {
    // since lynx native use name instead of id here, we should use name
    // too.
    if (child && child->GetComponentName() == id_str) {
      if (scroll_direction_ == ScrollDirection::kVertical) {
        int offset = child->Top() - ContentInsetTop();
        ScrollTo(smooth, offset);
      } else {
        int offset = child->Left() - ContentInsetLeft();
        ScrollTo(smooth, offset);
      }
      break;
    }
  }
}

void ScrollView::SetScrollToIndex(int index) {
  auto& children = GetChildren();

  if (index < 0 || static_cast<unsigned int>(index) >= children.size()) {
    return;
  }
  int offset;
  if (scroll_direction_ == ScrollDirection::kVertical) {
    offset = children[index]->Top();
  } else {
    if (IsRtlDirection()) {
      offset = children[index]->Left() + children[index]->Width() - Width();
    } else {
      offset = children[index]->Left();
    }
  }
  OnScrollUpdate(offset);
}

void ScrollView::AutoScroll(bool start, float rate) {
  if (start && IsScrollEnabled()) {
    if (rate > 0 && !auto_scroll_) {
      const int rate_per_frame = static_cast<int>(std::max(1.f, (rate / 60)));
      auto_scroll_ = true;
      auto_scroll_rate_ = rate_per_frame;
      StartSmoothScroll();
    }
  } else {
    auto_scroll_ = false;
    auto_scroll_rate_ = 0;
  }
}

void ScrollView::OnScrollAnimationStart(float start, float delta,
                                        int duration) {
  SetScrollStatus(Scrollable::ScrollStatus::kAnimating);
}

void ScrollView::OnScrollAnimationEnd() {
  SetScrollStatus(Scrollable::ScrollStatus::kIdle);
}

// TODO(liuguoliang): Check if ScrollStatus::kAnimating is needed.
void ScrollView::StartSmoothScroll() {
  if (auto_scroll_) {
    if (scroll_direction_ == ScrollDirection::kVertical) {
      ScrollTo(false, auto_scroll_rate_ + scroll_offset_.y());
      if (scroll_offset_.y() <
          static_cast<RenderScroll*>(render_object())->MaxScrollHeight()) {
        page_view()->GetTaskRunner()->PostDelayedTask(
            [self = weak_factory_.GetWeakPtr()]() {
              if (self) {
                self->StartSmoothScroll();
              }
            },
            fml::TimeDelta::FromMilliseconds(16));
      } else {
        auto_scroll_ = false;
      }
    } else {
      float offset = 0.f;
      offset = scroll_offset_.x() +
               (IsRtlDirection() ? (-auto_scroll_rate_) : auto_scroll_rate_);
      ScrollTo(false, offset);
      bool has_space_to_scroll =
          IsRtlDirection()
              ? (scroll_offset_.x() > 0)
              : (scroll_offset_.x() <
                 static_cast<RenderScroll*>(render_object())->MaxScrollWidth());
      if (has_space_to_scroll) {
        page_view()->GetTaskRunner()->PostDelayedTask(
            [self = weak_factory_.GetWeakPtr()]() {
              if (self) {
                self->StartSmoothScroll();
              }
            },
            fml::TimeDelta::FromMilliseconds(16));
      } else {
        auto_scroll_ = false;
      }
    }
  }
}

void ScrollView::StartScrollInto(BaseView* node, std::string block,
                                 std::string inline_mode,
                                 std::string behavior) {
  auto* render_scroll = static_cast<RenderScroll*>(render_object_.get());
  float scroll_offset = 0;
  bool is_smooth = false;
  if (behavior == "smooth") {
    is_smooth = true;
  }
  if (CanScrollY()) {
    scroll_offset +=
        node->BoundsRelativeTo(this).location().y() + GetScrollOffset().y();
    if (block == "center") {
      scroll_offset -= (Height() - node->Height()) / 2;
    } else if (block == "end") {
      scroll_offset -= (Height() - node->Height());
    }
    scroll_offset = std::max(
        0.f, std::min<float>(scroll_offset, render_scroll->MaxScrollHeight()));
  } else {
    scroll_offset +=
        (IsRtlDirection() ? node->BoundsRelativeTo(this).MaxX() - Width()
                          : node->BoundsRelativeTo(this).x()) +
        GetScrollOffset().x();
    float width_delta = Width() - node->Width();
    if (inline_mode == "center") {
      scroll_offset -=
          (IsRtlDirection() ? (-width_delta / 2) : width_delta / 2);
    } else if (inline_mode == "end") {
      scroll_offset -= (IsRtlDirection() ? -width_delta : width_delta);
    }
    scroll_offset = std::max(
        0.f, std::min<float>(scroll_offset, render_scroll->MaxScrollWidth()));
  }
  ScrollTo(is_smooth, scroll_offset);
}

void ScrollView::CorrectScrollOffset() {
  if (skip_correct_scroll_offset_) {
    return;
  }
  RenderScroll* scroll = GetRenderScroll();
  if (!scroll) {
    return;
  }
  // Moving a scroll-view can temporarily detach its render object while child
  // insertions are still being replayed. Skip paint-offset correction until the
  // scroll render object is attached to a renderer again.
  if (!scroll->GetRenderer()) {
    return;
  }
  // Use the paint offset to correct the scroll offset. If we correct it using
  // scroll_offset_, the corrected scroll offset can get out of sync with the
  // paint offset, causing flickering.
  const auto paint_offset = scroll->GetPaintOffsetForScroll();
  if (scroll_direction_ == ScrollDirection::kVertical) {
    const float target_offset =
        std::min(paint_offset.y(), scroll->MaxScrollHeight());
    if (RoughlyNotZero(scroll_offset_.y() - target_offset)) {
      OnScrollUpdate(target_offset);
      return;
    }
    if (RoughlyNotZero(scroll->ScrollTop() - target_offset)) {
      scroll->ScrollTo(scroll->ScrollLeft(), target_offset);
      last_scroll_offset_ = scroll->ScrollOffset();
    }
  } else {
    const float target_offset =
        std::min(paint_offset.x(), scroll->MaxScrollWidth());
    if (RoughlyNotZero(scroll_offset_.x() - target_offset)) {
      OnScrollUpdate(target_offset);
      return;
    }
    if (RoughlyNotZero(scroll->ScrollLeft() - target_offset)) {
      scroll->ScrollTo(target_offset, scroll->ScrollTop());
      last_scroll_offset_ = scroll->ScrollOffset();
    }
  }
}

void ScrollView::EnableSticky() {
  enable_sticky_ = true;
  // This function will only be called when the sticky is updated
  HandleSticky();
}

void ScrollView::HandleSticky() {
  int left = scroll_offset_.x();
  int top = scroll_offset_.y();
  for (auto* child : children_) {
    child->CheckStickyOnParentScrollAndReset(left, top);
  }
}

#ifndef NDEBUG
std::string ScrollView::ToString() const {
  std::stringstream ss;
  ss << BaseView::ToString();
  ss << " direction=" << static_cast<uint8_t>(scroll_direction_);
  ss << " scroll_offset=" << GetScrollOffset().ToString();
  ss << " content_size_=" << content_size_.ToString();
  return ss.str();
}
#endif

NestedScrollable* ScrollView::NextScrollable() {
  return FindScrollableById(next_scrollable_)
             ?: NestedScrollable::NextScrollable();
}

NestedScrollable* ScrollView::PreviousScrollable() {
  return FindScrollableById(pre_scrollable_)
             ?: NestedScrollable::PreviousScrollable();
}

NestedScrollable* ScrollView::FindScrollableById(const std::string& id) {
  if (!id.empty()) {
    auto view = ViewContext::FindViewByIdSelector(id, page_view());
    return NestedScrollable::GetScrollable(view);
  }
  return nullptr;
}

bool ScrollView::CanInvokeScrollImmediately() const {
  // This logic is copied from Lynx UIScrollView.canInvokeScrollImmediately().
  int view_size = 0;
  int content_size = 0;
  if (scroll_direction_ == ScrollDirection::kVertical) {
    view_size = Height();
    content_size = GetRenderScroll()->OverflowRect().height();
    return view_size != 0 &&
           content_size > GetRenderScroll()->PaddingBoxRect().height();
  } else {
    view_size = Width();
    content_size = GetRenderScroll()->OverflowRect().width();
    return view_size != 0 &&
           content_size > GetRenderScroll()->PaddingBoxRect().width();
  }
}

FloatPoint ScrollView::DoScroll(FloatPoint delta, bool by_user_input,
                                bool ignore_repaint) {
  auto point = NestedScrollable::DoScroll(delta, by_user_input, ignore_repaint);
  OnViewPostionUpdate(FloatPoint(0, 0));
  return point;
}

std::vector<float> ScrollView::GestureScrollBy(float delta_x, float delta_y) {
  std::vector<float> res(4, 0);
  FloatPoint prev_scroll_offset = scroll_offset_;
  bool is_horizontal = scroll_direction_ == ScrollDirection::kHorizontal;
  ScrollWithDelta(false, is_horizontal ? delta_x : delta_y);
  FloatPoint delta = scroll_offset_ - prev_scroll_offset;
  res[0] = is_horizontal ? delta.x() : 0;
  res[1] = is_horizontal ? 0 : delta.y();
  res[2] = is_horizontal ? (delta_x - delta.x()) : delta_x;
  res[3] = is_horizontal ? delta_y : (delta_y - delta.y());
  return res;
}

bool ScrollView::CanConsumeGesture(float delta_x, float delta_y) {
  if (CanScrollX()) {
    return CanScrollBy(delta_x, 0);
  } else {
    return CanScrollBy(0, delta_y);
  }
}

float ScrollView::ScrollX() { return scroll_offset_.x(); }

int8_t ScrollView::GetScrollContainerDirection() {
  return CanScrollX() ? GestureConstants::DIRECTION_HORIZONTAL
                      : GestureConstants::DIRECTION_VERTICAL;
}

bool ScrollView::IsAtBorder(bool is_start) {
  if (is_start) {
    return CanScrollX() ? (scroll_offset_.x() <= 0) : (scroll_offset_.y() <= 0);
  } else {
    auto render_scroll = GetRenderScroll();
    return CanScrollX()
               ? (scroll_offset_.x() >= render_scroll->MaxScrollWidth())
               : (scroll_offset_.y() >= render_scroll->MaxScrollHeight());
  }
}

float ScrollView::ScrollY() { return scroll_offset_.y(); }

}  // namespace clay
