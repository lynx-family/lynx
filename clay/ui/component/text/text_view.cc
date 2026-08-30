// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/text/text_view.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "base/include/fml/task_runner.h"
#include "clay/fml/logging.h"
#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/geometry/float_rect.h"
#include "clay/public/value.h"
#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/common/text_input_type_traits.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/component_constants.h"
#include "clay/ui/component/keywords.h"
#include "clay/ui/component/overlay_view.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/scroll_view.h"
#include "clay/ui/component/selection_handle_view.h"
#include "clay/ui/component/text/inline_text_view.h"
#include "clay/ui/component/view.h"
#include "clay/ui/gesture/long_press_gesture_recognizer.h"
#include "clay/ui/gesture/multi_tap_gesture_recognizer.h"
#include "clay/ui/lynx_module/type_utils.h"
#include "clay/ui/rendering/render_scroll.h"

namespace clay {

namespace {

LYNX_UI_METHOD_BEGIN(TextView) {
  LYNX_UI_METHOD(TextView, getTextBoundingRect);
  LYNX_UI_METHOD(TextView, setTextSelection);
  LYNX_UI_METHOD(TextView, getSelectedText);
}
LYNX_UI_METHOD_END(TextView);

constexpr int kDefaultContentDistance = 12;

// Hot key tags.
constexpr uint32_t kTagCommand = 1;
constexpr uint32_t kTagControl = 1 << 1;

uint32_t AddTag(uint32_t value, uint32_t tag) { return value | tag; }

uint32_t RemoveTag(uint32_t value, uint32_t tag) { return value & ~tag; }

bool IsHighSurrogate(char16_t code_unit) {
  return code_unit >= 0xD800 && code_unit <= 0xDBFF;
}

bool IsLowSurrogate(char16_t code_unit) {
  return code_unit >= 0xDC00 && code_unit <= 0xDFFF;
}

bool IsParagraphSeparator(char16_t code_unit) {
  return code_unit == u'\n' || code_unit == u'\r' || code_unit == u'\u2028' ||
         code_unit == u'\u2029';
}

#if defined(OS_WIN) || defined(OS_OSX)
TextRange ExpandToSurrogatePairBoundaries(const std::u16string& text,
                                          const TextRange& range) {
  size_t start = std::min(range.start(), text.size());
  size_t end = std::min(range.end(), text.size());
  if (start > 0 && start < text.size() && IsLowSurrogate(text[start]) &&
      IsHighSurrogate(text[start - 1])) {
    --start;
  }
  if (end > 0 && end < text.size() && IsHighSurrogate(text[end - 1]) &&
      IsLowSurrogate(text[end])) {
    ++end;
  }
  return TextRange(start, end);
}
#endif

bool IsHandledByTextShadowNode(KeywordID kw) {
  switch (kw) {
    case KeywordID::kFontSize:
    case KeywordID::kTextOverflow:
    case KeywordID::kFontWeight:
    case KeywordID::kFontStyle:
    case KeywordID::kLineHeight:
    case KeywordID::kLineSpacing:
    case KeywordID::kLetterSpacing:
    case KeywordID::kFontFamily:
    case KeywordID::kTextAlign:
    case KeywordID::kTextDecoration:
    case KeywordID::kDirection:
    case KeywordID::kTextShadow:
    case KeywordID::kTextStrokeColor:
    case KeywordID::kTextStrokeWidth:
    case KeywordID::kEnableFontScaling:
    case KeywordID::kTextMaxline:
    case KeywordID::kWordBreak:
    case KeywordID::kWhiteSpace:
    case KeywordID::kTextIndent:
    case KeywordID::kTextMaxlength:
    case KeywordID::kXAutoFontSize:
    case KeywordID::kXAutoFontSizePresetSizes:
    case KeywordID::kTextSingleLineVerticalAlign:
    case KeywordID::kRichtype:
    case KeywordID::kText:
      return true;
    default:
      return false;
  }
}

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

static clay::Value::Map CreateHandleMap(float x, float y, float radius) {
  clay::Value::Map map;
  map["x"] = clay::Value(x);
  map["y"] = clay::Value(y);
  map["radius"] = clay::Value(radius);
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
      if (!custom_text_selection_) {
        ResetGestureRecognizers();
      }
    } else {
      ClearGestureRecognizers();
    }
  } else if (kw == KeywordID::kCustomContextMenu) {
    custom_context_menu_ = attribute_utils::GetBool(value);
  } else if (kw == KeywordID::kCustomTextSelection) {
    custom_text_selection_ = attribute_utils::GetBool(value);
  } else if (kw == KeywordID::kSelectionHandleColor) {
    SetSelectionHandleColor(value.IsString()
                                ? attribute_utils::GetColor(value)
                                : Color(attribute_utils::GetUint(value, 0)));
  } else if (kw == KeywordID::kSelectionHandleSize) {
    SetSelectionHandleSize(attribute_utils::GetNum(value));
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
  } else if (IsHandledByTextShadowNode(kw)) {
    // Text style attributes are consumed by TextShadowNode.
    return;
  } else {
    BaseView::SetAttribute(attr, value);
  }
}

