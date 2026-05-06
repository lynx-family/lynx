// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_text.h"

#include <native_drawing/drawing_brush.h>
#include <native_drawing/drawing_path.h>
#include <native_drawing/drawing_rect.h>

#include <algorithm>
#include <cmath>
#include <stack>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/dom/lynx_get_ui_result.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/font_collection_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/paragraph_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/style_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/utils/unicode_decode_utils.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"

namespace lynx {
namespace tasm {
namespace harmony {

namespace {

enum class HandleNodeType : int32_t {
  kStartDot = 1,
  kEndDot = 2,
};

static std::unordered_map<ArkUI_GestureRecognizer*, UIText*>
    g_recognizer_to_text;

}  // namespace

UIText::UIText(LynxContext* context, int sign, const std::string& tag)
    : UIBase(context, ARKUI_NODE_CUSTOM, sign, tag) {
  if (context->IsEnableTextOverflow()) {
    overflow_ = {true, true};
  }
  InitAccessibilityAttrs(LynxAccessibilityMode::kEnable, "text");
}

UIText::~UIText() {
  // Main gestures.
  if (long_press_gesture_) {
    g_recognizer_to_text.erase(long_press_gesture_);
    NodeManager::Instance().DisposeGesture(long_press_gesture_);
    long_press_gesture_ = nullptr;
  }
  if (pan_gesture_) {
    g_recognizer_to_text.erase(pan_gesture_);
    NodeManager::Instance().DisposeGesture(pan_gesture_);
    pan_gesture_ = nullptr;
  }
  if (tap_gesture_) {
    g_recognizer_to_text.erase(tap_gesture_);
    NodeManager::Instance().DisposeGesture(tap_gesture_);
    tap_gesture_ = nullptr;
  }

  // Handle gestures.
  if (start_handle_pan_gesture_) {
    NodeManager::Instance().DisposeGesture(start_handle_pan_gesture_);
    start_handle_pan_gesture_ = nullptr;
  }
  if (end_handle_pan_gesture_) {
    NodeManager::Instance().DisposeGesture(end_handle_pan_gesture_);
    end_handle_pan_gesture_ = nullptr;
  }

  // Remove handle nodes from parent if needed.
  if (handle_parent_clip_overridden_ && handle_parent_clip_node_) {
    auto parent = NodeManager::Instance().GetParent(DrawNode());
    if (parent && parent == handle_parent_clip_node_) {
      NodeManager::Instance().SetAttributeWithNumberValue(
          handle_parent_clip_node_, NODE_CLIP, handle_parent_clip_value_);
    }
    handle_parent_clip_overridden_ = false;
    handle_parent_clip_node_ = nullptr;
  }
  if (handle_parent_) {
    if (start_handle_dot_node_) {
      NodeManager::Instance().RemoveNode(handle_parent_,
                                         start_handle_dot_node_);
    }
    if (end_handle_dot_node_) {
      NodeManager::Instance().RemoveNode(handle_parent_, end_handle_dot_node_);
    }
  }

  if (start_handle_dot_node_) {
    NodeManager::Instance().UnregisterNodeCustomEvent(
        start_handle_dot_node_, ARKUI_NODE_CUSTOM_EVENT_ON_DRAW);
    NodeManager::Instance().RemoveNodeCustomEventReceiver(
        start_handle_dot_node_, HandleNodeCustomEventReceiver);
    NodeManager::Instance().DisposeNode(start_handle_dot_node_);
    start_handle_dot_node_ = nullptr;
  }
  if (end_handle_dot_node_) {
    NodeManager::Instance().UnregisterNodeCustomEvent(
        end_handle_dot_node_, ARKUI_NODE_CUSTOM_EVENT_ON_DRAW);
    NodeManager::Instance().RemoveNodeCustomEventReceiver(
        end_handle_dot_node_, HandleNodeCustomEventReceiver);
    NodeManager::Instance().DisposeNode(end_handle_dot_node_);
    end_handle_dot_node_ = nullptr;
  }

  handle_nodes_attached_ = false;
  handle_parent_ = nullptr;
}

void UIText::Update(ParagraphHarmony* paragraph) {
  paragraph_ = fml::RefPtr<ParagraphHarmony>(paragraph);
}

void UIText::UpdateExtraData(
    const fml::RefPtr<fml::RefCountedThreadSafeStorage>& bundle) {
  auto* paragraph = reinterpret_cast<ParagraphHarmony*>(bundle.get());
  paragraph_ = fml::RefPtr<ParagraphHarmony>(paragraph);
  paragraph_->SetEventTargetParent(weak_from_this());

  ClearSelection();
  Invalidate();
}

void UIText::FrameDidChanged() {
  UIBase::FrameDidChanged();
  if (paragraph_) {
    translate_left_offset_ =
        (padding_left_ + GetBorderLeftWidth()) * context_->ScaledDensity() +
        paragraph_->GetTranslateLeftOffset();
    translate_top_offset_ =
        (padding_top_ + GetBorderTopWidth()) * context_->ScaledDensity();

    UpdateInlineImageFrame();
    SetAccessibilityLabelDirtyFlag();

    UpdateHandleNodes();
  }
}

const std::string& UIText::GetAccessibilityLabel() const {
  if (UIBase::GetAccessibilityLabel().empty() && paragraph_) {
    return paragraph_->GetText();
  }
  return UIBase::GetAccessibilityLabel();
}

void UIText::UpdateInlineImageFrame() {
  auto rects = paragraph_->GetRectsForPlaceholders();
  size_t count = rects.GetCount();
  const auto& placeholder_signs = paragraph_->GetPlaceholders();
  for (auto& child_ui : Children()) {
    auto iterator = std::find(placeholder_signs.begin(),
                              placeholder_signs.end(), child_ui->Sign());
    if (iterator == placeholder_signs.end()) {
      child_ui->SetVisibility(false);
      continue;
    }
    size_t index = iterator - placeholder_signs.begin();
    if (index < count) {
      child_ui->SetVisibility(true);
    } else {
      // inline image or view will be omitted
      child_ui->SetVisibility(false);
    }
  }
}

void UIText::OnDraw(OH_Drawing_Canvas* canvas, ArkUI_NodeHandle node) {
  UIBase::OnDraw(canvas, node);
  if (NeedDraw(node)) {
    Render(canvas);
    UpdateHandleNodes();
  }
}

void UIText::OnDrawBehind(OH_Drawing_Canvas* canvas, ArkUI_NodeHandle node) {
  UIBase::OnDrawBehind(canvas, node);
  Render(canvas);
  UpdateHandleNodes();
}

void UIText::Render(OH_Drawing_Canvas* canvas) const {
  if (paragraph_) {
    OH_Drawing_CanvasSave(canvas);
    OH_Drawing_CanvasTranslate(canvas, translate_left_offset_,
                               translate_top_offset_);

    if (HasValidSelection()) {
      DrawSelectionHighlight(canvas);
    }

    // Paint paragraph at translated origin.
    paragraph_->Draw(canvas, 0, 0);

    if (HasValidSelection()) {
      DrawSelectionHandles(canvas);
    }

    OH_Drawing_CanvasRestore(canvas);
  }
}

EventTarget* UIText::HitTest(float point[2]) {
  if (paragraph_) {
    float point_exclude_padding_border[] = {
        (point[0] - (padding_left_ + GetBorderLeftWidth())) *
            context_->ScaledDensity(),
        (point[1] - (padding_top_ + GetBorderTopWidth())) *
            context_->ScaledDensity()};
    EventTarget* ret = paragraph_->HitTest(point_exclude_padding_border);
    if (ret != nullptr) {
      return ret;
    }
  }

  return UIBase::HitTest(point);
}

void UIText::OnPropUpdate(const std::string& name, const lepus::Value& value) {
  if (name == "text-selection") {
    if (value.IsBool()) {
      enable_text_selection_ = value.Bool();
      Invalidate();
    }
    return;
  }
  if (name == "selection-handle-color") {
    if (value.IsUInt32()) {
      handle_color_argb_ = value.UInt32();
      Invalidate();
    }
    return;
  }
  if (name == "selection-background-color") {
    if (value.IsUInt32()) {
      selection_bg_color_argb_ = value.UInt32();
      Invalidate();
    }
    return;
  }
  if (name == "selection-handle-size") {
    if (value.IsNumber()) {
      handle_dot_size_vp_ = std::max(0.f, static_cast<float>(value.Number()));
      Invalidate();
    }
    return;
  }
  if (name == "custom-context-menu") {
    if (value.IsBool()) {
      enable_custom_context_menu_ = value.Bool();
      Invalidate();
    }
    return;
  }
  if (name == "custom-text-selection") {
    if (value.IsBool()) {
      enable_custom_text_selection_ = value.Bool();
      Invalidate();
    }
    return;
  }
  UIBase::OnPropUpdate(name, value);
}

void UIText::OnNodeReady() {
  UIBase::OnNodeReady();

  UpdateSelectionGestureState();
}

void UIText::InvokeMethod(
    const std::string& method, const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  if (method == "getTextBoundingRect") {
    GetTextBoundingRect(args, std::move(callback));
  } else if (method == "setTextSelection") {
    SetTextSelection(args, std::move(callback));
  } else if (method == "getSelectedText") {
    GetSelectedText(args, std::move(callback));
  } else {
    UIBase::InvokeMethod(method, args, std::move(callback));
  }
}

void UIText::InitSelectionGestures() {
  long_press_gesture_ =
      NodeManager::Instance().CreateLongPressGesture(1, false, 500);
  if (long_press_gesture_) {
    g_recognizer_to_text[long_press_gesture_] = this;
    NodeManager::Instance().SetGestureEventTarget(
        long_press_gesture_, GESTURE_EVENT_ACTION_ACCEPT, this, OnLongPress);
    NodeManager::Instance().AddGestureToNode(DrawNode(), long_press_gesture_,
                                             PARALLEL, NORMAL_GESTURE_MASK);
  }

  pan_gesture_ =
      NodeManager::Instance().CreatePanGesture(1, GESTURE_DIRECTION_ALL, 5);
  if (pan_gesture_) {
    g_recognizer_to_text[pan_gesture_] = this;
    NodeManager::Instance().SetGestureEventTarget(
        pan_gesture_,
        GESTURE_EVENT_ACTION_ACCEPT | GESTURE_EVENT_ACTION_UPDATE |
            GESTURE_EVENT_ACTION_END | GESTURE_EVENT_ACTION_CANCEL,
        this, OnPan);
    NodeManager::Instance().AddGestureToNode(DrawNode(), pan_gesture_, PARALLEL,
                                             NORMAL_GESTURE_MASK);
  }

  tap_gesture_ =
      NodeManager::Instance().createTapGestureWithDistanceThreshold(1, 1, 5);
  if (tap_gesture_) {
    g_recognizer_to_text[tap_gesture_] = this;
    NodeManager::Instance().SetGestureEventTarget(
        tap_gesture_, GESTURE_EVENT_ACTION_ACCEPT, this, OnTap);
    NodeManager::Instance().AddGestureToNode(DrawNode(), tap_gesture_, PARALLEL,
                                             NORMAL_GESTURE_MASK);
  }

  NodeManager::Instance().SetGestureInterrupterToNode(DrawNode(),
                                                      OnGestureInterrupt);
}

void UIText::UpdateSelectionGestureState() {
  const bool should_enable =
      enable_text_selection_ && !enable_custom_text_selection_;
  if (should_enable) {
    if (!selection_gestures_inited_) {
      InitSelectionGestures();
      selection_gestures_inited_ = true;
    }
  } else if (selection_gestures_inited_) {
    DisposeSelectionGestures();
  }
}

void UIText::DisposeSelectionGestures() {
  if (long_press_gesture_) {
    g_recognizer_to_text.erase(long_press_gesture_);
    NodeManager::Instance().DisposeGesture(long_press_gesture_);
    long_press_gesture_ = nullptr;
  }
  if (pan_gesture_) {
    g_recognizer_to_text.erase(pan_gesture_);
    NodeManager::Instance().DisposeGesture(pan_gesture_);
    pan_gesture_ = nullptr;
  }
  if (tap_gesture_) {
    g_recognizer_to_text.erase(tap_gesture_);
    NodeManager::Instance().DisposeGesture(tap_gesture_);
    tap_gesture_ = nullptr;
  }
  selection_gestures_inited_ = false;
}

float UIText::GetXForEvent(const ArkUI_UIInputEvent* event) const {
  if (!context_) {
    return 0.f;
  }
  float x = OH_ArkUI_PointerEvent_GetX(event) -
            (padding_left_ + GetBorderLeftWidth()) * context_->ScaledDensity();
  if (paragraph_) {
    x -= paragraph_->GetTranslateLeftOffset();
  }
  return x;
}

float UIText::GetYForEvent(const ArkUI_UIInputEvent* event) const {
  if (!context_) {
    return 0.f;
  }
  return OH_ArkUI_PointerEvent_GetY(event) -
         (padding_top_ + GetBorderTopWidth()) * context_->ScaledDensity();
}

void UIText::OnLongPress(ArkUI_GestureEvent* event, void* extra) {
  auto* self = static_cast<UIText*>(extra);
  if (!self || !event) {
    return;
  }
  if (!self->enable_text_selection_ || self->enable_custom_text_selection_ ||
      !self->paragraph_) {
    return;
  }

  const ArkUI_UIInputEvent* raw_event =
      OH_ArkUI_GestureEvent_GetRawInputEvent(event);
  if (!raw_event) {
    return;
  }

  auto action = OH_ArkUI_GestureEvent_GetActionType(event);
  self->long_press_x_ = self->GetXForEvent(raw_event);
  self->long_press_y_ = self->GetYForEvent(raw_event);
  self->HandleLongPress(action);
}

void UIText::OnPan(ArkUI_GestureEvent* event, void* extra) {
  auto* self = static_cast<UIText*>(extra);
  if (!self || !event) {
    return;
  }
  if (!self->enable_text_selection_ || self->enable_custom_text_selection_ ||
      !self->paragraph_) {
    return;
  }

  auto raw_event = OH_ArkUI_GestureEvent_GetRawInputEvent(event);
  if (!raw_event) {
    return;
  }

  // cancel tap after long press
  self->long_press_fired_ = false;

  float local_x = self->GetXForEvent(raw_event);
  float local_y = self->GetYForEvent(raw_event);

  auto action = OH_ArkUI_GestureEvent_GetActionType(event);
  self->HandlePan(action, local_x, local_y);
}

void UIText::OnTap(ArkUI_GestureEvent* event, void* extra) {
  auto* self = static_cast<UIText*>(extra);
  if (!self) {
    return;
  }
  if (!self->enable_text_selection_ || self->enable_custom_text_selection_) {
    return;
  }

  // cancel tap after long press
  if (self->long_press_fired_) {
    self->long_press_fired_ = false;
    return;
  }

  if (self->is_in_selection_) {
    self->ClearSelection();
  }
}

ArkUI_GestureInterruptResult UIText::OnGestureInterrupt(
    ArkUI_GestureInterruptInfo* info) {
  auto* recognizer = OH_ArkUI_GestureInterruptInfo_GetRecognizer(info);
  auto it = g_recognizer_to_text.find(recognizer);
  if (it == g_recognizer_to_text.end()) {
    return GESTURE_INTERRUPT_RESULT_REJECT;
  }

  UIText* text = it->second;
  if (!text) {
    return GESTURE_INTERRUPT_RESULT_REJECT;
  }

  if (recognizer == text->long_press_gesture_) {
    return GESTURE_INTERRUPT_RESULT_CONTINUE;
  }
  if (recognizer == text->tap_gesture_) {
    return GESTURE_INTERRUPT_RESULT_CONTINUE;
  }
  if (recognizer == text->pan_gesture_) {
    if (text->ShouldInterceptPan()) {
      return GESTURE_INTERRUPT_RESULT_CONTINUE;
    }
    return GESTURE_INTERRUPT_RESULT_REJECT;
  }
  return GESTURE_INTERRUPT_RESULT_REJECT;
}

bool UIText::ShouldInterceptPan() const {
  if (!enable_text_selection_ || enable_custom_text_selection_) {
    return false;
  }
  if (track_long_press_) {
    return true;
  }
  if (is_in_selection_ && (is_adjust_start_ || is_adjust_end_)) {
    return true;
  }
  return false;
}

int32_t UIText::GetTextChar16Count() const {
  if (!paragraph_) {
    return 0;
  }
  return static_cast<int32_t>(base::SizeOfUtf16(paragraph_->GetText()));
}

bool UIText::HasValidSelection() const {
  if (!(enable_text_selection_ || enable_custom_text_selection_) ||
      !is_in_selection_ || !paragraph_) {
    return false;
  }
  const int32_t len = GetTextChar16Count();
  return select_start_ >= 0 && select_end_ >= 0 && select_start_ <= len &&
         select_end_ <= len;
}

void UIText::ClearSelection() {
  select_start_ = -1;
  select_end_ = -1;
  last_select_start_ = -1;
  last_select_end_ = -1;
  is_adjust_start_ = false;
  is_adjust_end_ = false;
  track_long_press_ = false;
  long_press_fired_ = false;
  show_start_handle_ = true;
  show_end_handle_ = true;
  if (is_in_selection_) {
    Invalidate();
    SendSelectionChangeEvent();
  }
  is_in_selection_ = false;
  HideHandleNodes();
}

void UIText::UpdateSelectionRange(int32_t start, int32_t end) {
  if (start != select_start_ || end != select_end_) {
    Invalidate();
  }
  select_start_ = start;
  select_end_ = end;
}

void UIText::UpdateSelectStartEnd() {
  is_forward_ = last_select_start_ == -1
                    ? select_start_ < select_end_
                    : (last_select_start_ < select_start_ ||
                       last_select_end_ < select_end_);

  if (select_start_ > select_end_) {
    std::swap(select_start_, select_end_);
  }
  last_select_start_ = select_start_;
  last_select_end_ = select_end_;

  SendSelectionChangeEvent();
}

void UIText::SendSelectionChangeEvent() const {
  if (!context_ || !HasEvent("selectionchange")) {
    return;
  }
  auto detail = lepus::Dictionary::Create();
  detail->SetValue("start", select_start_);
  detail->SetValue("end", select_end_);
  detail->SetValue("direction", is_forward_ ? "forward" : "backward");
  CustomEvent event{Sign(), "selectionchange", "detail", lepus_value(detail)};
  context_->SendEvent(event);
}

int32_t UIText::GetOffsetForPosition(float x, float y) const {
  if (!paragraph_) {
    return 0;
  }
  const int32_t len = GetTextChar16Count();
  const double max_width = paragraph_->GetMaxWidth();
  const double height = paragraph_->GetHeight();
  if (!(max_width > 0.0) || !(height > 0.0)) {
    return 0;
  }

  constexpr double kEpsilon = 0.001;
  const double clamped_x = std::clamp(static_cast<double>(x), 0.0,
                                      std::max(0.0, max_width - kEpsilon));
  const double clamped_y =
      std::clamp(static_cast<double>(y), 0.0, std::max(0.0, height - kEpsilon));

  auto pos = paragraph_->GetPositionAndAffinity(clamped_x, clamped_y);
  return std::min(len, static_cast<int32_t>(pos.GetPosition()));
}

void UIText::AdjustEndPosition(float x, float y) {
  is_adjust_end_ = true;
  int32_t select_end = GetOffsetForPosition(x, y);
  const int32_t len = GetTextChar16Count();
  if (select_start_ == select_end) {
    if (select_end == len || (x < start_x_ && select_end > 0)) {
      select_end--;
    } else {
      select_end++;
    }
  }
  UpdateSelectionRange(select_start_, select_end);
}

void UIText::AdjustStartPosition(float x, float y) {
  is_adjust_start_ = true;
  int32_t select_start = GetOffsetForPosition(x, y);
  const int32_t len = GetTextChar16Count();
  if (select_start == select_end_) {
    if (select_start == len || (x < end_x_ && select_start > 0)) {
      select_start--;
    } else {
      select_start++;
    }
  }
  UpdateSelectionRange(select_start, select_end_);
}

void UIText::PerformEndSelection(float x, float y) {
  if (!is_in_selection_) {
    ClearSelection();
    return;
  }

  if (is_adjust_start_) {
    AdjustStartPosition(x, y);
    UpdateSelectStartEnd();
  } else if (is_adjust_end_) {
    AdjustEndPosition(x, y);
    UpdateSelectStartEnd();
  }

  is_adjust_start_ = false;
  is_adjust_end_ = false;
}

void UIText::HandleLongPress(ArkUI_GestureEventActionType action) {
  if (enable_custom_text_selection_) {
    return;
  }
  if (!paragraph_) {
    return;
  }

  const int32_t start = GetOffsetForPosition(long_press_x_, long_press_y_);
  if (action & GESTURE_EVENT_ACTION_ACCEPT) {
    ClearSelection();

    long_press_fired_ = true;
    is_in_selection_ = true;
    track_long_press_ = true;
    start_x_ = end_x_ = long_press_x_;
    start_y_ = end_y_ = long_press_y_;
    select_start_ = select_end_ = start;
    show_start_handle_ = true;
    show_end_handle_ = true;
    AdjustEndPosition(end_x_, end_y_);
  } else if (action & GESTURE_EVENT_ACTION_END) {
    if (track_long_press_) {
      PerformEndSelection(long_press_x_, long_press_y_);
      track_long_press_ = false;
    }
  }
}

void UIText::HandlePan(ArkUI_GestureEventActionType action, float x, float y) {
  if (enable_custom_text_selection_) {
    return;
  }
  if (is_in_selection_) {
    if (action & GESTURE_EVENT_ACTION_UPDATE) {
      if (is_adjust_start_) {
        AdjustStartPosition(x, y);
      } else if (is_adjust_end_) {
        AdjustEndPosition(x, y);
      }
    } else if (action & GESTURE_EVENT_ACTION_END) {
      PerformEndSelection(x, y);
    }

    track_long_press_ = false;
  }
}

std::vector<base::geometry::FloatRect> UIText::GetTextBoundingBoxes(
    int32_t start, int32_t end) const {
  if (!paragraph_ || start < 0 || start > end) {
    return {};
  }
  const int32_t len = GetTextChar16Count();
  if (end > len) {
    end = len;
  }

  int32_t query_start = start;
  int32_t query_end = end;
  if (query_start == query_end) {
    if (query_start < len) {
      query_end = query_start + 1;
    } else if (query_start > 0) {
      query_start = query_start - 1;
    } else {
      return {};
    }
  }

  TextBoxHarmony text_box = paragraph_->GetRectsForRange(
      static_cast<size_t>(query_start), static_cast<size_t>(query_end));
  std::vector<base::geometry::FloatRect> boxes;
  const uint32_t count = text_box.GetCount();
  boxes.reserve(count);
  for (uint32_t i = 0; i < count; ++i) {
    const float left = text_box.GetLeft(i);
    const float top = text_box.GetTop(i);
    float right = text_box.GetRight(i);
    const float bottom = text_box.GetBottom(i);
    if (start == end) {
      // Caret-like rect: keep height, zero width.
      right = left;
    }
    boxes.emplace_back(base::geometry::Point<float>(left, top),
                       base::geometry::Size<float>(right - left, bottom - top));
  }
  return boxes;
}

fml::RefPtr<lepus::Dictionary> UIText::GetMapFromRect(
    float* rect, const base::geometry::FloatRect& box) const {
  auto ret = lepus::Dictionary::Create();
  const float density = context_ ? context_->ScaledDensity() : 1.f;
  const float px_to_vp = density > 0.f ? (1.f / density) : 1.f;

  ret->SetValue("left", rect[0] + padding_left_ + GetBorderLeftWidth() +
                            box.X() * px_to_vp);
  ret->SetValue(
      "top", rect[1] + padding_top_ + GetBorderTopWidth() + box.Y() * px_to_vp);
  ret->SetValue("right", rect[0] + padding_left_ + GetBorderLeftWidth() +
                             box.MaxX() * px_to_vp);
  ret->SetValue("bottom", rect[1] + padding_top_ + GetBorderTopWidth() +
                              box.MaxY() * px_to_vp);
  ret->SetValue("width", box.Width() * px_to_vp);
  ret->SetValue("height", box.Height() * px_to_vp);
  return ret;
}

fml::RefPtr<lepus::Dictionary> UIText::GetTextBoundingRectFromBoxes(
    const std::vector<base::geometry::FloatRect>& boxes,
    const lepus::Value& args, float* rect) const {
  auto ret = lepus::Dictionary::Create();
  if (boxes.empty()) {
    return ret;
  }

  auto bounding = boxes[0];
  for (const auto& b : boxes) {
    const float left = std::min(bounding.X(), b.X());
    const float top = std::min(bounding.Y(), b.Y());
    const float right = std::max(bounding.MaxX(), b.MaxX());
    const float bottom = std::max(bounding.MaxY(), b.MaxY());
    bounding = base::geometry::FloatRect(
        base::geometry::Point<float>(left, top),
        base::geometry::Size<float>(right - left, bottom - top));
  }

  ret->SetValue("boundingRect", GetMapFromRect(rect, bounding));
  auto box_arr = lepus::CArray::Create();
  for (const auto& b : boxes) {
    box_arr->emplace_back(GetMapFromRect(rect, b));
  }
  ret->SetValue("boxes", std::move(box_arr));
  return ret;
}

void UIText::GetTextBoundingRect(
    const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  if (!args.IsTable()) {
    callback(LynxGetUIResult::PARAM_INVALID,
             lepus_value("params is not table!"));
    return;
  }
  const auto table = args.Table();
  const auto& start = table->GetValue("start");
  const auto& end = table->GetValue("end");
  if (!start.IsNumber() || !end.IsNumber()) {
    callback(LynxGetUIResult::PARAM_INVALID,
             lepus_value("params is not number!"));
    return;
  }

  int32_t start_number = static_cast<int32_t>(start.Number());
  int32_t end_number = static_cast<int32_t>(end.Number());
  if (start_number > end_number || start_number < 0 || end_number < 0) {
    callback(LynxGetUIResult::PARAM_INVALID,
             lepus_value("param start or end is invalid!"));
    return;
  }
  if (!paragraph_) {
    callback(LynxGetUIResult::NO_UI_FOR_NODE,
             lepus_value("paragraph is null!"));
    return;
  }

  std::vector<base::geometry::FloatRect> boxes =
      GetTextBoundingBoxes(start_number, end_number);
  if (!boxes.empty()) {
    float rect[4] = {0, 0, width_, height_};
    GetBoundingClientRect(args, rect);
    auto ret = GetTextBoundingRectFromBoxes(boxes, args, rect);
    callback(LynxGetUIResult::SUCCESS, lepus_value(ret));
  } else {
    callback(LynxGetUIResult::UNKNOWN,
             lepus_value("Can not find text bounding rect."));
  }
}

std::vector<std::vector<float>> UIText::GetHandlesInfo() const {
  if (!HasValidSelection()) {
    return {};
  }
  int32_t start_index = std::min(select_start_, select_end_);
  int32_t end_index = std::max(select_start_, select_end_);
  if (start_index == end_index) {
    return {};
  }

  float density = context_ ? context_->ScaledDensity() : 1.f;
  if (density <= 0.f) {
    density = 1.f;
  }
  const float dot_diameter_vp =
      handle_dot_size_vp_ > 0.f ? handle_dot_size_vp_ : 15.f;
  const float dot_radius_px = (dot_diameter_vp / 2.f) * density;
  const float stem_w_px = std::max(0.f, stem_width_vp_) * density;

  // Use char bounding rects for start/end.
  TextBoxHarmony start_box = paragraph_->GetRectsForRange(
      static_cast<size_t>(start_index), static_cast<size_t>(start_index + 1));
  TextBoxHarmony end_box = paragraph_->GetRectsForRange(
      static_cast<size_t>(end_index - 1), static_cast<size_t>(end_index));
  if (start_box.GetCount() == 0 || end_box.GetCount() == 0) {
    return {};
  }

  // Pick first box for start, last box for end.
  const float start_left = start_box.GetLeft(0);
  const float start_top = start_box.GetTop(0);
  const float start_right = start_box.GetRight(0);
  const float start_bottom = start_box.GetBottom(0);
  base::geometry::FloatRect start_rect(
      base::geometry::Point<float>(start_left, start_top),
      base::geometry::Size<float>(start_right - start_left,
                                  start_bottom - start_top));
  const uint32_t end_last = end_box.GetCount() - 1;
  const float end_left = end_box.GetLeft(end_last);
  const float end_top = end_box.GetTop(end_last);
  const float end_right = end_box.GetRight(end_last);
  const float end_bottom = end_box.GetBottom(end_last);
  base::geometry::FloatRect end_rect(
      base::geometry::Point<float>(end_left, end_top),
      base::geometry::Size<float>(end_right - end_left, end_bottom - end_top));

  const float begin_anchor_x_px = start_rect.X();
  const float begin_anchor_y_px = start_rect.Y();
  const float end_anchor_x_px = end_rect.MaxX();
  const float end_anchor_y_px = end_rect.MaxY();

  const float begin_center_x_px =
      stem_w_px > 0.f
          ? (std::round(begin_anchor_x_px - stem_w_px / 2.f) + stem_w_px / 2.f)
          : begin_anchor_x_px;
  const float end_center_x_px =
      stem_w_px > 0.f
          ? (std::round(end_anchor_x_px - stem_w_px / 2.f) + stem_w_px / 2.f)
          : end_anchor_x_px;

  return {
      {begin_center_x_px, begin_anchor_y_px - dot_radius_px, dot_radius_px},
      {end_center_x_px, end_anchor_y_px + dot_radius_px, dot_radius_px},
  };
}

fml::RefPtr<lepus::Dictionary> UIText::GetHandleMap(
    const std::vector<float>& handle, float* rect) const {
  auto ret = lepus::Dictionary::Create();
  if (handle.size() < 3) {
    return ret;
  }
  const float density = context_ ? context_->ScaledDensity() : 1.f;
  const float px_to_vp = density > 0.f ? (1.f / density) : 1.f;

  // Note: handle coordinates are in paragraph local px (without
  // padding/border).
  const float x_vp = handle[0] * px_to_vp;
  const float y_vp = handle[1] * px_to_vp;
  ret->SetValue("x", rect[0] + padding_left_ + GetBorderLeftWidth() + x_vp);
  ret->SetValue("y", rect[1] + padding_top_ + GetBorderTopWidth() + y_vp);
  ret->SetValue("radius", handle[2] * px_to_vp);
  return ret;
}

void UIText::SetTextSelection(
    const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  if (!context_) {
    callback(LynxGetUIResult::UNKNOWN, lepus_value("context is null!"));
    return;
  }
  if (!args.IsTable()) {
    callback(LynxGetUIResult::PARAM_INVALID,
             lepus_value("params is not table!"));
    return;
  }
  if (!paragraph_) {
    callback(LynxGetUIResult::NO_UI_FOR_NODE,
             lepus_value("paragraph is null!"));
    return;
  }

  const auto table = args.Table();
  const float density = context_->ScaledDensity();

  const auto& start_x_val = table->GetValue("startX");
  const auto& start_y_val = table->GetValue("startY");
  const auto& end_x_val = table->GetValue("endX");
  const auto& end_y_val = table->GetValue("endY");
  if (!start_x_val.IsNumber() || !start_y_val.IsNumber() ||
      !end_x_val.IsNumber() || !end_y_val.IsNumber()) {
    callback(LynxGetUIResult::PARAM_INVALID,
             lepus_value("parameter is invalid"));
    return;
  }

  double start_x =
      start_x_val.Number() * density - padding_left_ - GetBorderLeftWidth();
  double start_y =
      start_y_val.Number() * density - padding_top_ - GetBorderTopWidth();
  double end_x =
      end_x_val.Number() * density - padding_left_ - GetBorderLeftWidth();
  double end_y =
      end_y_val.Number() * density - padding_top_ - GetBorderTopWidth();

  if (start_x < 0 || start_y < 0 || end_x < 0 || end_y < 0) {
    ClearSelection();
    callback(LynxGetUIResult::SUCCESS, lepus_value());
    return;
  }

  // Apply paragraph translate-left offset to convert to un-translated coords.
  start_x -= paragraph_->GetTranslateLeftOffset();
  end_x -= paragraph_->GetTranslateLeftOffset();

  // Optional flags.
  const auto& show_start_val = table->GetValue("showStartHandle");
  const auto& show_end_val = table->GetValue("showEndHandle");
  show_start_handle_ = show_start_val.IsBool() ? show_start_val.Bool() : true;
  show_end_handle_ = show_end_val.IsBool() ? show_end_val.Bool() : true;

  int32_t start_index = GetOffsetForPosition(static_cast<float>(start_x),
                                             static_cast<float>(start_y));
  int32_t end_index = GetOffsetForPosition(static_cast<float>(end_x),
                                           static_cast<float>(end_y));
  if (start_index < 0 || end_index < 0) {
    ClearSelection();
    callback(LynxGetUIResult::SUCCESS, lepus_value());
    return;
  }

  if (start_index == end_index) {
    const int32_t len = GetTextChar16Count();
    if (start_index >= len && start_index > 0) {
      start_index--;
    } else {
      end_index++;
    }
  }

  is_in_selection_ = true;
  UpdateSelectionRange(start_index, end_index);
  UpdateSelectStartEnd();

  std::vector<base::geometry::FloatRect> boxes = GetTextBoundingBoxes(
      std::min(start_index, end_index), std::max(start_index, end_index));
  if (boxes.empty()) {
    callback(LynxGetUIResult::SUCCESS, lepus_value());
  } else {
    float rect[4] = {0, 0, width_, height_};
    GetBoundingClientRect(args, rect);
    auto ret = GetTextBoundingRectFromBoxes(boxes, args, rect);

    auto handles = GetHandlesInfo();
    auto handle_arr = lepus::CArray::Create();
    for (const auto& h : handles) {
      handle_arr->emplace_back(GetHandleMap(h, rect));
    }
    ret->SetValue("handles", std::move(handle_arr));

    callback(LynxGetUIResult::SUCCESS, lepus_value(ret));
  }

  Invalidate();
}

void UIText::GetSelectedText(
    const lepus::Value& args,
    base::MoveOnlyClosure<void, int32_t, const lepus::Value&> callback) {
  (void)args;
  if (!paragraph_) {
    callback(LynxGetUIResult::NO_UI_FOR_NODE,
             lepus_value("paragraph is null!"));
    return;
  }

  std::string selected;
  if (select_start_ >= 0 && select_end_ > select_start_) {
    // UTF-16 indices -> slice.
    auto u16 = base::U8StringToU16(paragraph_->GetText());
    const int32_t len = static_cast<int32_t>(u16.size());
    const int32_t s = std::clamp(select_start_, 0, len);
    const int32_t e = std::clamp(select_end_, 0, len);
    if (e > s) {
      selected = base::U16StringToU8(std::u16string_view(u16).substr(
          static_cast<size_t>(s), static_cast<size_t>(e - s)));
    }
  }
  auto ret = lepus::Dictionary::Create();
  ret->SetValue("selectedText", std::move(selected));
  callback(LynxGetUIResult::SUCCESS, lepus_value(ret));
}

void UIText::DrawSelectionHighlight(OH_Drawing_Canvas* canvas) const {
  if (!canvas || !HasValidSelection()) {
    return;
  }

  const int32_t start = std::min(select_start_, select_end_);
  const int32_t end = std::max(select_start_, select_end_);
  std::vector<base::geometry::FloatRect> boxes =
      GetTextBoundingBoxes(start, end);
  if (boxes.empty()) {
    return;
  }

  OH_Drawing_Brush* brush = OH_Drawing_BrushCreate();
  OH_Drawing_BrushSetAntiAlias(brush, true);
  OH_Drawing_BrushSetColor(brush, selection_bg_color_argb_);
  OH_Drawing_CanvasAttachBrush(canvas, brush);
  for (const auto& b : boxes) {
    OH_Drawing_Rect* r =
        OH_Drawing_RectCreate(b.X(), b.Y(), b.MaxX(), b.MaxY());
    OH_Drawing_CanvasDrawRect(canvas, r);
    OH_Drawing_RectDestroy(r);
  }
  OH_Drawing_CanvasDetachBrush(canvas);
  OH_Drawing_BrushDestroy(brush);
}

void UIText::DrawSelectionHandles(OH_Drawing_Canvas* canvas) const {
  if (!canvas || !HasValidSelection()) {
    return;
  }
  if (!show_start_handle_ && !show_end_handle_) {
    return;
  }

  int32_t start_index = std::min(select_start_, select_end_);
  int32_t end_index = std::max(select_start_, select_end_);
  if (start_index == end_index) {
    return;
  }

  const float density = context_ ? context_->ScaledDensity() : 1.f;
  const float stem_w_px = std::max(0.f, stem_width_vp_) * density;

  TextBoxHarmony start_box = paragraph_->GetRectsForRange(
      static_cast<size_t>(start_index), static_cast<size_t>(start_index + 1));
  TextBoxHarmony end_box = paragraph_->GetRectsForRange(
      static_cast<size_t>(end_index - 1), static_cast<size_t>(end_index));
  if (start_box.GetCount() == 0 || end_box.GetCount() == 0) {
    return;
  }

  const float start_top = start_box.GetTop(0);
  const float start_bottom = start_box.GetBottom(0);
  const float end_top = end_box.GetTop(end_box.GetCount() - 1);
  const float end_bottom = end_box.GetBottom(end_box.GetCount() - 1);

  const float begin_anchor_x_px = start_box.GetLeft(0);
  const float end_anchor_x_px = end_box.GetRight(end_box.GetCount() - 1);

  const float begin_center_x_px =
      stem_w_px > 0.f
          ? (std::round(begin_anchor_x_px - stem_w_px / 2.f) + stem_w_px / 2.f)
          : begin_anchor_x_px;
  const float end_center_x_px =
      stem_w_px > 0.f
          ? (std::round(end_anchor_x_px - stem_w_px / 2.f) + stem_w_px / 2.f)
          : end_anchor_x_px;

  OH_Drawing_Brush* brush = OH_Drawing_BrushCreate();
  OH_Drawing_BrushSetAntiAlias(brush, true);
  OH_Drawing_BrushSetColor(brush, handle_color_argb_);
  OH_Drawing_CanvasAttachBrush(canvas, brush);

  const auto draw_stem = [&](float center_x, float top, float bottom) {
    const float left = std::round(center_x - stem_w_px / 2.f);
    top = std::round(top);
    bottom = std::round(bottom);
    OH_Drawing_Rect* r =
        OH_Drawing_RectCreate(left, top, left + stem_w_px, bottom);
    OH_Drawing_CanvasDrawRect(canvas, r);
    OH_Drawing_RectDestroy(r);
  };

  const bool start_handle_is_begin = select_start_ <= select_end_;
  if (start_handle_is_begin) {
    if (show_start_handle_) {
      draw_stem(begin_center_x_px, start_top, start_bottom);
    }
    if (show_end_handle_) {
      draw_stem(end_center_x_px, end_top, end_bottom);
    }
  } else {
    if (show_start_handle_) {
      draw_stem(end_center_x_px, end_top, end_bottom);
    }
    if (show_end_handle_) {
      draw_stem(begin_center_x_px, start_top, start_bottom);
    }
  }

  OH_Drawing_CanvasDetachBrush(canvas);
  OH_Drawing_BrushDestroy(brush);
}

void UIText::HandleNodeCustomEventReceiver(ArkUI_NodeCustomEvent* event) {
  auto* extra = reinterpret_cast<HandleDrawExtra*>(
      OH_ArkUI_NodeCustomEvent_GetUserData(event));
  if (!extra || !extra->owner) {
    return;
  }
  ArkUI_NodeCustomEventType type = OH_ArkUI_NodeCustomEvent_GetEventType(event);
  if (type != ARKUI_NODE_CUSTOM_EVENT_ON_DRAW) {
    return;
  }
  ArkUI_DrawContext* ctx = OH_ArkUI_NodeCustomEvent_GetDrawContextInDraw(event);
  if (!ctx) {
    return;
  }
  auto* canvas =
      reinterpret_cast<OH_Drawing_Canvas*>(OH_ArkUI_DrawContext_GetCanvas(ctx));
  if (!canvas) {
    return;
  }
  extra->owner->DrawHandleNode(canvas, extra->type);
}

void UIText::OnHandlePan(ArkUI_GestureEvent* event, void* extra) {
  auto* pan_extra = static_cast<HandlePanExtra*>(extra);
  if (!pan_extra || !pan_extra->owner) {
    return;
  }
  pan_extra->owner->HandlePanOnHandleNode(pan_extra->is_start, event);
}

ArkUI_GestureInterruptResult UIText::OnHandleGestureInterrupt(
    ArkUI_GestureInterruptInfo* info) {
  (void)info;
  return GESTURE_INTERRUPT_RESULT_CONTINUE;
}

void UIText::EnsureHandleNodesCreated() {
  if (start_handle_dot_node_ && end_handle_dot_node_) {
    return;
  }

  const auto create_dot_node = [&](ArkUI_NodeHandle& node,
                                   HandleDrawExtra& draw_extra, int32_t type) {
    if (node) {
      return;
    }
    node = NodeManager::Instance().CreateNode(ARKUI_NODE_CUSTOM);
    if (!node) {
      return;
    }
    draw_extra.owner = this;
    draw_extra.type = type;
    NodeManager::Instance().AddNodeCustomEventReceiver(
        node, HandleNodeCustomEventReceiver);
    NodeManager::Instance().RegisterNodeCustomEvent(
        node, ARKUI_NODE_CUSTOM_EVENT_ON_DRAW, &draw_extra);
    NodeManager::Instance().SetAttributeWithNumberValue(node, NODE_Z_INDEX,
                                                        9999);
    NodeManager::Instance().SetAttributeWithNumberValue(
        node, NODE_VISIBILITY, static_cast<int32_t>(ARKUI_VISIBILITY_HIDDEN));
  };

  create_dot_node(start_handle_dot_node_, start_dot_draw_extra_,
                  static_cast<int32_t>(HandleNodeType::kStartDot));
  create_dot_node(end_handle_dot_node_, end_dot_draw_extra_,
                  static_cast<int32_t>(HandleNodeType::kEndDot));

  if (start_handle_dot_node_ && !start_handle_pan_gesture_) {
    start_pan_extra_.owner = this;
    start_pan_extra_.is_start = true;
    start_handle_pan_gesture_ =
        NodeManager::Instance().CreatePanGesture(1, GESTURE_DIRECTION_ALL, 1);
    if (start_handle_pan_gesture_) {
      NodeManager::Instance().SetGestureEventTarget(
          start_handle_pan_gesture_,
          GESTURE_EVENT_ACTION_ACCEPT | GESTURE_EVENT_ACTION_UPDATE |
              GESTURE_EVENT_ACTION_END | GESTURE_EVENT_ACTION_CANCEL,
          &start_pan_extra_, OnHandlePan);
      NodeManager::Instance().AddGestureToNode(start_handle_dot_node_,
                                               start_handle_pan_gesture_,
                                               PRIORITY, NORMAL_GESTURE_MASK);
      NodeManager::Instance().SetGestureInterrupterToNode(
          start_handle_dot_node_, OnHandleGestureInterrupt);
    }
  }

  if (end_handle_dot_node_ && !end_handle_pan_gesture_) {
    end_pan_extra_.owner = this;
    end_pan_extra_.is_start = false;
    end_handle_pan_gesture_ =
        NodeManager::Instance().CreatePanGesture(1, GESTURE_DIRECTION_ALL, 1);
    if (end_handle_pan_gesture_) {
      NodeManager::Instance().SetGestureEventTarget(
          end_handle_pan_gesture_,
          GESTURE_EVENT_ACTION_ACCEPT | GESTURE_EVENT_ACTION_UPDATE |
              GESTURE_EVENT_ACTION_END | GESTURE_EVENT_ACTION_CANCEL,
          &end_pan_extra_, OnHandlePan);
      NodeManager::Instance().AddGestureToNode(end_handle_dot_node_,
                                               end_handle_pan_gesture_,
                                               PRIORITY, NORMAL_GESTURE_MASK);
      NodeManager::Instance().SetGestureInterrupterToNode(
          end_handle_dot_node_, OnHandleGestureInterrupt);
    }
  }
}

void UIText::EnsureHandleNodesAttached() {
  EnsureHandleNodesCreated();
  if (!start_handle_dot_node_ || !end_handle_dot_node_) {
    return;
  }
  // Attach handle nodes to the same parent as DrawNode() so they scroll
  // together.
  auto parent = NodeManager::Instance().GetParent(DrawNode());
  if (!parent) {
    return;
  }
  if (handle_nodes_attached_ && handle_parent_ == parent) {
    return;
  }
  if (handle_parent_) {
    NodeManager::Instance().RemoveNode(handle_parent_, start_handle_dot_node_);
    NodeManager::Instance().RemoveNode(handle_parent_, end_handle_dot_node_);
  }
  if (handle_parent_clip_overridden_ && handle_parent_clip_node_) {
    auto current_parent = NodeManager::Instance().GetParent(DrawNode());
    if (current_parent && current_parent == handle_parent_clip_node_) {
      NodeManager::Instance().SetAttributeWithNumberValue(
          handle_parent_clip_node_, NODE_CLIP, handle_parent_clip_value_);
    }
    handle_parent_clip_overridden_ = false;
  }

  handle_parent_clip_node_ = parent;
  handle_parent_clip_value_ =
      NodeManager::Instance().GetAttribute<int32_t>(parent, NODE_CLIP, 0);

  NodeManager::Instance().InsertNodeAfter(parent, start_handle_dot_node_,
                                          DrawNode());
  NodeManager::Instance().InsertNodeAfter(parent, end_handle_dot_node_,
                                          start_handle_dot_node_);
  handle_nodes_attached_ = true;
  handle_parent_ = parent;
}

void UIText::HideHandleNodes() {
  if (handle_parent_clip_overridden_ && handle_parent_clip_node_) {
    // Restore parent clipping after selection handles are hidden.
    auto current_parent = NodeManager::Instance().GetParent(DrawNode());
    if (current_parent && current_parent == handle_parent_clip_node_) {
      NodeManager::Instance().SetAttributeWithNumberValue(
          handle_parent_clip_node_, NODE_CLIP, handle_parent_clip_value_);
    }
    handle_parent_clip_overridden_ = false;
  }
  if (start_handle_dot_node_) {
    NodeManager::Instance().SetAttributeWithNumberValue(
        start_handle_dot_node_, NODE_VISIBILITY,
        static_cast<int32_t>(ARKUI_VISIBILITY_HIDDEN));
  }
  if (end_handle_dot_node_) {
    NodeManager::Instance().SetAttributeWithNumberValue(
        end_handle_dot_node_, NODE_VISIBILITY,
        static_cast<int32_t>(ARKUI_VISIBILITY_HIDDEN));
  }
}

void UIText::UpdateHandleNodes() {
  if (!HasValidSelection()) {
    HideHandleNodes();
    return;
  }
  if (!show_start_handle_ && !show_end_handle_) {
    HideHandleNodes();
    return;
  }

  EnsureHandleNodesAttached();
  if (!handle_nodes_attached_) {
    return;
  }
  if (!start_handle_dot_node_ || !end_handle_dot_node_) {
    handle_nodes_attached_ = false;
    handle_parent_ = nullptr;
    return;
  }

  const auto handles = GetHandlesInfo();
  if (handles.size() < 2 || handles[0].size() < 3 || handles[1].size() < 3) {
    HideHandleNodes();
    return;
  }

  float density = context_ ? context_->ScaledDensity() : 1.f;
  if (density <= 0.f) {
    density = 1.f;
  }

  const float dot_diameter_vp =
      handle_dot_size_vp_ > 0.f ? handle_dot_size_vp_ : 15.f;
  const float response_radius_vp = 20.f;
  // Expand the handle node size to include extra hit area for pan gestures.
  dot_node_size_vp_ = dot_diameter_vp + response_radius_vp * 2.f;
  dot_radius_px_ = (dot_diameter_vp / 2.f) * density;

  const float border_left = GetBorderLeftWidth();
  const float border_top = GetBorderTopWidth();
  const float para_translate_vp =
      paragraph_ ? (paragraph_->GetTranslateLeftOffset() / density) : 0.f;

  // Handle coordinates are dot centers in paragraph-local px (without
  // padding/border).
  const float begin_center_x_px = handles[0][0];
  const float begin_center_y_px = handles[0][1];
  const float end_center_x_px = handles[1][0];
  const float end_center_y_px = handles[1][1];

  const float begin_center_vp_x = begin_center_x_px / density +
                                  para_translate_vp + padding_left_ +
                                  border_left;
  const float begin_center_vp_y =
      begin_center_y_px / density + padding_top_ + border_top;

  const float end_center_vp_x = end_center_x_px / density + para_translate_vp +
                                padding_left_ + border_left;
  const float end_center_vp_y =
      end_center_y_px / density + padding_top_ + border_top;

  const bool start_node_is_begin = select_start_ <= select_end_;
  if (start_node_is_begin) {
    start_dot_left_vp_ = left_ + (begin_center_vp_x - dot_node_size_vp_ / 2.f);
    start_dot_top_vp_ = top_ + (begin_center_vp_y - dot_node_size_vp_ / 2.f);
    end_dot_left_vp_ = left_ + (end_center_vp_x - dot_node_size_vp_ / 2.f);
    end_dot_top_vp_ = top_ + (end_center_vp_y - dot_node_size_vp_ / 2.f);
  } else {
    start_dot_left_vp_ = left_ + (end_center_vp_x - dot_node_size_vp_ / 2.f);
    start_dot_top_vp_ = top_ + (end_center_vp_y - dot_node_size_vp_ / 2.f);
    end_dot_left_vp_ = left_ + (begin_center_vp_x - dot_node_size_vp_ / 2.f);
    end_dot_top_vp_ = top_ + (begin_center_vp_y - dot_node_size_vp_ / 2.f);
  }

  if (handle_parent_clip_node_ && !handle_parent_clip_overridden_ &&
      handle_parent_clip_value_ != 0) {
    // Handles may extend outside the text node bounds; disable clipping while
    // visible.
    NodeManager::Instance().SetAttributeWithNumberValue(
        handle_parent_clip_node_, NODE_CLIP, 0);
    handle_parent_clip_overridden_ = true;
  }

  NodeManager::Instance().SetAttributeWithNumberValue(
      start_handle_dot_node_, NODE_WIDTH, dot_node_size_vp_);
  NodeManager::Instance().SetAttributeWithNumberValue(
      start_handle_dot_node_, NODE_HEIGHT, dot_node_size_vp_);
  NodeManager::Instance().SetAttributeWithNumberValue(
      start_handle_dot_node_, NODE_POSITION, start_dot_left_vp_,
      start_dot_top_vp_);
  const int32_t dot_size_px =
      static_cast<int32_t>(std::lround(dot_node_size_vp_ * density));
  const int32_t start_left_px =
      static_cast<int32_t>(std::lround(start_dot_left_vp_ * density));
  const int32_t start_top_px =
      static_cast<int32_t>(std::lround(start_dot_top_vp_ * density));
  NodeManager::Instance().SetMeasuredSize(start_handle_dot_node_, dot_size_px,
                                          dot_size_px);
  NodeManager::Instance().SetLayoutPosition(start_handle_dot_node_,
                                            start_left_px, start_top_px);
  NodeManager::Instance().SetAttributeWithNumberValue(
      start_handle_dot_node_, NODE_VISIBILITY,
      static_cast<int32_t>(show_start_handle_ ? ARKUI_VISIBILITY_VISIBLE
                                              : ARKUI_VISIBILITY_HIDDEN));

  NodeManager::Instance().SetAttributeWithNumberValue(
      end_handle_dot_node_, NODE_WIDTH, dot_node_size_vp_);
  NodeManager::Instance().SetAttributeWithNumberValue(
      end_handle_dot_node_, NODE_HEIGHT, dot_node_size_vp_);
  NodeManager::Instance().SetAttributeWithNumberValue(
      end_handle_dot_node_, NODE_POSITION, end_dot_left_vp_, end_dot_top_vp_);
  const int32_t end_left_px =
      static_cast<int32_t>(std::lround(end_dot_left_vp_ * density));
  const int32_t end_top_px =
      static_cast<int32_t>(std::lround(end_dot_top_vp_ * density));
  NodeManager::Instance().SetMeasuredSize(end_handle_dot_node_, dot_size_px,
                                          dot_size_px);
  NodeManager::Instance().SetLayoutPosition(end_handle_dot_node_, end_left_px,
                                            end_top_px);
  NodeManager::Instance().SetAttributeWithNumberValue(
      end_handle_dot_node_, NODE_VISIBILITY,
      static_cast<int32_t>(show_end_handle_ ? ARKUI_VISIBILITY_VISIBLE
                                            : ARKUI_VISIBILITY_HIDDEN));

  NodeManager::Instance().Invalidate(start_handle_dot_node_);
  NodeManager::Instance().Invalidate(end_handle_dot_node_);
}

void UIText::HandlePanOnHandleNode(bool is_start, ArkUI_GestureEvent* event) {
  if (!event) {
    return;
  }
  if (!enable_text_selection_ || enable_custom_text_selection_ ||
      !is_in_selection_ || !paragraph_) {
    return;
  }
  auto raw_event = OH_ArkUI_GestureEvent_GetRawInputEvent(event);
  if (!raw_event) {
    return;
  }
  auto action = OH_ArkUI_GestureEvent_GetActionType(event);
  if (action & GESTURE_EVENT_ACTION_ACCEPT) {
    handle_pan_accept_x_ = is_start ? start_dot_left_vp_ : end_dot_left_vp_;
    handle_pan_accept_y_ = is_start ? start_dot_top_vp_ : end_dot_top_vp_;
  }

  float density = context_ ? context_->ScaledDensity() : 1.f;
  if (density <= 0.f) {
    density = 1.f;
  }

  float x_px = OH_ArkUI_PointerEvent_GetX(raw_event);
  float y_px = OH_ArkUI_PointerEvent_GetY(raw_event);

  // Convert from handle node's local to UIText's paragraph local.
  float local_x =
      (handle_pan_accept_x_ - padding_left_ - GetBorderLeftWidth() - left_) *
          density +
      x_px - paragraph_->GetTranslateLeftOffset();
  float local_y =
      (handle_pan_accept_y_ - padding_top_ - GetBorderTopWidth() - top_) *
          density +
      y_px;

  if ((action & GESTURE_EVENT_ACTION_ACCEPT) ||
      (action & GESTURE_EVENT_ACTION_UPDATE)) {
    if (is_start) {
      AdjustStartPosition(local_x, local_y);
    } else {
      AdjustEndPosition(local_x, local_y);
    }
  } else if (action & GESTURE_EVENT_ACTION_END) {
    PerformEndSelection(local_x, local_y);
  }
}

void UIText::DrawHandleNode(OH_Drawing_Canvas* canvas, int32_t type) const {
  if (!canvas) {
    return;
  }
  if (!HasValidSelection()) {
    return;
  }
  if ((type == static_cast<int32_t>(HandleNodeType::kStartDot) &&
       !show_start_handle_) ||
      (type == static_cast<int32_t>(HandleNodeType::kEndDot) &&
       !show_end_handle_)) {
    return;
  }

  float density = context_ ? context_->ScaledDensity() : 1.f;
  if (density <= 0.f) {
    density = 1.f;
  }
  const float size_px = dot_node_size_vp_ * density;
  const float cx = size_px / 2.f;
  const float cy = size_px / 2.f;

  if (dot_radius_px_ <= 0.f) {
    return;
  }

  // Draw a hollow ring for the selection handle dot.
  const float ring_width_px = 2.f * density;
  OH_Drawing_Pen* pen = OH_Drawing_PenCreate();
  OH_Drawing_PenSetAntiAlias(pen, true);
  OH_Drawing_PenSetColor(pen, handle_color_argb_);
  OH_Drawing_PenSetWidth(pen, ring_width_px);
  OH_Drawing_CanvasAttachPen(canvas, pen);

  OH_Drawing_Path* path = OH_Drawing_PathCreate();
  const float left = cx - dot_radius_px_;
  const float top = cy - dot_radius_px_;
  const float right = cx + dot_radius_px_;
  const float bottom = cy + dot_radius_px_;
  OH_Drawing_PathMoveTo(path, right, cy);
  // Some implementations treat a 360-degree sweep as a no-op; use two
  // 180-degree arcs.
  OH_Drawing_PathArcTo(path, left, top, right, bottom, 0, 180);
  OH_Drawing_PathArcTo(path, left, top, right, bottom, 180, 180);
  OH_Drawing_PathClose(path);
  OH_Drawing_CanvasDrawPath(canvas, path);

  OH_Drawing_CanvasDetachPen(canvas);
  OH_Drawing_PathDestroy(path);
  OH_Drawing_PenDestroy(pen);
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
