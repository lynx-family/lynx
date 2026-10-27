// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/text/text_view.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "clay/fml/logging.h"
#include "clay/gfx/geometry/float_point.h"
#include "clay/public/value.h"
#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/scroll_view.h"
#include "clay/ui/component/text/inline_text_view.h"
#include "clay/ui/component/view.h"
#include "clay/ui/gesture/long_press_gesture_recognizer.h"
#include "clay/ui/lynx_module/type_utils.h"
#include "clay/ui/rendering/render_scroll.h"

namespace clay {

namespace {

LYNX_UI_METHOD_BEGIN(TextView) {
  LYNX_UI_METHOD(TextView, getTextBoundingRect);
}
LYNX_UI_METHOD_END(TextView);

constexpr int kDefaultContentDistance = 12;

// Hot key tags.
constexpr uint32_t kTagCommand = 1;
constexpr uint32_t kTagControl = 1 << 1;

uint32_t AddTag(uint32_t value, uint32_t tag) { return value | tag; }

uint32_t RemoveTag(uint32_t value, uint32_t tag) { return value & ~tag; }

}  // namespace

static clay::Value::Map CreateRectMap(const FloatRect& rect) {
  clay::Value::Map map;
  map["left"] = clay::Value(rect.x());
  map["right"] = clay::Value(rect.MaxX());
  map["top"] = clay::Value(rect.y());
  map["bottom"] = clay::Value(rect.MaxY());
  map["width"] = clay::Value(rect.width());
  map["height"] = clay::Value(rect.height());
  return map;
}

TextView::TextView(int id, PageView* page_view)
    : TextView(id, "text", std::make_unique<RenderText>(), page_view) {}

TextView::TextView(int id, const std::string& tag,
                   std::unique_ptr<RenderObject> render_object,
                   PageView* page_view)
    : WithTypeInfo(id, tag, std::move(render_object), page_view),
      weak_factory_(this) {}

TextView::~TextView() { HideSelectionPopup(); }

void TextView::SetAttribute(const char* attr, const clay::Value& value) {
  auto kw = GetKeywordID(attr);
  if (kw == KeywordID::kTextSelection) {
    if ((is_text_selection_ = attribute_utils::GetBool(value))) {
      SetFocusable(true);
      ResetGestureRecognizers();
      GetRenderText()->SetSelectionChangedListener(
          [this](TextBox* text_box, int selection_start, int selection_end) {
            BringIntoView(text_box);
            OnSelectionChanged(selection_start, selection_end);
          });
    } else {
      ClearGestureRecognizers();
    }
  } else if (kw == KeywordID::kColor) {
    if (value.IsUint()) {
      SetColor(Color(attribute_utils::GetUint(value, 0xff000000)));
    } else if (value.IsArray()) {
      // The `setColor` interface of `baseView` is only used to trigger
      // animation. And the color of array value is linear gradient, which is
      // not supported currently. See `BaseTextShadowNode::RegisterSetters`.
    } else if (!value.IsNone()) {
      FML_DLOG(WARNING) << "KeywordID::kColor value is not valid.";
    }
  } else {
    BaseView ::SetAttribute(attr, value);
  }
}

void TextView::SetBorderWidth(Side side, float width) {
  BaseView::SetBorderWidth(side, width);
  UpdateInlineImageInfo();
}
void TextView::SetPaddings(float padding_left, float padding_top,
                           float padding_right, float padding_bottom) {
  BaseView::SetPaddings(padding_left, padding_top, padding_right,
                        padding_bottom);
  UpdateInlineImageInfo();
}

void TextView::PushInlineImageIndex(int id, int placeholder_id) {
  inline_images_index_.emplace(id, placeholder_id);
}
void TextView::PushInlineViewIndex(int id, int placeholder_id) {
  inline_views_index_.emplace(id, placeholder_id);
}

void TextView::SetColor(Color color) {
  if (TransitionMgr()->Enabled(ClayAnimationPropertyType::kColor) &&
      TransitionMgr()->TransitionTo(ClayAnimationPropertyType::kColor, color)) {
    UpdateTransitionRasterAnimation(ClayAnimationPropertyType::kColor);
    return;
  }
  SetProperty(ClayAnimationPropertyType::kColor, color, false);
}

bool TextView::OnKeyEvent(const KeyEvent* key_event) {
  if (ApplyHotKey(key_event)) {
    return false;
  }
  return true;
}

bool TextView::ApplyHotKey(const KeyEvent* key_event) {
  auto key_code = key_event->GetLogical();
  bool is_up = key_event->GetType() == KeyEventType::kUp;
  UpdateHotKeyTag(key_code, is_up);
  if (hot_key_tag_ == 0) {
    // Not in hot key mode.
    return false;
  }
  if (!is_up) {
    // Only handle second hot key down and repeat.
    if (hot_key_tag_ == kTagCommand) {
      HandleCommandHotKey(key_code);
    } else if (hot_key_tag_ == kTagControl) {
      HandleCtrlHotKey(key_code);
    }
    // Do not respond to multiple modifier key combination, like 'command' +
    // 'control' + key.
  }
  return true;
}

void TextView::UpdateHotKeyTag(LogicalKeyboardKey key_code, bool is_up) {
  switch (key_code) {
    case KeyCode::kMetaLeft:
    case KeyCode::kMetaRight:
    case KeyCode::kMeta:
      hot_key_tag_ = is_up ? RemoveTag(hot_key_tag_, kTagCommand)
                           : AddTag(hot_key_tag_, kTagCommand);
      break;
    case KeyCode::kControlLeft:
    case KeyCode::kControlRight:
    case KeyCode::kControl:
      hot_key_tag_ = is_up ? RemoveTag(hot_key_tag_, kTagControl)
                           : AddTag(hot_key_tag_, kTagControl);
      break;
    default:
      break;
  }
}

void TextView::ClearGestureRecognizers() {
#if defined(OS_ANDROID) || defined(OS_IOS)
  RemoveGestureRecognizer(long_press_recognizer_);
  long_press_recognizer_ = nullptr;
#else
  RemoveGestureRecognizer(drag_recognizer_);
  drag_recognizer_ = nullptr;
#endif
}

void TextView::HandleCommandHotKey(LogicalKeyboardKey key_code) {
  HandleWinCtrlAndMacCommandHotKey(key_code);
}

void TextView::HandleCtrlHotKey(LogicalKeyboardKey key_code) {
#if defined(WIN32) || defined(ENABLE_HEADLESS)
  HandleWinCtrlAndMacCommandHotKey(key_code);
#endif
}

void TextView::HandleWinCtrlAndMacCommandHotKey(LogicalKeyboardKey key_code) {
  switch (key_code) {
    case KeyCode::kKeyC: {
      // Copy to clipboard.
      std::u16string selected = GetRenderText()->GetSelectionString();
      if (selected.length() > 0) {
        page_view()->SetClipboardData(selected);
      }
      break;
    }
    case KeyCode::kKeyA: {
      // Select all.
      GetRenderText()->SetAllSelection();
      break;
    }
    default:
      break;
  }
}

void TextView::ResetGestureRecognizers() {
  ClearGestureRecognizers();
#if defined(OS_ANDROID) || defined(OS_IOS)
  auto long_press_recognizer = std::make_unique<LongPressGestureRecognizer>(
      page_view()->gesture_manager());
  long_press_recognizer_ = long_press_recognizer.get();
  long_press_recognizer->SetLongPressDownCallback(
      [this](const PointerEvent& event) {
        PerformBeginSelection(event.position);
      });
  long_press_recognizer->SetLongPressMoveCallback(
      [this](const PointerEvent& event) {
        PerformMoveSelection(event.position);
      });
  long_press_recognizer->SetLongPressCancelCallback(
      [this]() { PerformCancelSelection(); });
  long_press_recognizer->SetLongPressEndCallback(
      [this](const PointerEvent&) { ShowSelectionPopup(); });
  long_press_recognizer->SetDriftTolerance(10000);
  AddGestureRecognizer(std::move(long_press_recognizer));
#else
  auto drag_recognizer =
      std::make_unique<DragGestureRecognizer>(page_view()->gesture_manager());
  drag_recognizer->SetDelegate(this);
  drag_recognizer->SetTouchSlop(1);
  drag_recognizer_ = drag_recognizer.get();
  drag_recognizer->SetDragDownCallback(
      [this](const PointerEvent&) { RequestFocus(); });
  drag_recognizer->SetDragStartCallback(
      [this](const FloatPoint& event) { PerformBeginSelection(event); });
  drag_recognizer->SetDragUpdateCallback(
      [this](const FloatPoint& event, const FloatSize& delta) {
        PerformMoveSelection(event);
      });
  drag_recognizer->SetDragCancelCallback(
      [this]() { PerformCancelSelection(); });
  AddGestureRecognizer(std::move(drag_recognizer));
#endif
}

void TextView::PerformBeginSelection(FloatPoint point) {
#ifndef ENABLE_CLAY_LITE
  point -= BoundsRelativeTo(nullptr).location();
  selection_start_pos_ = point;
  selection_end_pos_ = point;
  static_cast<RenderText*>(render_object())
      ->SetSelection(selection_start_pos_, selection_end_pos_);
#endif
}

void TextView::PerformMoveSelection(FloatPoint point) {
#ifndef ENABLE_CLAY_LITE
  point -= BoundsRelativeTo(nullptr).location();
  selection_end_pos_ = point;
  static_cast<RenderText*>(render_object())
      ->SetSelection(selection_start_pos_, selection_end_pos_);
#endif
}

void TextView::PerformCancelSelection() { HideSelectionPopup(); }

void TextView::OnSelectionChanged(int selection_start, int selection_end) {
  std::string direction;
  if (selection_start > selection_end) {
    direction = "backward";
  } else {
    direction = "forward";
  }
  page_view()->SendEvent(
      id(), event_attr::kEventEditSelectionChange,
      {"start", "end", "direction"}, std::min(selection_start, selection_end),
      std::max(selection_start, selection_end), direction.c_str());
}

void TextView::getTextBoundingRect(const LynxModuleValues& args,
                                   const LynxUIMethodCallback& callback) {
  int start = -1, end = -1;
  CastNamedLynxModuleArgs({"start", "end"}, args, start, end);
  if (start >= end || start < 0 || end < 0 ||
      start > static_cast<int>(GetRenderText()->GetText().length()) ||
      end > static_cast<int>(GetRenderText()->GetText().length())) {
    callback(LynxUIMethodResult::kParamInvalid, clay::Value());
    return;
  }
  const auto& line_rects = GetRenderText()->GetTextLineRects(start, end);
  const auto& bounding_rect = page_view()->ConvertTo<kPixelTypeLogical>(
      GetRenderText()->GetTextBoundingRect(start, end, line_rects));

  clay::Value::Map result;
  result["boundingRect"] = clay::Value(CreateRectMap(bounding_rect));
  clay::Value::Array array(line_rects.size());
  for (size_t i = 0; i < line_rects.size(); i++) {
    array[i] = clay::Value(CreateRectMap(
        page_view()->ConvertTo<kPixelTypeLogical>(line_rects[i])));
  }
  result["boxes"] = clay::Value(std::move(array));
  callback(LynxUIMethodResult::kSuccess, clay::Value(std::move(result)));
}

RenderText* TextView::GetRenderText() {
  return static_cast<RenderText*>(render_object_.get());
}

void TextView::OnContentSizeChanged(const FloatRect& old_rect,
                                    const FloatRect& new_rect) {
  BaseView::OnContentSizeChanged(old_rect, new_rect);
  if (old_rect.width() == new_rect.width()) {
    return;
  }
  MarkNeedsLayout();
}

void TextView::OnBoundsChanged(const FloatRect& old_bounds,
                               const FloatRect& new_bounds) {
  BaseView::OnBoundsChanged(old_bounds, new_bounds);
  MarkNeedsLayout();
}

void TextView::FocusHasChanged(bool focused, bool is_leaf) {
#ifndef ENABLE_CLAY_LITE
  GetRenderText()->SetSelection(selection_end_pos_, selection_end_pos_);
#endif
  BaseView::FocusHasChanged(focused, is_leaf);
}

BaseView* TextView::GetTopViewToAcceptEvent(const FloatPoint& position,
                                            FloatPoint* relative_position) {
  FML_DCHECK(relative_position);
  if (!BaseView::CanAcceptEvent()) {
    // Not layouted yet.
    return nullptr;
  }

  FloatPoint point_by_self = GetPointBySelf(position);
  if (point_by_self.x() < 0 || point_by_self.x() > Width() ||
      point_by_self.y() < 0 || point_by_self.y() > Height()) {
    return nullptr;
  }

  FloatPoint point_by_paragraph = point_by_self;
  point_by_paragraph.Move(-BorderLeft() - PaddingLeft(),
                          -BorderTop() - PaddingTop());
  *relative_position = point_by_paragraph;
  BaseView* view = nullptr;
  view = GetViewAtPosition(point_by_paragraph, position);
  return view ?: this;
}

BaseView* TextView::GetViewAtPosition(const FloatPoint& point_by_paragraph,
                                      const FloatPoint& point_by_page) {
  auto paragraph = GetRenderText()->GetPainter()->GetParagraph();
  if (!paragraph) {
    return nullptr;
  }

  // Check if the point is located in a placeholder which is inline image /
  // inline view.
  int index = -1;
  for (const auto& box : paragraph->GetRectsForPlaceholders()) {
    if (box.rect.Contains(point_by_paragraph.x(), point_by_paragraph.y())) {
      index = box.placeholder_id;
      break;
    }
  }
  if (index >= 0) {
    for (auto image_index : inline_images_index_) {
      if (image_index.second == index) {
        return page_view_->FindViewByViewId(image_index.first);
      }
    }
    for (auto view_index : inline_views_index_) {
      if (view_index.second == index) {
        auto view = page_view_->FindViewByViewId(view_index.first);
        FloatPoint relative_position;
        auto top_view =
            view->GetTopViewToAcceptEvent(point_by_page, &relative_position);
        return top_view ? top_view : view;
      }
    }
    return nullptr;
  }

  auto text_pos = paragraph->GetGlyphPositionAtCoordinate(
      point_by_paragraph.x(), point_by_paragraph.y());

  // If there is no click on the text, no event response is required
  if (!ClickOnText(text_pos.position, point_by_paragraph, paragraph)) {
    return nullptr;
  }

  // Check if the point is located in an inline text / inline truncation.
  for (auto child : children_) {
    if (child->Is<View>()) {
      for (auto truncation_child : child->GetChildren()) {
        if (truncation_child->Is<InlineTextView>()) {
          auto view = static_cast<InlineTextView*>(truncation_child)
                          ->GetDeepestViewInPos(text_pos);
          if (view != nullptr) {
            return view;
          }
        }
      }
    } else if (child->Is<InlineTextView>()) {
      auto view =
          static_cast<InlineTextView*>(child)->GetDeepestViewInPos(text_pos);
      if (view != nullptr) {
        return view;
      }
    }
  }
  return nullptr;
}

bool TextView::ClickOnText(size_t glyph_index,
                           const FloatPoint& point_by_paragraph,
                           txt::Paragraph* paragraph) {
  if (paragraph) {
    auto line_metrics = paragraph->GetLineMetrics();
    for (auto line_metric : line_metrics) {
      if (glyph_index < line_metric.start_index ||
          glyph_index >= line_metric.end_index) {
        continue;
      }
      auto text_boxes = paragraph->GetRectsForRange(
          std::max(int(glyph_index) - 1, int(line_metric.start_index)),
          std::min(glyph_index + 1, line_metric.end_index),
          txt::Paragraph::RectHeightStyle::kTight,
          txt::Paragraph::RectWidthStyle::kTight);
      for (auto box : text_boxes) {
        if (point_by_paragraph.x() >= box.rect.Left() &&
            point_by_paragraph.x() <= box.rect.Right() &&
            point_by_paragraph.y() >= box.rect.Top() &&
            point_by_paragraph.y() <= box.rect.Bottom()) {
          return true;
        }
      }
    }
  }
  return false;
}

void TextView::ApplyPaintTransform(BaseView* child, Transform* transform) {
  FloatPoint point(Left(), Top());
  auto parent = Parent();
  FloatSize offset = parent->GetScrollOffset();
  point.Move(-offset.width(), -offset.height());

  point.Move(parent->Left(), parent->Top());
  transform->Translate(point.x(), point.y());
}

std::vector<FloatPoint> TextView::GetAnchorPosition() {
  RenderText* render_text = GetRenderText();
  auto select_position = render_text->GetSelectPosition();
  std::vector<Point> end_points = render_text->GetPointsFromRangeSelection(
      select_position[0], select_position[1]);
  if (end_points.empty()) {
    return std::vector<FloatPoint>();
  }
  ScrollView* scroll_view = FindScrollView();
  FloatSize scroll_offset;
  if (scroll_view != nullptr) {
    scroll_offset = scroll_view->GetScrollOffset();
  }
  auto start_point = FloatPoint(0, 0);
  auto end_point = FloatPoint(width_, height_);
  LocalToGlobal(start_point);
  LocalToGlobal(end_point);
  auto left = std::min(start_point.x(), end_point.x());
  auto top = std::min(start_point.y(), end_point.y());
  auto right = std::max(start_point.x(), end_point.x());
  auto bottom = std::max(start_point.y(), end_point.y());
  FloatRect editing_region = FloatRect(left, top, right - left, bottom - top);
  bool is_multiline =
      end_points.back().y() - end_points.front().y() >
      render_text->GetPainter()->GetLineHeightForPosition(select_position[1]) /
          2;
  double mid_x = is_multiline
                     ? editing_region.width() / 2
                     : (end_points.front().x() + end_points.back().x()) / 2 -
                           scroll_offset.width();
  FloatPoint mid_point =
      FloatPoint(mid_x, end_points[0].y() - scroll_offset.height() -
                            render_text->GetPainter()->GetLineHeightForPosition(
                                select_position[1]));
  // TODO(wangyanyi) now just not consider the iOS, because in iOS there is a
  // safe area concept
  double anchor_x =
      std::clamp(static_cast<double>(mid_point.x() + editing_region.x()), 8.0,
                 static_cast<double>(page_view()->Width()) - 8.0);
  FloatPoint anchor_above = FloatPoint(
      anchor_x, end_points.front().y() - scroll_offset.height() -
                    render_text->GetPainter()->GetLineHeightForPosition(
                        select_position[1]) +
                    editing_region.y());
  FloatPoint anchor_blow =
      FloatPoint(anchor_x, end_points.back().y() - scroll_offset.height() +
                               editing_region.y());
  return std::vector<FloatPoint>{anchor_above, anchor_blow};
}

void TextView::ShowSelectionPopup() {
#ifndef ENABLE_CLAY_LITE
  if (GetRenderText()->IsCollapsed()) {
    return;
  }
#if OS_ANDROID || OS_IOS
  HideSelectionPopup();
  selection_popup_ = new SelectionPopupView(page_view());
  selection_popup_->SetCopyFunction([weak = weak_factory_.GetWeakPtr()]() {
    if (weak) {
      weak->HandleCopy();
    }
  });
  selection_popup_->SetSelectAllFunction([weak = weak_factory_.GetWeakPtr()]() {
    if (weak) {
      weak->HandleSelectAll();
    }
  });
  selection_popup_->SetAnchorOffset(GetAnchorPosition());
  auto scroll_view = FindScrollView();
  selection_popup_->SetBoundsHeightAndWidth(
      scroll_view ? std::min(scroll_view->Width(), this->Width())
                  : std::min(this->Width(), page_view()->Width()),
      scroll_view ? std::min(scroll_view->Height(), this->Height())
                  : std::min(this->Height(), page_view()->Height()));
  selection_popup_->BuildSelectionPopup(
      std::vector<ActionType>{ActionType::kCopy, ActionType::kSelectAll});
  page_view()->AddChild(selection_popup_);
#endif
#endif
}

void TextView::HideSelectionPopup() {
#ifndef ENABLE_CLAY_LITE
  if (selection_popup_) {
    page_view()->RemoveChild(selection_popup_);
    delete selection_popup_;
    selection_popup_ = nullptr;
  }
#endif
}

void TextView::HandleCopy() {
#ifndef ENABLE_CLAY_LITE
  auto editing_text = GetRenderText()->GetSelectionString();
  page_view()->SetClipboardData(editing_text);
  GetRenderText()->SetSelection(selection_end_pos_, selection_end_pos_);
  page_view()->GetTaskRunner()->PostTask([weak = weak_factory_.GetWeakPtr()]() {
    if (weak) {
      weak->HideSelectionPopup();
    }
  });
#endif
}

void TextView::HandleSelectAll() {
#ifndef ENABLE_CLAY_LITE
  GetRenderText()->SetAllSelection();
  page_view()->GetTaskRunner()->PostTask([weak = weak_factory_.GetWeakPtr()]() {
    if (weak) {
      weak->HideSelectionPopup();
    }
  });
#endif
}

void TextView::BringIntoView(TextBox* text_box) {
  if (text_box == nullptr) {
    return;
  }
  FloatSize scroll_offset(0, 0);
  ScrollView* scroll_view = FindScrollView();
  if (scroll_view == nullptr) {
    return;
  }
  double target_offset = 0;
  if (scroll_view->CanScrollY()) {
    auto additional_offset = std::clamp(
        static_cast<float>(0.0),
        text_box->GetBottom() - scroll_view->Height() -
            scroll_view->GetScrollOffset().height() + kDefaultContentDistance,
        text_box->GetTop() - scroll_view->GetScrollOffset().height());
    target_offset = std::clamp(
        (additional_offset + scroll_view->GetScrollOffset().height()), 0.f,
        static_cast<RenderScroll*>(scroll_view->render_object())
            ->MaxScrollHeight());
  } else if (scroll_view->CanScrollX()) {
    auto additional_offset = std::clamp(
        static_cast<float>(0.0),
        text_box->GetRight() - scroll_view->Width() -
            scroll_view->GetScrollOffset().width() + kDefaultContentDistance,
        text_box->GetLeft() - scroll_view->GetScrollOffset().width());
    target_offset = std::clamp(
        (additional_offset + scroll_view->GetScrollOffset().width()), 0.f,
        static_cast<RenderScroll*>(scroll_view->render_object())
            ->MaxScrollWidth());
  }
  scroll_view->StopAnimation();
  scroll_view->ScrollTo(true, target_offset);
}

ScrollView* TextView::FindScrollView() {
  BaseView* parent = Parent();
  while (parent) {
    if (parent->Is<ScrollView>()) {
      return static_cast<ScrollView*>(parent);
    }
    parent = parent->Parent();
  }
  return nullptr;
}

#ifdef ENABLE_ACCESSIBILITY
std::u16string TextView::GetAccessibilityLabel() const {
  FML_DCHECK(render_object());
  return static_cast<RenderText*>(render_object())->GetText();
}
#endif

bool TextView::IsPointerAllowed(const GestureRecognizer& gesture_recognizer,
                                const PointerEvent& event) {
  if (event.device == PointerEvent::DeviceType::kMouse) {
    return event.buttons == PointerEvent::MouseButton::kPrimary;
  } else {
    return true;
  }
}

}  // namespace clay