void TextView::SetBorderWidth(std::vector<Side> sides,
                              std::vector<float> widths) {
  BaseView::SetBorderWidth(sides, widths);
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

void TextView::ClearInlineIndexes() {
  inline_images_index_.clear();
  inline_views_index_.clear();
}

void TextView::SetInlineEmojiInfo(
    std::vector<InlineEmojiInfo> inline_emoji_info) {
  GetRenderText()->SetInlineEmojiInfo(std::move(inline_emoji_info));
}

void TextView::SetColor(Color color) {
  if (IsTransitionAnimationReady() &&
      TransitionMgr()->Enabled(ClayAnimationPropertyType::kColor) &&
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
#if defined(OS_WIN) || defined(OS_OSX)
  RemoveGestureRecognizer(multi_tap_recognizer_);
  multi_tap_recognizer_ = nullptr;
  RemoveGestureRecognizer(drag_recognizer_);
  drag_recognizer_ = nullptr;
#elif defined(OS_ANDROID) || defined(OS_IOS)
  RemoveGestureRecognizer(long_press_recognizer_);
  RemoveGestureRecognizer(double_tap_recognizer_);
  double_tap_recognizer_ = nullptr;
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
      auto range = GetRenderText()->GetSelection();
      UpdateSelectionRange(range.start(), range.end());
      OnSelectionChanged(range.start(), range.end());
      break;
    }
    default:
      break;
  }
}

void TextView::ResetGestureRecognizers() {
  ClearGestureRecognizers();
#if defined(OS_WIN) || defined(OS_OSX)
  auto multi_tap_recognizer = std::make_unique<MultiTapGestureRecognizer>(
      page_view()->gesture_manager());
  multi_tap_recognizer->SetDelegate(this);
  multi_tap_recognizer_ = multi_tap_recognizer.get();
  multi_tap_recognizer->SetMultiTapCallback([this](const PointerEvent& up_event,
                                                   int tap_counts) {
    auto point = GetPointBySelf(up_event.position);
    point.Move(-BorderLeft() - PaddingLeft(), -BorderTop() - PaddingTop());
    RequestFocus();
    auto render_text = GetRenderText();
    auto glyph_pos = render_text->GetPainter()->GetGlyphPositionAtCoordinate(
        point.x(), point.y());
    TextRange range(glyph_pos.first, glyph_pos.first);
    if (tap_counts == 1) {
      render_text->SetSelection(range);
    } else if (tap_counts == 2) {
      range = SelectWord(glyph_pos.first, glyph_pos.second);
    } else if (tap_counts >= 3) {
      range = SelectParagraph(glyph_pos.first, glyph_pos.second);
    }
    UpdateSelectionRange(range.start(), range.end());
    OnSelectionChanged(range.start(), range.end());
  });
  auto drag_recognizer =
      std::make_unique<DragGestureRecognizer>(page_view()->gesture_manager());
  drag_recognizer->SetDelegate(this);
  drag_recognizer->SetTouchSlop(1);
  drag_recognizer_ = drag_recognizer.get();
  drag_recognizer->SetDragDownCallback([this](const PointerEvent& event) {
    RequestFocus();
    drag_start_point_ = event.position;
  });
  drag_recognizer->SetDragStartCallback([this](const FloatPoint& event) {
    PerformBeginSelection(drag_start_point_);
    PerformMoveSelection(event);
  });
  drag_recognizer->SetDragUpdateCallback(
      [this](const FloatPoint& event, const FloatSize& delta) {
        PerformMoveSelection(event);
      });
  drag_recognizer->SetDragCancelCallback(
      [this]() { PerformCancelSelection(); });
  AddGestureRecognizer(std::move(drag_recognizer));
  AddGestureRecognizer(std::move(multi_tap_recognizer));
#elif defined(OS_ANDROID) || defined(OS_IOS)
#ifndef ENABLE_CLAY_LITE
  auto double_tap_recognizer = std::make_unique<MultiTapGestureRecognizer>(
      page_view()->gesture_manager());
  double_tap_recognizer_ = double_tap_recognizer.get();
  double_tap_recognizer->SetMultiTapCallback(
      [this](const PointerEvent& up_event, int tap_counts) {
        if (tap_counts == 2) {
          auto point = up_event.position - BoundsRelativeTo(nullptr).location();
          RequestFocus();
          auto render_text = GetRenderText();
          auto glyph_pos =
              render_text->GetPainter()->GetGlyphPositionAtCoordinate(
                  point.x(), point.y());
          auto word = SelectWord(glyph_pos.first);
          render_text->SetSelection(word);
          UpdateSelectionRange(word.start(), word.end());
          OnSelectionChanged(word.start(), word.end());
          ShowSelectionHandle();
          if (!custom_context_menu_) {
            ShowSelectionPopup();
          }
        }
      });
  auto long_press_recognizer = std::make_unique<LongPressGestureRecognizer>(
      page_view()->gesture_manager());
  long_press_recognizer_ = long_press_recognizer.get();
  long_press_recognizer->SetLongPressStartCallback(
      [this](const PointerEvent& event) {
        RequestFocus();
        GetRenderText()->SetAllSelection();
        auto range = GetRenderText()->GetSelection();
        UpdateSelectionRange(range.start(), range.end());
        OnSelectionChanged(range.start(), range.end());
        ShowSelectionHandle();
        if (!custom_context_menu_) {
          ShowSelectionPopup();
        }
      });
  AddGestureRecognizer(std::move(long_press_recognizer));
  AddGestureRecognizer(std::move(double_tap_recognizer));
#endif
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

TextRange TextView::SelectWord(size_t pos) {
  auto painter = GetRenderText()->GetPainter();
  auto word_range = painter->GetWordBoundary(pos);
#if defined(OS_WIN) || defined(OS_OSX)
  word_range =
      ExpandToSurrogatePairBoundaries(GetRenderText()->GetText(), word_range);
#endif
  GetRenderText()->SetSelection(word_range);
  return word_range;
}

TextRange TextView::SelectWord(size_t pos, Affinity affinity) {
  const auto& text = GetRenderText()->GetText();
  if (affinity == Affinity::kUpstream && pos > 0) {
    --pos;
    if (pos > 0 && pos < text.size() && IsLowSurrogate(text[pos]) &&
        IsHighSurrogate(text[pos - 1])) {
      --pos;
    }
  }
  return SelectWord(pos);
}

TextRange TextView::SelectLine(size_t pos) {
  return SelectLine(pos, Affinity::kUpstream);
}

TextRange TextView::SelectLine(size_t pos, Affinity affinity) {
  auto line_range =
      GetRenderText()->GetPainter()->GetLineRangeForPosition(pos, affinity);
  GetRenderText()->SetSelection(line_range);
  return line_range;
}

TextRange TextView::SelectParagraph(size_t pos) {
  return SelectParagraph(pos, Affinity::kUpstream);
}

TextRange TextView::SelectParagraph(size_t pos, Affinity affinity) {
  const auto& text = GetRenderText()->GetText();
  if (text.empty()) {
    TextRange empty_range(0, 0);
    GetRenderText()->SetSelection(empty_range);
    return empty_range;
  }

  size_t offset = std::min(pos, text.size());
  if (affinity == Affinity::kUpstream && offset > 0) {
    --offset;
  } else if (offset == text.size()) {
    --offset;
  }
  if (offset > 0 && text[offset] == u'\n' && text[offset - 1] == u'\r') {
    --offset;
  }

  size_t start = offset;
  while (start > 0 && !IsParagraphSeparator(text[start - 1])) {
    --start;
  }

  size_t end = offset;
  while (end < text.size() && !IsParagraphSeparator(text[end])) {
    ++end;
  }
  if (end < text.size()) {
    if (text[end] == u'\r' && end + 1 < text.size() && text[end + 1] == u'\n') {
      end += 2;
    } else {
      ++end;
    }
  }

  TextRange paragraph_range(start, end);
  GetRenderText()->SetSelection(paragraph_range);
  return paragraph_range;
}

void TextView::UpdateSelectionRange(int selection_start, int selection_end) {
  selection_direction_forward_ =
      selection_start_pos_ == -1 ? selection_end > selection_start
                                 : (selection_start_pos_ < selection_start ||
                                    selection_end_pos_ < selection_end);
  selection_start_pos_ = selection_start;
  selection_end_pos_ = selection_end;
}

void TextView::PerformBeginSelection(FloatPoint point) {
#ifndef ENABLE_CLAY_LITE
#if defined(OS_WIN) || defined(OS_OSX)
  point = GetPointBySelf(point);
  point.Move(-BorderLeft() - PaddingLeft(), -BorderTop() - PaddingTop());
#else
  point -= BoundsRelativeTo(nullptr).location();
#endif
  auto selection_start =
      GetRenderText()
          ->GetPainter()
          ->GetGlyphPositionAtCoordinate(point.x(), point.y())
          .first;
  UpdateSelectionRange(selection_start, selection_start);
  auto range = TextRange(selection_start_pos_, selection_end_pos_);
  GetRenderText()->SetSelection(range);
  OnSelectionChanged(range.start(), range.end());
#endif
}

void TextView::PerformMoveSelection(FloatPoint point,
                                    SelectionHandleView* handle_bar) {
#ifndef ENABLE_CLAY_LITE
#if defined(OS_WIN) || defined(OS_OSX)
  point = GetPointBySelf(point);
  point.Move(-BorderLeft() - PaddingLeft(), -BorderTop() - PaddingTop());
#else
  point -= BoundsRelativeTo(nullptr).location();
#endif
  auto selection_end = GetRenderText()
                           ->GetPainter()
                           ->GetGlyphPositionAtCoordinate(point.x(), point.y())
                           .first;
  UpdateSelectionRange(selection_start_pos_, selection_end);
  auto render_text = GetRenderText();
  auto range = TextRange(std::min(selection_start_pos_, selection_end_pos_),
                         std::max(selection_start_pos_, selection_end_pos_));
  render_text->SetSelection(range);
  OnSelectionChanged(range.start(), range.end());
  auto text_box = render_text->GetEndTextPositionTopAndBottom();
  BringIntoView(&text_box);
#endif
}

void TextView::PerformCancelSelection() {
  HideSelectionPopup();
  HideSelectionHandle();
}

void TextView::OnSelectionChanged(int selection_start, int selection_end) {
  if (!is_text_selection_) {
    return;
  }
  const char* direction = selection_direction_forward_ ? "forward" : "backward";
  page_view()->SendEvent(id(), event_attr::kEventSelectionChange,
                         {"start", "end", "direction"},
                         std::min(selection_start, selection_end),
                         std::max(selection_start, selection_end), direction);
}

void TextView::setTextSelection(const LynxModuleValues& args,
                                const LynxUIMethodCallback& callback) {
  int start_x = 0, start_y = 0, end_x = 0, end_y = 0;
  bool show_start_handle = true, show_end_handle = true;
  CastNamedLynxModuleArgs(
      {"startX", "startY", "endX", "endY", "showStartHandle", "showEndHandle"},
      args, start_x, start_y, end_x, end_y, show_start_handle, show_end_handle);
  auto render_text = GetRenderText();
  auto start_index = render_text->GetPainter()
                         ->GetGlyphPositionAtCoordinate(start_x, start_y)
                         .first;
  auto end_index = render_text->GetPainter()
                       ->GetGlyphPositionAtCoordinate(end_x, end_y)
                       .first;
  UpdateSelectionRange(start_index, end_index);
  selection_start_pos_ = std::min(start_index, end_index);
  selection_end_pos_ = std::max(start_index, end_index);
  const auto range = TextRange(selection_start_pos_, selection_end_pos_);
  render_text->SetSelection(range);
  OnSelectionChanged(range.start(), range.end());
  ShowSelectionHandle(show_start_handle, show_end_handle);

  const auto& line_rects =
      GetRenderText()->GetTextLineRects(range.start(), range.end());
  const auto& bounding_rect = page_view()->ConvertTo<kPixelTypeLogical>(
      GetRenderText()->GetTextBoundingRect(range.start(), range.end(),
                                           line_rects));
  clay::Value::Map result;
  result["boundingRect"] = clay::Value(CreateRectMap(bounding_rect));
  clay::Value::Array box_array(line_rects.size());
  for (size_t i = 0; i < line_rects.size(); i++) {
    box_array[i] = clay::Value(CreateRectMap(
        page_view()->ConvertTo<kPixelTypeLogical>(line_rects[i])));
  }
  clay::Value::Array handle_array(2);
  if (!line_rects.empty()) {
    float handle_radius = kSelectionHandleRadius;
#ifndef ENABLE_CLAY_LITE
    if (selection_handle_size_ > 0) {
      handle_radius = selection_handle_size_ / 2;
    }
#endif
    auto start_handle_x =
        page_view()->ConvertTo<kPixelTypeLogical>(line_rects.front().left()) -
        1;
    auto start_handle_y =
        page_view()->ConvertTo<kPixelTypeLogical>(line_rects.front().top()) -
        handle_radius;
    auto end_handle_x =
        page_view()->ConvertTo<kPixelTypeLogical>(line_rects.back().right()) +
        1;
    auto end_handle_y =
        page_view()->ConvertTo<kPixelTypeLogical>(line_rects.back().bottom()) +
        handle_radius;
    handle_array[0] = clay::Value(
        CreateHandleMap(start_handle_x, start_handle_y, handle_radius));
    handle_array[1] =
        clay::Value(CreateHandleMap(end_handle_x, end_handle_y, handle_radius));
  } else {
    handle_array[0] = clay::Value();
    handle_array[1] = clay::Value();
  }
  result["boxes"] = clay::Value(std::move(box_array));
  result["handles"] = clay::Value(std::move(handle_array));
  callback(LynxUIMethodResult::kSuccess, clay::Value(std::move(result)));
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

void TextView::getSelectedText(const LynxUIMethodCallback& callback) {
  auto text = lynx::base::U16StringToU8(GetRenderText()->GetSelectionString());
  clay::Value::Map result;
  result["selectedText"] = clay::Value(std::move(text));
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
  auto range = TextRange(selection_end_pos_, selection_end_pos_);
  UpdateSelectionRange(range.start(), range.end());
  GetRenderText()->SetSelection(range);
  OnSelectionChanged(range.start(), range.end());
#endif
  BaseView::FocusHasChanged(focused, is_leaf);
  HideSelectionPopup();
  HideSelectionHandle();
}

BaseView* TextView::GetTopViewToAcceptEvent(const FloatPoint& position,
                                            FloatPoint* relative_position,
                                            int platform_try_hit_id) {
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
  view = GetViewAtPosition(point_by_paragraph, position, platform_try_hit_id);
  BaseView* target = view ?: this;
  return target->ShouldPassEventToNativeInherited() ? nullptr : target;
}

BaseView* TextView::GetViewAtPosition(const FloatPoint& point_by_paragraph,
                                      const FloatPoint& point_by_page,
                                      int platform_try_hit_id) {
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
    if (GetRenderText()->IsInlineEmojiPlaceholder(index)) {
      index = -1;
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
        if (!view) {
          return nullptr;
        }
        FloatPoint relative_position;
        auto top_view = view->GetTopViewToAcceptEvent(
            point_by_page, &relative_position, platform_try_hit_id);
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

std::vector<FloatPoint> TextView::GetAnchorPosition() {
#ifndef ENABLE_CLAY_LITE
  RenderText* render_text = GetRenderText();
  auto select_range = render_text->GetSelectionRange();
  std::vector<Point> end_points = render_text->GetPointsFromRangeSelection(
      select_range[0], select_range[1]);
  if (end_points.empty()) {
    return std::vector<FloatPoint>();
  }
  FML_DCHECK(selection_handle_container_);
  auto container_bounds_rect =
      selection_handle_container_->BoundsRelativeTo(nullptr);
  auto bounds_rect = BoundsRelativeTo(nullptr);
  auto start_point = FloatPoint(0 + bounds_rect.left(), 0 + bounds_rect.top());
  auto end_point =
      FloatPoint(width_ + bounds_rect.left(),
                 std::min(height_ + bounds_rect.top(), bounds_rect.bottom()));
  auto left = std::min(start_point.x(), end_point.x());
  auto top = std::min(start_point.y(), end_point.y());
  auto right = std::max(start_point.x(), end_point.x());
  auto bottom = std::max(start_point.y(), end_point.y());
  FloatRect editing_region = FloatRect(left, top, right - left, bottom - top);
  bool is_multiline =
      end_points.back().y() - end_points.front().y() >
      render_text->GetPainter()->GetLineHeightForPosition(select_range[1]) / 2;
  double mid_x = is_multiline
                     ? editing_region.width() / 2
                     : (end_points.front().x() + end_points.back().x()) / 2;
  // TODO(wangyanyi) now just not consider the iOS, because in iOS there is a
  // safe area concept
  double anchor_x =
      std::clamp(static_cast<double>(mid_x + editing_region.x()), 8.0,
                 static_cast<double>(page_view()->Width()) - 8.0);
  FloatPoint anchor_above = FloatPoint(
      anchor_x,
      std::max<float>(end_points.front().y() -
                          render_text->GetPainter()->GetLineHeightForPosition(
                              select_range[1]) +
                          editing_region.y(),
                      container_bounds_rect.top()));
  FloatPoint anchor_blow =
      FloatPoint(anchor_x, end_points.back().y() + editing_region.y());
  return std::vector<FloatPoint>{anchor_above, anchor_blow};
#else
  return std::vector<FloatPoint>();
#endif
}

void TextView::ShowSelectionPopup() {
#ifndef ENABLE_CLAY_LITE
  if (GetRenderText()->IsCollapsed()) {
    return;
  }
  HideSelectionPopup();
  if (!selection_popup_) {
    selection_popup_ = new SelectionPopupView(page_view());
    selection_popup_->SetCopyFunction([weak = weak_factory_.GetWeakPtr()]() {
      if (weak) {
        weak->HandleCopy();
      }
    });
    selection_popup_->SetSelectAllFunction(
        [weak = weak_factory_.GetWeakPtr()]() {
          if (weak) {
            weak->HandleSelectAll();
          }
        });
  }
  selection_popup_->SetAnchorOffset(GetAnchorPosition());
  selection_popup_->SetBoundsHeightAndWidth(page_view()->Width(),
                                            page_view()->Height());
  selection_popup_->BuildSelectionPopup(
      std::vector<ActionType>{ActionType::kCopy, ActionType::kSelectAll});
  page_view()->AddChild(selection_popup_);
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

FloatRect TextView::GetDisplayRect() {
  FloatRect result = BoundsRelativeTo(nullptr);
  auto parent = Parent();
  while (parent) {
    auto parent_rect = parent->BoundsRelativeTo(nullptr);
    result.Intersect(parent_rect);
    parent = parent->Parent();
  }
  return result;
}

void TextView::UpdateSelectionHandleLayout(SelectionHandleView* handle) {
#ifndef ENABLE_CLAY_LITE
  FML_DCHECK(handle);
  auto text_box = handle->GetHandleType() == TextSelectionHandleType::kLeft
                      ? GetRenderText()->GetLeftTextBox()
                      : GetRenderText()->GetRightTextBox();
  auto stroke_width = page_view()->ConvertFrom<kPixelTypeLogical>(2);
  auto handle_x = handle->GetHandleType() == TextSelectionHandleType::kLeft
                      ? text_box.GetLeft()
                      : text_box.GetRight();
  auto offset =
      FloatPoint(handle_x + scroll_offset_.x() + stroke_width,
                 text_box.GetTop() + scroll_offset_.y() + stroke_width);
  handle->BuildSelectionHandle(text_box.rect.height(), offset);
#endif
}

void TextView::UpdateSelectionHandleTypes() {
#ifndef ENABLE_CLAY_LITE
  if (selection_start_pos_ == selection_end_pos_) {
    return;
  }
  const auto start_type = selection_start_pos_ < selection_end_pos_
                              ? TextSelectionHandleType::kLeft
                              : TextSelectionHandleType::kRight;
  const auto end_type = selection_start_pos_ < selection_end_pos_
                            ? TextSelectionHandleType::kRight
                            : TextSelectionHandleType::kLeft;
  if (start_selection_handle_) {
    start_selection_handle_->SetHandleType(start_type);
    UpdateSelectionHandleLayout(start_selection_handle_);
  }
  if (end_selection_handle_) {
    end_selection_handle_->SetHandleType(end_type);
    UpdateSelectionHandleLayout(end_selection_handle_);
  }
#endif
}

void TextView::ShowSelectionHandle(bool show_start_handle,
                                   bool show_end_handle) {
#ifndef ENABLE_CLAY_LITE
  if (GetRenderText()->IsCollapsed()) {
    return;
  }
  HideSelectionHandle();
  auto stroke_width = page_view()->ConvertFrom<kPixelTypeLogical>(2);
  if (!start_selection_handle_ && show_start_handle) {
    const auto start_type = selection_start_pos_ <= selection_end_pos_
                                ? TextSelectionHandleType::kLeft
                                : TextSelectionHandleType::kRight;
    start_selection_handle_ = new SelectionHandleView(page_view(), start_type);
    start_selection_handle_->SetSelectionHandleColor(selection_handle_color_);
    start_selection_handle_->SetSelectionHandleSize(selection_handle_size_);
    start_selection_handle_->SetHandleMove(
        [this](const FloatPoint& position, SelectionHandleView* view) {
          UpdateSelectionHandle(position, view);
          if (!custom_context_menu_) {
            ShowSelectionPopup();
          }
        });
    start_selection_handle_->SetHandleDragDown(
        [this](const PointerEvent& event, SelectionHandleView* view) {
          BeginSelectionHandleDrag(event, view);
        });
    start_selection_handle_->SetHandleDragEnd(
        [this](SelectionHandleView* view) { EndSelectionHandleDrag(view); });
    UpdateSelectionHandleLayout(start_selection_handle_);
  }
  if (!end_selection_handle_ && show_end_handle) {
    const auto end_type = selection_start_pos_ <= selection_end_pos_
                              ? TextSelectionHandleType::kRight
                              : TextSelectionHandleType::kLeft;
    end_selection_handle_ = new SelectionHandleView(page_view(), end_type);
    end_selection_handle_->SetSelectionHandleColor(selection_handle_color_);
    end_selection_handle_->SetSelectionHandleSize(selection_handle_size_);
    end_selection_handle_->SetHandleMove(
        [this](const FloatPoint& position, SelectionHandleView* view) {
          UpdateSelectionHandle(position, view);
          if (!custom_context_menu_) {
            ShowSelectionPopup();
          }
        });
    end_selection_handle_->SetHandleDragDown(
        [this](const PointerEvent& event, SelectionHandleView* view) {
          BeginSelectionHandleDrag(event, view);
        });
    end_selection_handle_->SetHandleDragEnd(
        [this](SelectionHandleView* view) { EndSelectionHandleDrag(view); });
    UpdateSelectionHandleLayout(end_selection_handle_);
  }
  if (!start_selection_handle_ && !end_selection_handle_) {
    return;
  }
  if (!selection_handle_container_) {
    selection_handle_container_ =
        new OverlayView(-1, "handle_container", page_view());
    selection_handle_container_->SetOverflow(CSSProperty::OVERFLOW_XY);
    page_view()->AddChild(selection_handle_container_);
    auto display_rect = GetDisplayRect();
    selection_handle_container_->SetBound(
        display_rect.left() - stroke_width, display_rect.top() - stroke_width,
        display_rect.width() + 2 * stroke_width,
        display_rect.height() + 2 * stroke_width);
  }
  if (start_selection_handle_) {
    selection_handle_container_->AddChild(start_selection_handle_);
  }
  if (end_selection_handle_) {
    selection_handle_container_->AddChild(end_selection_handle_);
  }
#endif
}

void TextView::BeginSelectionHandleDrag(const PointerEvent& event,
                                        SelectionHandleView* handle_bar) {
#ifndef ENABLE_CLAY_LITE
  auto render_text = GetRenderText();
  FloatPoint endpoint;
  if (handle_bar->GetHandleType() == TextSelectionHandleType::kLeft) {
    auto text_box = render_text->GetLeftTextBox();
    endpoint = FloatPoint(text_box.GetLeft(), text_box.GetTop());
  } else {
    auto text_box = render_text->GetRightTextBox();
    endpoint = FloatPoint(text_box.GetRight(), text_box.GetBottom());
  }
  auto pointer = event.position - BoundsRelativeTo(nullptr).location();
  selection_handle_drag_offset_ = pointer - endpoint;
  dragging_selection_handle_ = handle_bar;
#endif
}

void TextView::EndSelectionHandleDrag(SelectionHandleView* handle_bar) {
#ifndef ENABLE_CLAY_LITE
  if (dragging_selection_handle_ == handle_bar) {
    auto range = GetRenderText()->GetSelection();
    OnSelectionChanged(range.start(), range.end());
    dragging_selection_handle_ = nullptr;
    selection_handle_drag_offset_ = FloatPoint();
  }
#endif
}

void TextView::UpdateSelectionHandle(FloatPoint point,
                                     SelectionHandleView* handle_bar) {
#ifndef ENABLE_CLAY_LITE
  FML_DCHECK(handle_bar);
  FML_DCHECK(dragging_selection_handle_ == handle_bar);
  auto render_text = GetRenderText();
  point -= BoundsRelativeTo(nullptr).location();
  point -= selection_handle_drag_offset_;
  auto glyph_pos = render_text->GetPainter()->GetGlyphPositionAtCoordinate(
      point.x(), point.y());
  auto focus_pos = glyph_pos.first;
  const bool updating_start = handle_bar == start_selection_handle_;
  const int fixed_pos =
      updating_start ? selection_end_pos_ : selection_start_pos_;
  const int previous_focus_pos =
      updating_start ? selection_start_pos_ : selection_end_pos_;
  if (fixed_pos >= 0 &&
      focus_pos == static_cast<decltype(focus_pos)>(fixed_pos)) {
    const auto text_length = render_text->GetText().length();
    bool expand_left = previous_focus_pos < fixed_pos;
    if (previous_focus_pos == fixed_pos) {
      auto fixed_x = updating_start ? render_text->GetRightTextBox().GetRight()
                                    : render_text->GetLeftTextBox().GetLeft();
      expand_left = point.x() < fixed_x;
    }
    if ((focus_pos >= text_length || expand_left) && focus_pos > 0) {
      focus_pos--;
    } else if (focus_pos < text_length) {
      focus_pos++;
    }
  }
  const int focus_index = static_cast<int>(focus_pos);
  if (handle_bar == start_selection_handle_) {
    UpdateSelectionRange(focus_index, selection_end_pos_);
  } else {
    UpdateSelectionRange(selection_start_pos_, focus_index);
  }
  render_text->SetSelection(
      TextRange(std::min(selection_start_pos_, selection_end_pos_),
                std::max(selection_start_pos_, selection_end_pos_)));
  TextBox end_box = GetRenderText()->GetEndTextPositionTopAndBottom();
  BringIntoView(&end_box);

  UpdateSelectionHandleTypes();
#endif
}

void TextView::SetSelectionHandleSize(float selection_handle_size) {
#ifndef ENABLE_CLAY_LITE
  selection_handle_size_ =
      selection_handle_size <= 0 ? 0 : selection_handle_size;
  if (start_selection_handle_) {
    start_selection_handle_->SetSelectionHandleSize(selection_handle_size_);
    UpdateSelectionHandleLayout(start_selection_handle_);
  }
  if (end_selection_handle_) {
    end_selection_handle_->SetSelectionHandleSize(selection_handle_size_);
    UpdateSelectionHandleLayout(end_selection_handle_);
  }
#endif
}

void TextView::SetSelectionHandleColor(Color selection_handle_color) {
#ifndef ENABLE_CLAY_LITE
  selection_handle_color_ = selection_handle_color;
  if (start_selection_handle_) {
    start_selection_handle_->SetSelectionHandleColor(selection_handle_color_);
  }
  if (end_selection_handle_) {
    end_selection_handle_->SetSelectionHandleColor(selection_handle_color_);
  }
#endif
}

void TextView::HideSelectionHandle() {
#ifndef ENABLE_CLAY_LITE
  dragging_selection_handle_ = nullptr;
  selection_handle_drag_offset_ = FloatPoint();
  page_view()->RemoveChild(selection_handle_container_);
  if (start_selection_handle_) {
    selection_handle_container_->RemoveChild(start_selection_handle_);
    delete start_selection_handle_;
    start_selection_handle_ = nullptr;
  }
  if (end_selection_handle_) {
    selection_handle_container_->RemoveChild(end_selection_handle_);
    delete end_selection_handle_;
    end_selection_handle_ = nullptr;
  }
  delete selection_handle_container_;
  selection_handle_container_ = nullptr;
#endif
}

void TextView::HandleCopy() {
#ifndef ENABLE_CLAY_LITE
  auto editing_text = GetRenderText()->GetSelectionString();
  page_view()->SetClipboardData(editing_text);
  auto range = TextRange(selection_end_pos_, selection_end_pos_);
  UpdateSelectionRange(range.start(), range.end());
  GetRenderText()->SetSelection(range);
  OnSelectionChanged(range.start(), range.end());
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
  auto range = GetRenderText()->GetSelection();
  UpdateSelectionRange(range.start(), range.end());
  OnSelectionChanged(range.start(), range.end());
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
            scroll_view->GetScrollOffset().y() + kDefaultContentDistance,
        text_box->GetTop() - scroll_view->GetScrollOffset().y());
    target_offset = std::clamp(
        (additional_offset + scroll_view->GetScrollOffset().y()), 0.f,
        static_cast<RenderScroll*>(scroll_view->render_object())
            ->MaxScrollHeight());
#ifndef ENABLE_CLAY_LITE
    if (start_selection_handle_) {
      start_selection_handle_->SetScrollOffset(FloatPoint(0, target_offset));
    }
    if (end_selection_handle_) {
      end_selection_handle_->SetScrollOffset(FloatPoint(0, target_offset));
    }
#endif
  } else if (scroll_view->CanScrollX()) {
    auto additional_offset = std::clamp(
        static_cast<float>(0.0),
        text_box->GetRight() - scroll_view->Width() -
            scroll_view->GetScrollOffset().x() + kDefaultContentDistance,
        text_box->GetLeft() - scroll_view->GetScrollOffset().x());
    target_offset = std::clamp(
        (additional_offset + scroll_view->GetScrollOffset().x()), 0.f,
        static_cast<RenderScroll*>(scroll_view->render_object())
            ->MaxScrollWidth());
#ifndef ENABLE_CLAY_LITE
    if (start_selection_handle_) {
      start_selection_handle_->SetScrollOffset(FloatPoint(target_offset, 0));
    }
    if (end_selection_handle_) {
      end_selection_handle_->SetScrollOffset(FloatPoint(target_offset, 0));
    }
#endif
  }
  scroll_view->StopAnimation();
  scroll_view->ScrollTo(false, target_offset);
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
  // PanZoom events are trackpad two-finger scrolls. They should not be
  // captured by text-selection drag; let the enclosing scroll-view handle them.
  if (event.type == PointerEvent::EventType::kPanZoomStartEvent) {
    return false;
  }
  if (event.device == PointerEvent::DeviceType::kMouse) {
    return event.buttons == PointerEvent::MouseButton::kPrimary;
  } else {
    return true;
  }
}

void TextView::OnViewPostionUpdate(FloatPoint scroll_offset) {
#ifndef ENABLE_CLAY_LITE
  if (start_selection_handle_) {
    start_selection_handle_->UpdatePosWithScroll(scroll_offset -
                                                 scroll_offset_);
  }
  if (end_selection_handle_) {
    end_selection_handle_->UpdatePosWithScroll(scroll_offset - scroll_offset_);
  }
  if (selection_popup_) {
    selection_popup_->UpdatePosWithScroll(
        scroll_offset, selection_handle_container_->BoundsRelativeTo(nullptr));
  }
  scroll_offset_ = scroll_offset;
#endif
  BaseView::OnViewPostionUpdate(scroll_offset);
}

}  // namespace clay
