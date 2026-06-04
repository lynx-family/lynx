// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/text/text_view.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "base/include/fml/task_runner.h"
#include "base/include/string/string_utils.h"
#include "clay/fml/logging.h"
#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/geometry/float_rect.h"
#include "clay/public/value.h"
#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/common/text_input_type_traits.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/keywords.h"
#include "clay/ui/component/overlay_view.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/scroll_view.h"
#include "clay/ui/component/selection_handle_view.h"
#include "clay/ui/component/text/inline_text_view.h"
#include "clay/ui/component/text/unicode_util.h"
#include "clay/ui/component/view.h"
#include "clay/ui/component/view_context.h"
#include "clay/ui/gesture/long_press_gesture_recognizer.h"
#include "clay/ui/gesture/multi_tap_gesture_recognizer.h"
#include "clay/ui/lynx_module/type_utils.h"
#include "clay/ui/rendering/render_scroll.h"
#include "clay/ui/shadow/bundle.h"
#include "clay/ui/shadow/inline_image_shadow_node.h"
#include "clay/ui/shadow/inline_text_shadow_node.h"
#include "clay/ui/shadow/inline_view_shadow_node.h"
#include "clay/ui/shadow/raw_text_shadow_node.h"
#include "clay/ui/shadow/text_shadow_node.h"

namespace clay {

namespace {

LYNX_UI_METHOD_BEGIN(TextView) {
  LYNX_UI_METHOD(TextView, getTextBoundingRect);
  LYNX_UI_METHOD(TextView, setTextSelection);
  LYNX_UI_METHOD(TextView, getSelectedText);
}
LYNX_UI_METHOD_END(TextView);

constexpr int kDefaultContentDistance = 12;

constexpr int kDoubleTapCount = 2;
constexpr int kTripleTapCount = 3;

// Hot key tags.
constexpr uint32_t kTagCommand = 1;
constexpr uint32_t kTagControl = 1 << 1;
constexpr bool kContentEditableDebugLog = true;

class ContentEditableDebugLog {
 public:
  ContentEditableDebugLog() { stream_ << "[contenteditable] "; }
  ~ContentEditableDebugLog() {
    stream_ << '\n';
    const std::string line = stream_.str();
    static_cast<void>(std::fwrite(line.data(), line.size(), 1, stderr));
    std::fflush(stderr);
  }

  template <typename T>
  ContentEditableDebugLog& operator<<(const T& value) {
    stream_ << value;
    return *this;
  }

 private:
  std::ostringstream stream_;
};

#define CONTENT_EDITABLE_LOG \
  if constexpr (kContentEditableDebugLog) ContentEditableDebugLog()

uint32_t AddTag(uint32_t value, uint32_t tag) { return value | tag; }

uint32_t RemoveTag(uint32_t value, uint32_t tag) { return value & ~tag; }

bool IsContentEditableAttribute(const char* attr) {
  return attr && std::strcmp(attr, "contenteditable") == 0;
}

bool IsSnapshotValuesAttribute(const char* attr) {
  return attr && std::strcmp(attr, "values") == 0;
}

bool ParseContentEditableValue(const clay::Value& value) {
  if (value.IsBool()) {
    return value.GetBool();
  }
  if (value.IsString()) {
    const auto& text = value.GetString();
    return text.empty() || text == "true";
  }
  return false;
}

bool TryParseContentEditableFromSnapshotValue(const clay::Value& value,
                                              bool* enabled) {
  if (value.IsMap()) {
    const auto& map = value.GetMap();
    auto iter = map.find("contenteditable");
    if (iter != map.end()) {
      *enabled = ParseContentEditableValue(iter->second);
      return true;
    }
    return false;
  }
  if (value.IsArray()) {
    for (const auto& item : value.GetArray()) {
      if (TryParseContentEditableFromSnapshotValue(item, enabled)) {
        return true;
      }
    }
  }
  return false;
}

bool IsHighSurrogate(char16_t value) {
  return value >= 0xD800 && value <= 0xDBFF;
}

bool IsLowSurrogate(char16_t value) {
  return value >= 0xDC00 && value <= 0xDFFF;
}

std::u16string BuildEditableStreamText(const EditableInlineStream& stream) {
  std::u16string text;
  for (const auto& item : stream) {
    if (item.type == EditableInlineStreamItemType::kText) {
      text += item.text;
    } else {
      text += u"\uFFFC";
    }
  }
  return text;
}

size_t PreviousCodePointOffset(const std::u16string& text, size_t offset) {
  offset = std::min(offset, text.length());
  if (offset == 0) {
    return 0;
  }
  auto previous = offset - 1;
  if (previous > 0 && IsLowSurrogate(text[previous]) &&
      IsHighSurrogate(text[previous - 1])) {
    return previous - 1;
  }
  return previous;
}

size_t NextCodePointOffset(const std::u16string& text, size_t offset) {
  offset = std::min(offset, text.length());
  if (offset >= text.length()) {
    return text.length();
  }
  auto next = offset + 1;
  if (IsHighSurrogate(text[offset]) && next < text.length() &&
      IsLowSurrogate(text[next])) {
    return next + 1;
  }
  return next;
}

size_t MoveOffsetByCodePoints(const std::u16string& text, size_t offset,
                              int distance) {
  offset = std::min(offset, text.length());
  if (distance < 0) {
    for (int i = 0; i < -distance; ++i) {
      offset = PreviousCodePointOffset(text, offset);
    }
    return offset;
  }
  for (int i = 0; i < distance; ++i) {
    offset = NextCodePointOffset(text, offset);
  }
  return offset;
}

const char* EditableItemTypeName(EditableInlineStreamItemType type) {
  switch (type) {
    case EditableInlineStreamItemType::kText:
      return "text";
    case EditableInlineStreamItemType::kAtomicInlineImage:
      return "inline-image";
    case EditableInlineStreamItemType::kAtomicInlineView:
      return "inline-view";
  }
  return "unknown";
}

bool IsAtomicEditableItem(EditableInlineStreamItemType type) {
  return type == EditableInlineStreamItemType::kAtomicInlineImage ||
         type == EditableInlineStreamItemType::kAtomicInlineView;
}

int ShadowChildIndex(BaseTextShadowNode* parent, ShadowNode* child) {
  if (!parent || !child) {
    return -1;
  }
  const auto& children = parent->GetChildren();
  auto iter = std::find(children.begin(), children.end(), child);
  if (iter == children.end()) {
    return -1;
  }
  return static_cast<int>(std::distance(children.begin(), iter));
}

int ViewChildIndex(BaseView* parent, BaseView* child) {
  if (!parent || !child) {
    return -1;
  }
  return parent->GetChildIndex(child);
}

int ClampedInsertIndex(int index, size_t child_count) {
  if (index < 0) {
    return static_cast<int>(child_count);
  }
  return static_cast<int>(
      std::min(static_cast<size_t>(index), child_count));
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
      text_input_controller_(
          std::make_unique<TextInputController>(page_view, id, this)),
      weak_factory_(this) {}

TextView::~TextView() {
#ifndef ENABLE_CLAY_LITE
  StopContentEditableCaretBlinking();
#endif
  EndEditableInput();
  HideSelectionPopup();
}

void TextView::SetAttribute(const char* attr, const clay::Value& value) {
  bool content_editable_value = false;
  if (IsContentEditableAttribute(attr) ||
      (IsSnapshotValuesAttribute(attr) &&
       TryParseContentEditableFromSnapshotValue(value,
                                                &content_editable_value))) {
    content_editable_ = IsContentEditableAttribute(attr)
                            ? ParseContentEditableValue(value)
                            : content_editable_value;
    editable_caret_initialized_ = false;
    SetFocusable(content_editable_ || is_text_selection_);
    if (content_editable_) {
      GetRenderText()->SetSelectionChangedListener(
          [this](int selection_start, int selection_end) {
            OnSelectionChanged(selection_start, selection_end);
          });
      if (!custom_text_selection_) {
        ResetGestureRecognizers();
      }
    } else {
#ifndef ENABLE_CLAY_LITE
      StopContentEditableCaretBlinking();
#endif
      page_view()->StopInput(this);
      if (!is_text_selection_) {
        ClearGestureRecognizers();
      }
    }
    return;
  }

  auto kw = GetKeywordID(attr);
  if (kw == KeywordID::kTextSelection) {
    if ((is_text_selection_ = attribute_utils::GetBool(value))) {
      SetFocusable(true);
      if (!custom_text_selection_) {
        ResetGestureRecognizers();
      }
      GetRenderText()->SetSelectionChangedListener(
          [this](int selection_start, int selection_end) {
            OnSelectionChanged(selection_start, selection_end);
          });
    } else {
      SetFocusable(content_editable_);
      if (content_editable_) {
        GetRenderText()->SetSelectionChangedListener(
            [this](int selection_start, int selection_end) {
              OnSelectionChanged(selection_start, selection_end);
            });
        if (!custom_text_selection_) {
          ResetGestureRecognizers();
        }
      } else {
        ClearGestureRecognizers();
      }
    }
  } else if (kw == KeywordID::kCustomContextMenu) {
    custom_context_menu_ = attribute_utils::GetBool(value);
  } else if (kw == KeywordID::kCustomTextSelection) {
    custom_text_selection_ = attribute_utils::GetBool(value);
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
  } else if (kw == KeywordID::kCaretColor) {
    Color color;
    if (Color::Parse(attribute_utils::GetCString(value), &color)) {
      GetRenderText()->SetCaretColor(color);
    }
  } else {
    BaseView ::SetAttribute(attr, value);
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
  inline_images_index_[id] = placeholder_id;
}
void TextView::PushInlineViewIndex(int id, int placeholder_id) {
  inline_views_index_[id] = placeholder_id;
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

#ifndef ENABLE_CLAY_LITE
void TextView::UpdateContentEditableCaretBlinking() {
  if (!IsRuntimeContentEditable() || !IsFocused() || selection_end_pos_ < 0 ||
      selection_start_pos_ != selection_end_pos_) {
    StopContentEditableCaretBlinking();
    return;
  }
  if (!caret_blink_controller_) {
    caret_blink_controller_ = std::make_unique<CaretBlinkController>(
        page_view()->GetTaskRunner(), [this](bool display) {
          GetRenderText()->SetCaretVisible(display);
        });
  }
  caret_blink_controller_->Start();
}

void TextView::StopContentEditableCaretBlinking() {
  if (caret_blink_controller_) {
    caret_blink_controller_->Stop();
    return;
  }
  GetRenderText()->SetCaretVisible(false);
}
#endif

bool TextView::OnKeyEvent(const KeyEvent* key_event) {
  if (IsRuntimeContentEditable() && key_event) {
    CONTENT_EDITABLE_LOG << "key-event view=" << id()
                         << " type=" << static_cast<int>(key_event->GetType())
                         << " logical="
                         << static_cast<uint64_t>(key_event->GetLogical())
                         << " char_len="
                         << key_event->GetCharacter().length()
                         << " synthesized=" << key_event->GetSynthesized()
                         << " focused=" << IsFocused()
                         << " connected="
                         << (text_input_controller_ &&
                             text_input_controller_->ConnectKeyboard());
  }
  if (ApplyHotKey(key_event)) {
    return false;
  }
  if (IsRuntimeContentEditable() && HandleEditableKeyEvent(key_event)) {
    return true;
  }
  return KeyEventHandler::OnKeyEvent(key_event);
}

void TextView::OnCommitText(std::string text) {
  CONTENT_EDITABLE_LOG << "commit-text view=" << id()
                       << " utf8_len=" << text.length()
                       << " focused=" << IsFocused()
                       << " connected="
                       << (text_input_controller_ &&
                           text_input_controller_->ConnectKeyboard());
  InsertEditableText(UnicodeUtil::Utf8ToUtf16(text));
}

void TextView::OnComposingText(std::string text) {
  CONTENT_EDITABLE_LOG << "composing-text view=" << id()
                       << " utf8_len=" << text.length()
                       << " focused=" << IsFocused()
                       << " connected="
                       << (text_input_controller_ &&
                           text_input_controller_->ConnectKeyboard());
  // Keep IME pre-edit text out of the shadow source. The committed text will
  // arrive through OnCommitText or a non-composing platform editing state.
}

void TextView::OnKeyboardEvent(std::unique_ptr<KeyEvent> key_event) {
  if (key_event) {
    OnKeyEvent(key_event.get());
  }
}

void TextView::OnDeleteSurroundingText(int before_length, int after_length) {
  if (!IsRuntimeContentEditable()) {
    return;
  }

  auto* shadow_node = GetEditableTextShadowNode();
  if (!shadow_node) {
    return;
  }

  auto stream_text = GetEditableStreamText(shadow_node);
  auto stream_length = stream_text.length();
  auto selection = GetEditableSelectionRange(stream_length);
  CONTENT_EDITABLE_LOG << "delete-surrounding view=" << id()
                       << " before=" << before_length
                       << " after=" << after_length
                       << " selection=[" << selection.first << ","
                       << selection.second << "] caret="
                       << editable_caret_offset_
                       << " stream_length=" << stream_length;
  if (selection.first != selection.second) {
    ReplaceEditableRange(selection.first, selection.second, u"",
                         selection.first);
    return;
  }

  editable_caret_offset_ = selection.second;
  auto before = before_length > 0 ? before_length : 0;
  auto after = after_length > 0 ? after_length : 0;
  auto start = MoveOffsetByCodePoints(stream_text, editable_caret_offset_,
                                      -before);
  auto end =
      MoveOffsetByCodePoints(stream_text, editable_caret_offset_, after);
  if (start == end) {
    return;
  }
  CONTENT_EDITABLE_LOG << "delete-range view=" << id() << " start=" << start
                       << " end=" << end;
  ReplaceEditableRange(start, end, u"", start);
}

void TextView::OnPerformAction(KeyboardAction) {}

void TextView::OnFinishInput() {
  CONTENT_EDITABLE_LOG << "finish-input view=" << id()
                       << " focused=" << IsFocused()
                       << " connected="
                       << (text_input_controller_ &&
                           text_input_controller_->ConnectKeyboard());
  if (IsRuntimeContentEditable()) {
    EndEditableInput();
  }
}

bool TextView::InsertEditableText(const std::u16string& text) {
  if (!IsRuntimeContentEditable() || text.empty()) {
    CONTENT_EDITABLE_LOG << "insert-text skipped view=" << id()
                         << " reason=not-editable-or-empty"
                         << " len=" << text.length();
    return false;
  }

  auto* shadow_node = GetEditableTextShadowNode();
  if (!shadow_node) {
    CONTENT_EDITABLE_LOG << "insert-text skipped view=" << id()
                         << " reason=no-shadow len=" << text.length();
    return false;
  }

  auto selection =
      GetEditableSelectionRange(GetEditableStreamLength(shadow_node));
  return ReplaceEditableRange(selection.first, selection.second, text,
                              selection.first + text.length());
}

void TextView::SetEditableTextShadowNodeForTesting(TextShadowNode* node) {
  editable_text_shadow_node_for_testing_ = node;
}

void TextView::SetEditableCaretOffsetForTesting(size_t offset) {
  editable_caret_offset_ = offset;
  editable_selection_base_offset_ = offset;
  editable_selection_extent_offset_ = offset;
  editable_caret_initialized_ = true;
}

void TextView::UpdateEditingState(std::string text, TextSelection selection,
                                  TextRange composing, Affinity) {
  if (!IsRuntimeContentEditable()) {
    return;
  }
  auto* shadow_node = GetEditableTextShadowNode();
  if (!shadow_node) {
    return;
  }

  auto old_text = GetEditableStreamText(shadow_node);
  auto new_text = UnicodeUtil::Utf8ToUtf16(text);
  auto selection_base = static_cast<size_t>(
      std::clamp(selection.base_offset(), 0, static_cast<int>(new_text.length())));
  auto selection_extent = static_cast<size_t>(std::clamp(
      selection.extent_offset(), 0, static_cast<int>(new_text.length())));
  CONTENT_EDITABLE_LOG << "platform-update view=" << id()
                       << " old_len=" << old_text.length()
                       << " new_len=" << new_text.length()
                       << " platform_selection_base=" << selection_base
                       << " platform_selection_extent=" << selection_extent
                       << " composing=[" << composing.base() << ","
                       << composing.extent() << "]";

  syncing_editable_input_state_ = true;
  if (!composing.collapsed()) {
    CONTENT_EDITABLE_LOG << "platform-composing view=" << id()
                         << " source_len=" << old_text.length()
                         << " platform_len=" << new_text.length();
    UpdateEditableCaretRectToPlatform();
    syncing_editable_input_state_ = false;
    return;
  }

  if (old_text == new_text) {
    SetEditableSelectionRange(selection_base, selection_extent);
    UpdateEditableCaretRectToPlatform();
    syncing_editable_input_state_ = false;
    return;
  }

  size_t prefix = 0;
  while (prefix < old_text.length() && prefix < new_text.length() &&
         old_text[prefix] == new_text[prefix]) {
    ++prefix;
  }

  size_t suffix = 0;
  while (suffix < old_text.length() - prefix &&
         suffix < new_text.length() - prefix &&
         old_text[old_text.length() - suffix - 1] ==
             new_text[new_text.length() - suffix - 1]) {
    ++suffix;
  }

  auto old_end = old_text.length() - suffix;
  auto new_end = new_text.length() - suffix;
  auto replacement = new_text.substr(prefix, new_end - prefix);
  auto selection_offset =
      std::min(selection_extent, prefix + replacement.length());
  CONTENT_EDITABLE_LOG << "platform-diff view=" << id()
                       << " old_range=[" << prefix << "," << old_end
                       << "] new_range=[" << prefix << "," << new_end
                       << "] replacement_len=" << replacement.length()
                       << " selection_offset=" << selection_offset;
  ReplaceEditableRange(prefix, old_end, replacement, selection_offset);
  SetEditableSelectionRange(selection_base, selection_extent);
  editable_caret_offset_ =
      std::min(selection_extent, GetEditableStreamLength(shadow_node));
  editable_caret_initialized_ = true;
  UpdateEditableCaretRectToPlatform();
  syncing_editable_input_state_ = false;
}

void TextView::PerformAction() {}

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
  RemoveGestureRecognizer(multi_tap_recognizer_);
  multi_tap_recognizer_ = nullptr;
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
      if (IsRuntimeContentEditable()) {
        SetEditableSelectionRange(0,
                                  GetEditableStreamLength(
                                      GetEditableTextShadowNode()));
      } else {
        GetRenderText()->SetAllSelection();
      }
      break;
    }
    case KeyCode::kKeyZ: {
      if (IsRuntimeContentEditable()) {
        UndoEditableMutation();
      }
      break;
    }
    default:
      break;
  }
}

void TextView::ResetGestureRecognizers() {
  ClearGestureRecognizers();
#ifndef ENABLE_CLAY_LITE
  auto multi_tap_recognizer = std::make_unique<MultiTapGestureRecognizer>(
      page_view()->gesture_manager());
  multi_tap_recognizer_ = multi_tap_recognizer.get();
  multi_tap_recognizer->SetMultiTapCallback(
      [this](const PointerEvent& up_event, int tap_counts) {
        auto point = up_event.position - BoundsRelativeTo(nullptr).location();
        if (tap_counts == 1) {
#if !defined(OS_ANDROID) && !defined(OS_IOS)
          CONTENT_EDITABLE_LOG << "single-tap view=" << id() << " point=("
                               << point.x() << "," << point.y() << ")";
          MoveCaretTo(point);
#endif
          return;
        }
        if (tap_counts == kDoubleTapCount) {
          SelectWordAt(point);
#if defined(OS_ANDROID) || defined(OS_IOS)
          ShowSelectionHandle();
          if (!custom_context_menu_) {
            ShowSelectionPopup();
          }
#endif
          return;
        }
        if (tap_counts == kTripleTapCount) {
          SelectLineAt(point);
#if defined(OS_ANDROID) || defined(OS_IOS)
          ShowSelectionHandle();
          if (!custom_context_menu_) {
            ShowSelectionPopup();
          }
#endif
        }
      });
  AddGestureRecognizer(std::move(multi_tap_recognizer));
#endif
#if defined(OS_ANDROID) || defined(OS_IOS)
#ifndef ENABLE_CLAY_LITE
  auto long_press_recognizer = std::make_unique<LongPressGestureRecognizer>(
      page_view()->gesture_manager());
  long_press_recognizer_ = long_press_recognizer.get();
  long_press_recognizer->SetLongPressStartCallback(
      [this](const PointerEvent& event) {
        RequestFocus();
        GetRenderText()->SetAllSelection();
        auto range = GetRenderText()->GetSelection();
        selection_start_pos_ = range.start();
        selection_end_pos_ = range.end();
        ShowSelectionHandle();
        if (!custom_context_menu_) {
          ShowSelectionPopup();
        }
      });
  AddGestureRecognizer(std::move(long_press_recognizer));
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
  drag_recognizer->SetDragEndCallback([this](const Velocity&) {
    if (IsRuntimeContentEditable()) {
      CONTENT_EDITABLE_LOG << "drag-end view=" << id()
                           << " base=" << editable_selection_base_offset_
                           << " extent=" << editable_selection_extent_offset_
                           << " focused=" << IsFocused()
                           << " connected="
                           << (text_input_controller_ &&
                               text_input_controller_->ConnectKeyboard());
      if (IsFocused()) {
        BeginEditableInput();
      }
    }
  });
  drag_recognizer->SetDragCancelCallback(
      [this]() { PerformCancelSelection(); });
  AddGestureRecognizer(std::move(drag_recognizer));
#endif
}

FloatPoint TextView::ViewPointToParagraphPoint(FloatPoint point_by_self) {
  point_by_self.Move(-BorderLeft() - PaddingLeft(),
                     -BorderTop() - PaddingTop());
  return point_by_self;
}

void TextView::PerformBeginSelection(FloatPoint point) {
#ifndef ENABLE_CLAY_LITE
  point -= BoundsRelativeTo(nullptr).location();
  auto point_by_paragraph = ViewPointToParagraphPoint(point);
  auto editable_offset = GetEditableOffsetForPoint(point_by_paragraph);
  CONTENT_EDITABLE_LOG << "drag-begin view=" << id()
                       << " editable=" << editable_offset;
  SetEditableSelectionRange(editable_offset, editable_offset);
#endif
}

void TextView::PerformMoveSelection(FloatPoint point,
                                    SelectionHandleView* handle_bar) {
#ifndef ENABLE_CLAY_LITE
  point -= BoundsRelativeTo(nullptr).location();
  auto point_by_paragraph = ViewPointToParagraphPoint(point);
  auto editable_extent = GetEditableOffsetForPoint(point_by_paragraph);
  CONTENT_EDITABLE_LOG << "drag-move view=" << id()
                       << " base=" << editable_selection_base_offset_
                       << " extent=" << editable_extent;
  SetEditableSelectionRange(editable_selection_base_offset_, editable_extent);
  auto render_text = GetRenderText();
  auto text_box = render_text->GetEndTextPositionTopAndBottom();
  BringIntoView(&text_box);
#endif
}

void TextView::PerformCancelSelection() {
  if (IsRuntimeContentEditable()) {
    CONTENT_EDITABLE_LOG << "drag-cancel view=" << id()
                         << " base=" << editable_selection_base_offset_
                         << " extent=" << editable_selection_extent_offset_
                         << " focused=" << IsFocused()
                         << " connected="
                         << (text_input_controller_ &&
                             text_input_controller_->ConnectKeyboard());
    if (IsFocused()) {
      BeginEditableInput();
    }
  }
  HideSelectionPopup();
  HideSelectionHandle();
}

void TextView::MoveCaretTo(FloatPoint point) {
#ifndef ENABLE_CLAY_LITE
  RequestFocus();
  MoveEditableCaretToOffset(
      GetEditableOffsetForPoint(ViewPointToParagraphPoint(point)));
  HideSelectionPopup();
  HideSelectionHandle();
#endif
}

void TextView::SelectWordAt(FloatPoint point) {
#ifndef ENABLE_CLAY_LITE
  RequestFocus();
  auto point_by_paragraph = ViewPointToParagraphPoint(point);
  auto render_text = GetRenderText();
  auto glyph_pos = render_text->GetPainter()->GetGlyphPositionAtCoordinate(
      point_by_paragraph.x(), point_by_paragraph.y());
  auto word = render_text->GetPainter()->GetWordBoundary(glyph_pos.first);
  if (word.collapsed() && glyph_pos.first > 0) {
    word = render_text->GetPainter()->GetWordBoundary(glyph_pos.first - 1);
  }
  render_text->SetSelection(word);
  StopContentEditableCaretBlinking();
  selection_start_pos_ = word.start();
  selection_end_pos_ = word.end();
#endif
}

void TextView::SelectLineAt(FloatPoint point) {
#ifndef ENABLE_CLAY_LITE
  RequestFocus();
  auto point_by_paragraph = ViewPointToParagraphPoint(point);
  auto render_text = GetRenderText();
  auto glyph_pos = render_text->GetPainter()->GetGlyphPositionAtCoordinate(
      point_by_paragraph.x(), point_by_paragraph.y());
  auto line =
      render_text->GetPainter()->GetLineRangeForPosition(glyph_pos.first);
  render_text->SetSelection(line);
  StopContentEditableCaretBlinking();
  selection_start_pos_ = line.start();
  selection_end_pos_ = line.end();
#endif
}

void TextView::OnSelectionChanged(int selection_start, int selection_end) {
  int event_start = selection_start;
  int event_end = selection_end;
#ifndef ENABLE_CLAY_LITE
  selection_start_pos_ = selection_start;
  selection_end_pos_ = selection_end;
#endif
  if (IsRuntimeContentEditable()) {
    if (applying_editable_selection_to_render_) {
      event_start = static_cast<int>(editable_selection_base_offset_);
      event_end = static_cast<int>(editable_selection_extent_offset_);
      CONTENT_EDITABLE_LOG << "selection-change ignored view=" << id()
                           << " render_base=" << selection_start
                           << " render_extent=" << selection_end
                           << " editable_base="
                           << editable_selection_base_offset_
                           << " editable_extent="
                           << editable_selection_extent_offset_;
    } else {
      auto stream_length = GetEditableStreamLength(GetEditableTextShadowNode());
      auto editable_base = std::min(
          GetEditableOffsetForRenderOffset(
              static_cast<size_t>(std::max(selection_start, 0))),
          stream_length);
      auto editable_extent = std::min(
          GetEditableOffsetForRenderOffset(
              static_cast<size_t>(std::max(selection_end, 0))),
          stream_length);
      if (ignore_next_collapsed_render_selection_change_ &&
          selection_start == selection_end &&
          editable_selection_base_offset_ !=
              editable_selection_extent_offset_) {
        ignore_next_collapsed_render_selection_change_ = false;
#ifndef ENABLE_CLAY_LITE
        selection_start_pos_ = static_cast<int>(GetRenderOffsetForEditableOffset(
            editable_selection_base_offset_));
        selection_end_pos_ = static_cast<int>(GetRenderOffsetForEditableOffset(
            editable_selection_extent_offset_));
#endif
        CONTENT_EDITABLE_LOG
            << "selection-change collapsed-ignored view=" << id()
            << " render_base=" << selection_start
            << " render_extent=" << selection_end
            << " editable_base=" << editable_selection_base_offset_
            << " editable_extent=" << editable_selection_extent_offset_;
        return;
      }
      ignore_next_collapsed_render_selection_change_ = false;
      editable_selection_base_offset_ = editable_base;
      editable_selection_extent_offset_ = editable_extent;
      editable_caret_offset_ = editable_selection_extent_offset_;
      editable_caret_initialized_ = true;
      event_start = static_cast<int>(editable_selection_base_offset_);
      event_end = static_cast<int>(editable_selection_extent_offset_);
      CONTENT_EDITABLE_LOG << "selection-change view=" << id()
                           << " render_base=" << selection_start
                           << " render_extent=" << selection_end
                           << " editable_base="
                           << editable_selection_base_offset_
                           << " editable_extent="
                           << editable_selection_extent_offset_
                           << " stream_length=" << stream_length;
      SyncEditableInputStateToPlatform();
    }
  }
#ifndef ENABLE_CLAY_LITE
  UpdateContentEditableCaretBlinking();
#endif

  std::string direction;
  if (event_start > event_end) {
    direction = "backward";
  } else {
    direction = "forward";
  }
  page_view()->SendEvent(
      id(), event_attr::kEventEditSelectionChange,
      {"start", "end", "direction"}, std::min(event_start, event_end),
      std::max(event_start, event_end), direction.c_str());
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
  render_text->SetSelection(TextRange(start_index, end_index));
  ShowSelectionHandle(show_start_handle, show_end_handle);

  const auto& line_rects =
      GetRenderText()->GetTextLineRects(start_index, end_index);
  const auto& bounding_rect = page_view()->ConvertTo<kPixelTypeLogical>(
      GetRenderText()->GetTextBoundingRect(start_index, end_index, line_rects));
  clay::Value::Map result;
  result["boundingRect"] = clay::Value(CreateRectMap(bounding_rect));
  clay::Value::Array box_array(line_rects.size());
  for (size_t i = 0; i < line_rects.size(); i++) {
    box_array[i] = clay::Value(CreateRectMap(
        page_view()->ConvertTo<kPixelTypeLogical>(line_rects[i])));
  }
  clay::Value::Array handle_array(2);
  if (!line_rects.empty()) {
    auto start_handle_x =
        page_view()->ConvertTo<kPixelTypeLogical>(line_rects.front().left()) -
        -1;
    auto start_handle_y =
        page_view()->ConvertTo<kPixelTypeLogical>(line_rects.front().top()) - 6;
    auto end_handle_x =
        page_view()->ConvertTo<kPixelTypeLogical>(line_rects.front().right()) +
        1;
    auto end_handle_y =
        page_view()->ConvertTo<kPixelTypeLogical>(line_rects.front().bottom()) +
        6;
    handle_array[0] =
        clay::Value(CreateHandleMap(start_handle_x, start_handle_y, 6));
    handle_array[1] =
        clay::Value(CreateHandleMap(end_handle_x, end_handle_y, 6));
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
  callback(LynxUIMethodResult::kSuccess, clay::Value(std::move(text)));
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
  CONTENT_EDITABLE_LOG << "focus-change view=" << id()
                       << " focused=" << focused << " is_leaf=" << is_leaf
                       << " contenteditable=" << IsRuntimeContentEditable()
                       << " connected="
                       << (text_input_controller_ &&
                           text_input_controller_->ConnectKeyboard());
  if (IsRuntimeContentEditable() && !focused && !editable_undo_stack_.empty()) {
    CONTENT_EDITABLE_LOG << "clear-undo-on-blur view=" << id()
                         << " undo_count=" << editable_undo_stack_.size();
    editable_undo_stack_.clear();
  }
#ifndef ENABLE_CLAY_LITE
  GetRenderText()->SetSelection(
      TextRange(selection_end_pos_, selection_end_pos_));
  UpdateContentEditableCaretBlinking();
#endif
  if (IsRuntimeContentEditable()) {
    if (focused) {
      if (!editable_caret_initialized_) {
        editable_caret_offset_ =
            GetEditableStreamLength(GetEditableTextShadowNode());
        editable_selection_base_offset_ = editable_caret_offset_;
        editable_selection_extent_offset_ = editable_caret_offset_;
        editable_caret_initialized_ = true;
      }
      BeginEditableInput();
    } else {
      EndEditableInput();
    }
  }
  BaseView::FocusHasChanged(focused, is_leaf);
  HideSelectionPopup();
  HideSelectionHandle();
}

bool TextView::HandleEditableKeyEvent(const KeyEvent* key_event) {
  if (!key_event) {
    return false;
  }
  if (key_event->GetType() != KeyEventType::kDown &&
      key_event->GetType() != KeyEventType::kRepeat) {
    return false;
  }

  auto key_code = static_cast<KeyCode>(key_event->GetLogical());
  switch (key_code) {
    case KeyCode::kBackspace:
      OnDeleteSurroundingText(1, 0);
      return true;
    case KeyCode::kDelete:
      OnDeleteSurroundingText(0, 1);
      return true;
    case KeyCode::kArrowLeft: {
      auto* shadow_node = GetEditableTextShadowNode();
      if (!shadow_node) {
        return true;
      }
      auto stream_text = GetEditableStreamText(shadow_node);
      auto selection = GetEditableSelectionRange(stream_text.length());
      if (selection.first != selection.second) {
        MoveEditableCaretToOffset(selection.first);
      } else {
        MoveEditableCaretToOffset(
            PreviousCodePointOffset(stream_text, selection.second));
      }
      return true;
    }
    case KeyCode::kArrowRight: {
      auto* shadow_node = GetEditableTextShadowNode();
      if (!shadow_node) {
        return true;
      }
      auto stream_text = GetEditableStreamText(shadow_node);
      auto selection = GetEditableSelectionRange(stream_text.length());
      if (selection.first != selection.second) {
        MoveEditableCaretToOffset(selection.second);
      } else {
        MoveEditableCaretToOffset(
            NextCodePointOffset(stream_text, selection.second));
      }
      return true;
    }
    case KeyCode::kHome:
      MoveEditableCaretToOffset(0);
      return true;
    case KeyCode::kEnd:
      MoveEditableCaretToOffset(GetEditableStreamLength(GetEditableTextShadowNode()));
      return true;
    default:
      break;
  }

  if (key_event->GetCharacter().empty()) {
    return false;
  }
#if defined(OS_ANDROID) || defined(OS_IOS) || defined(OS_HARMONY)
  if (key_event->GetType() == KeyEventType::kDown) {
    return InsertEditableText(
        UnicodeUtil::Utf8ToUtf16(key_event->GetCharacter()));
  }
#else
  if (key_event->GetSynthesized()) {
    return InsertEditableText(
        UnicodeUtil::Utf8ToUtf16(key_event->GetCharacter()));
  }
#endif
  return false;
}

TextShadowNode* TextView::GetEditableTextShadowNode() {
  if (editable_text_shadow_node_for_testing_) {
    return editable_text_shadow_node_for_testing_;
  }
  auto* context = page_view()->GetViewContext();
  if (!context) {
    return nullptr;
  }
  auto* node = context->FindShadowNodeByNodeId(id());
  if (!node || !node->IsTextShadowNode()) {
    return nullptr;
  }
  return static_cast<TextShadowNode*>(node);
}

bool TextView::IsRuntimeContentEditable() {
  if (content_editable_) {
    return true;
  }
  auto* shadow_node = GetEditableTextShadowNode();
  return shadow_node && shadow_node->IsContentEditable();
}

void TextView::EnsureRuntimeContentEditableState() {
  if (!IsRuntimeContentEditable()) {
    return;
  }
  SetFocusable(true);
  if (!custom_text_selection_ && !multi_tap_recognizer_) {
    ResetGestureRecognizers();
  }
}

size_t TextView::GetEditableStreamLength(TextShadowNode* shadow_node) {
  if (!shadow_node) {
    return 0;
  }
  size_t length = 0;
  for (const auto& item : shadow_node->BuildEditableInlineStream()) {
    length = std::max(length, item.start + item.length);
  }
  return length;
}

std::u16string TextView::GetEditableStreamText(TextShadowNode* shadow_node) {
  if (!shadow_node) {
    return std::u16string();
  }
  return BuildEditableStreamText(shadow_node->BuildEditableInlineStream());
}

size_t TextView::GetEditableOffsetForPoint(
    const FloatPoint& point_by_paragraph) {
  size_t atomic_offset = 0;
  if (GetEditableAtomicOffsetForPoint(point_by_paragraph, &atomic_offset)) {
    return atomic_offset;
  }
  auto glyph_pos = GetRenderText()->GetPainter()->GetGlyphPositionAtCoordinate(
      point_by_paragraph.x(), point_by_paragraph.y());
  auto editable_offset =
      GetEditableOffsetForRenderOffset(static_cast<size_t>(glyph_pos.first));
  CONTENT_EDITABLE_LOG << "point-to-editable view=" << id() << " point=("
                       << point_by_paragraph.x() << ","
                       << point_by_paragraph.y()
                       << ") render=" << glyph_pos.first
                       << " editable=" << editable_offset;
  return editable_offset;
}

bool TextView::GetEditableAtomicOffsetForPoint(
    const FloatPoint& point_by_paragraph, size_t* editable_offset) {
  auto* shadow_node = GetEditableTextShadowNode();
  auto* painter = GetRenderText()->GetPainter();
  if (!shadow_node || !painter || !editable_offset) {
    return false;
  }

  auto stream = shadow_node->BuildEditableInlineStream();
  auto get_render_range = [](const auto& item, size_t* start, size_t* end) {
    if (item.type == EditableInlineStreamItemType::kText && item.node &&
        item.node->IsRawTextShadowNode()) {
      auto* raw_text = static_cast<RawTextShadowNode*>(item.node);
      *start = raw_text->StartGlyph();
      *end = raw_text->EndGlyph();
      return true;
    }
    if (item.type == EditableInlineStreamItemType::kAtomicInlineImage &&
        item.node && item.node->IsInlineImageShadowNode()) {
      auto* image = static_cast<InlineImageShadowNode*>(item.node);
      *start = image->StartGlyph();
      *end = image->EndGlyph();
      return true;
    }
    if (item.type == EditableInlineStreamItemType::kAtomicInlineView &&
        item.node && item.node->IsInlineViewShadowNode()) {
      auto* view = static_cast<InlineViewShadowNode*>(item.node);
      *start = view->StartGlyph();
      *end = view->EndGlyph();
      return true;
    }
    return false;
  };

  auto y_overlaps = [](const FloatRect& a, const FloatRect& b) {
    return a.y() <= b.MaxY() && a.MaxY() >= b.y();
  };

  for (size_t index = 0; index < stream.size(); ++index) {
    const auto& item = stream[index];
    if (item.type != EditableInlineStreamItemType::kAtomicInlineImage &&
        item.type != EditableInlineStreamItemType::kAtomicInlineView) {
      continue;
    }

    size_t render_start = 0;
    size_t render_end = 0;
    if (!get_render_range(item, &render_start, &render_end)) {
      continue;
    }

    auto boxes = painter->GetRectsForRange(
        static_cast<int>(render_start), static_cast<int>(render_end),
        RectHeightStyle::kTight, RectWidthStyle::kTight);
    for (const auto& box : boxes) {
      const auto& rect = box.rect;
      if (point_by_paragraph.y() < rect.y() ||
          point_by_paragraph.y() > rect.MaxY()) {
        continue;
      }

      if (rect.Contains(point_by_paragraph)) {
        *editable_offset =
            point_by_paragraph.x() < rect.Center().x() ? item.start
                                                       : item.start + 1;
        CONTENT_EDITABLE_LOG << "point-to-editable atomic-hit view=" << id()
                             << " point=(" << point_by_paragraph.x() << ","
                             << point_by_paragraph.y() << ") item_type="
                             << EditableItemTypeName(item.type)
                             << " item_render=[" << render_start << ","
                             << render_end << "] item_editable=["
                             << item.start << "," << item.start + item.length
                             << "] rect=(" << rect.x() << "," << rect.y()
                             << " " << rect.width() << "x" << rect.height()
                             << ") editable=" << *editable_offset;
        return true;
      }

      float previous_right = -std::numeric_limits<float>::infinity();
      for (size_t previous = index; previous > 0; --previous) {
        size_t previous_start = 0;
        size_t previous_end = 0;
        if (!get_render_range(stream[previous - 1], &previous_start,
                              &previous_end)) {
          continue;
        }
        auto previous_boxes = painter->GetRectsForRange(
            static_cast<int>(previous_start), static_cast<int>(previous_end),
            RectHeightStyle::kTight, RectWidthStyle::kTight);
        for (const auto& previous_box : previous_boxes) {
          if (y_overlaps(previous_box.rect, rect)) {
            previous_right =
                std::max(previous_right, previous_box.rect.MaxX());
          }
        }
        if (previous_right != -std::numeric_limits<float>::infinity()) {
          break;
        }
      }

      float next_left = std::numeric_limits<float>::infinity();
      for (size_t next = index + 1; next < stream.size(); ++next) {
        size_t next_start = 0;
        size_t next_end = 0;
        if (!get_render_range(stream[next], &next_start, &next_end)) {
          continue;
        }
        auto next_boxes = painter->GetRectsForRange(
            static_cast<int>(next_start), static_cast<int>(next_end),
            RectHeightStyle::kTight, RectWidthStyle::kTight);
        for (const auto& next_box : next_boxes) {
          if (y_overlaps(next_box.rect, rect)) {
            next_left = std::min(next_left, next_box.rect.x());
          }
        }
        if (next_left != std::numeric_limits<float>::infinity()) {
          break;
        }
      }

      if (point_by_paragraph.x() > previous_right &&
          point_by_paragraph.x() < rect.x()) {
        *editable_offset = item.start;
        CONTENT_EDITABLE_LOG << "point-to-editable atomic-gap view=" << id()
                             << " point=(" << point_by_paragraph.x() << ","
                             << point_by_paragraph.y() << ") side=before"
                             << " item_type=" << EditableItemTypeName(item.type)
                             << " gap=(" << previous_right << "," << rect.x()
                             << ") editable=" << *editable_offset;
        return true;
      }

      if (point_by_paragraph.x() > rect.MaxX() &&
          point_by_paragraph.x() < next_left) {
        *editable_offset = item.start + 1;
        CONTENT_EDITABLE_LOG << "point-to-editable atomic-gap view=" << id()
                             << " point=(" << point_by_paragraph.x() << ","
                             << point_by_paragraph.y() << ") side=after"
                             << " item_type=" << EditableItemTypeName(item.type)
                             << " gap=(" << rect.MaxX() << "," << next_left
                             << ") editable=" << *editable_offset;
        return true;
      }
    }
  }

  return false;
}

size_t TextView::GetEditableOffsetForRenderOffset(size_t render_offset) {
  auto* shadow_node = GetEditableTextShadowNode();
  if (!shadow_node) {
    return render_offset;
  }

  size_t previous_editable_offset = 0;
  for (const auto& item : shadow_node->BuildEditableInlineStream()) {
    size_t render_start = 0;
    size_t render_end = 0;
    if (item.type == EditableInlineStreamItemType::kText && item.node &&
        item.node->IsRawTextShadowNode()) {
      auto* raw_text = static_cast<RawTextShadowNode*>(item.node);
      render_start = raw_text->StartGlyph();
      render_end = raw_text->EndGlyph();
      if (render_offset < render_start) {
        CONTENT_EDITABLE_LOG << "render-to-editable gap view=" << id()
                             << " render=" << render_offset
                             << " before_item=" << EditableItemTypeName(item.type)
                             << " item_render_start=" << render_start
                             << " editable=" << previous_editable_offset;
        return previous_editable_offset;
      }
      if (render_offset <= render_end) {
        auto layout_offset = std::min(render_offset - render_start,
                                      raw_text->GetLayoutTextLength());
        auto raw_offset =
            raw_text->GetRawEndIndexForLayoutTextLength(layout_offset);
        auto editable_offset = item.start + std::min(raw_offset, item.length);
        CONTENT_EDITABLE_LOG << "render-to-editable text view=" << id()
                             << " render=" << render_offset
                             << " item_render=[" << render_start << ","
                             << render_end << "] item_editable=["
                             << item.start << "," << item.start + item.length
                             << "] layout_offset=" << layout_offset
                             << " raw_offset=" << raw_offset
                             << " editable=" << editable_offset;
        return editable_offset;
      }
      previous_editable_offset = item.start + item.length;
      continue;
    }

    if (item.type == EditableInlineStreamItemType::kAtomicInlineImage &&
        item.node && item.node->IsInlineImageShadowNode()) {
      auto* image = static_cast<InlineImageShadowNode*>(item.node);
      render_start = image->StartGlyph();
      render_end = image->EndGlyph();
    } else if (item.type == EditableInlineStreamItemType::kAtomicInlineView &&
               item.node && item.node->IsInlineViewShadowNode()) {
      auto* view = static_cast<InlineViewShadowNode*>(item.node);
      render_start = view->StartGlyph();
      render_end = view->EndGlyph();
    } else {
      continue;
    }

    if (render_offset < render_start) {
      CONTENT_EDITABLE_LOG << "render-to-editable gap view=" << id()
                           << " render=" << render_offset
                           << " before_item=" << EditableItemTypeName(item.type)
                           << " item_render_start=" << render_start
                           << " editable=" << previous_editable_offset;
      return previous_editable_offset;
    }
    if (render_offset <= render_end) {
      auto editable_offset =
          render_offset <= render_start ? item.start : item.start + 1;
      CONTENT_EDITABLE_LOG << "render-to-editable atomic view=" << id()
                           << " render=" << render_offset
                           << " item_type=" << EditableItemTypeName(item.type)
                           << " item_render=[" << render_start << ","
                           << render_end << "] item_editable=["
                           << item.start << "," << item.start + item.length
                           << "] editable=" << editable_offset;
      return editable_offset;
    }
    previous_editable_offset = item.start + item.length;
  }

  CONTENT_EDITABLE_LOG << "render-to-editable end view=" << id()
                       << " render=" << render_offset
                       << " editable=" << previous_editable_offset;
  return previous_editable_offset;
}

size_t TextView::GetRenderOffsetForEditableOffset(size_t editable_offset) {
  auto* shadow_node = GetEditableTextShadowNode();
  if (!shadow_node) {
    return editable_offset;
  }

  size_t previous_render_offset = 0;
  for (const auto& item : shadow_node->BuildEditableInlineStream()) {
    if (editable_offset < item.start) {
      return previous_render_offset;
    }

    if (item.type == EditableInlineStreamItemType::kText && item.node &&
        item.node->IsRawTextShadowNode()) {
      auto* raw_text = static_cast<RawTextShadowNode*>(item.node);
      if (editable_offset <= item.start + item.length) {
        auto raw_offset = std::min(editable_offset - item.start, item.length);
        auto render_offset =
            raw_text->StartGlyph() +
            raw_text->GetLayoutTextLengthForRawEndIndex(raw_offset);
        CONTENT_EDITABLE_LOG << "editable-to-render text view=" << id()
                             << " editable=" << editable_offset
                             << " item_editable=[" << item.start << ","
                             << item.start + item.length << "] item_render=["
                             << raw_text->StartGlyph() << ","
                             << raw_text->EndGlyph()
                             << "] raw_offset=" << raw_offset
                             << " render=" << render_offset;
        return render_offset;
      }
      previous_render_offset = raw_text->EndGlyph();
      continue;
    }

    if (item.type == EditableInlineStreamItemType::kAtomicInlineImage &&
        item.node && item.node->IsInlineImageShadowNode()) {
      auto* image = static_cast<InlineImageShadowNode*>(item.node);
      if (editable_offset <= item.start + item.length) {
        auto render_offset = editable_offset <= item.start ? image->StartGlyph()
                                                           : image->EndGlyph();
        CONTENT_EDITABLE_LOG << "editable-to-render atomic view=" << id()
                             << " editable=" << editable_offset
                             << " item_type=" << EditableItemTypeName(item.type)
                             << " item_editable=[" << item.start << ","
                             << item.start + item.length << "] item_render=["
                             << image->StartGlyph() << "," << image->EndGlyph()
                             << "] render=" << render_offset;
        return render_offset;
      }
      previous_render_offset = image->EndGlyph();
      continue;
    }

    if (item.type == EditableInlineStreamItemType::kAtomicInlineView &&
        item.node && item.node->IsInlineViewShadowNode()) {
      auto* view = static_cast<InlineViewShadowNode*>(item.node);
      if (editable_offset <= item.start + item.length) {
        auto render_offset = editable_offset <= item.start ? view->StartGlyph()
                                                          : view->EndGlyph();
        CONTENT_EDITABLE_LOG << "editable-to-render atomic view=" << id()
                             << " editable=" << editable_offset
                             << " item_type=" << EditableItemTypeName(item.type)
                             << " item_editable=[" << item.start << ","
                             << item.start + item.length << "] item_render=["
                             << view->StartGlyph() << "," << view->EndGlyph()
                             << "] render=" << render_offset;
        return render_offset;
      }
      previous_render_offset = view->EndGlyph();
    }
  }

  CONTENT_EDITABLE_LOG << "editable-to-render end view=" << id()
                       << " editable=" << editable_offset
                       << " render=" << previous_render_offset;
  return previous_render_offset;
}

size_t TextView::GetEditableOffsetForPlaceholderId(int placeholder_id) {
  int node_id = -1;
  for (const auto& image_index : inline_images_index_) {
    if (image_index.second == placeholder_id) {
      node_id = image_index.first;
      break;
    }
  }
  if (node_id < 0) {
    for (const auto& view_index : inline_views_index_) {
      if (view_index.second == placeholder_id) {
        node_id = view_index.first;
        break;
      }
    }
  }

  auto* shadow_node = GetEditableTextShadowNode();
  if (node_id >= 0 && shadow_node) {
    for (const auto& item : shadow_node->BuildEditableInlineStream()) {
      if (item.node && item.node->id() == node_id) {
        return item.start + item.length;
      }
    }
  }
  return std::numeric_limits<size_t>::max();
}

std::pair<size_t, size_t> TextView::GetEditableSelectionRange(
    size_t stream_length) {
  editable_selection_base_offset_ =
      std::min(editable_selection_base_offset_, stream_length);
  editable_selection_extent_offset_ =
      std::min(editable_selection_extent_offset_, stream_length);
  if (editable_selection_base_offset_ != editable_selection_extent_offset_) {
    return {std::min(editable_selection_base_offset_,
                     editable_selection_extent_offset_),
            std::max(editable_selection_base_offset_,
                     editable_selection_extent_offset_)};
  }
  if (!editable_caret_initialized_) {
    editable_caret_offset_ = stream_length;
    editable_caret_initialized_ = true;
  } else {
    editable_caret_offset_ = std::min(editable_caret_offset_, stream_length);
  }
  editable_selection_base_offset_ = editable_caret_offset_;
  editable_selection_extent_offset_ = editable_caret_offset_;
  return {editable_caret_offset_, editable_caret_offset_};
}

void TextView::SetEditableSelectionRange(size_t base, size_t extent) {
  auto stream_length = GetEditableStreamLength(GetEditableTextShadowNode());
  base = std::min(base, stream_length);
  extent = std::min(extent, stream_length);
  editable_selection_base_offset_ = base;
  editable_selection_extent_offset_ = extent;
  editable_caret_offset_ = extent;
  editable_caret_initialized_ = true;
#ifndef ENABLE_CLAY_LITE
  selection_start_pos_ =
      static_cast<int>(GetRenderOffsetForEditableOffset(base));
  selection_end_pos_ =
      static_cast<int>(GetRenderOffsetForEditableOffset(extent));
  ignore_next_collapsed_render_selection_change_ = base != extent;
  applying_editable_selection_to_render_ = true;
  GetRenderText()->SetSelection(
      TextRange(selection_start_pos_, selection_end_pos_));
  applying_editable_selection_to_render_ = false;
  UpdateContentEditableCaretBlinking();
#endif
  SyncEditableInputStateToPlatform();
}

bool TextView::ReplaceEditableRange(size_t start, size_t end,
                                    const std::u16string& replacement,
                                    size_t selection_offset) {
  auto* shadow_node = GetEditableTextShadowNode();
  if (!shadow_node) {
    return false;
  }

  CONTENT_EDITABLE_LOG << "replace-range view=" << id() << " start=" << start
                       << " end=" << end
                       << " replacement_len=" << replacement.length()
                       << " requested_selection=" << selection_offset;
  PushEditableUndoSnapshot(shadow_node);
  auto result =
      shadow_node->ApplyEditableInlineStreamMutation(start, end, replacement);
  if (result.status != EditableInlineMutationStatus::kApplied) {
    CONTENT_EDITABLE_LOG << "replace-range failed view=" << id()
                         << " status=" << static_cast<int>(result.status);
    if (!editable_undo_stack_.empty()) {
      editable_undo_stack_.pop_back();
    }
    return false;
  }

  RemoveEditableAtomicViews(result.atomic_deletes);
  if (result.has_pending_text_insertion) {
    shadow_node->CreateRawTextNodeIfNeed(result.pending_text_insertion);
    result.selection_offset = result.pending_text_insertion_offset +
                              result.pending_text_insertion.length();
  }

  auto changed = result.text_changed || result.has_pending_text_insertion ||
                 !result.atomic_deletes.empty();
  CONTENT_EDITABLE_LOG << "replace-result view=" << id()
                       << " text_changed=" << result.text_changed
                       << " pending=" << result.has_pending_text_insertion
                       << " atomic_deletes=" << result.atomic_deletes.size()
                       << " result_selection=" << result.selection_offset;
  if (!changed && replacement.empty() && start == end) {
    if (!editable_undo_stack_.empty()) {
      editable_undo_stack_.pop_back();
    }
    return false;
  }

  editable_caret_offset_ = std::min(selection_offset, result.selection_offset);
  editable_caret_initialized_ = true;
  RefreshEditableTextView(shadow_node);
  MoveEditableCaretToOffset(editable_caret_offset_);
  if (IsRuntimeContentEditable()) {
    RequestFocus();
    CONTENT_EDITABLE_LOG << "post-mutation-input view=" << id()
                         << " changed=" << changed
                         << " focused=" << IsFocused()
                         << " connected="
                         << (text_input_controller_ &&
                             text_input_controller_->ConnectKeyboard());
    if (IsFocused() &&
        (!text_input_controller_ ||
         !text_input_controller_->ConnectKeyboard())) {
      BeginEditableInput();
    }
  }
  return true;
}

bool TextView::UndoEditableMutation() {
  if (editable_undo_stack_.empty()) {
    return false;
  }
  auto snapshot = std::move(editable_undo_stack_.back());
  editable_undo_stack_.pop_back();
  auto* shadow_node = GetEditableTextShadowNode();
  if (!shadow_node) {
    return false;
  }
  for (const auto& atomic : snapshot.atomic_children) {
    if (!atomic.node || !atomic.parent) {
      continue;
    }
    if (atomic.node->Parent() != atomic.parent) {
      atomic.parent->AddChild(
          atomic.node,
          ClampedInsertIndex(atomic.child_index, atomic.parent->ChildCount()));
      CONTENT_EDITABLE_LOG << "restore-atomic-shadow view=" << id()
                           << " node_id=" << atomic.node->id()
                           << " type=" << EditableItemTypeName(atomic.type)
                           << " child_index=" << atomic.child_index;
    }
    auto* view = page_view() ? page_view()->FindViewByViewId(atomic.node->id())
                             : nullptr;
    if (view && atomic.view_parent && view->Parent() != atomic.view_parent) {
      atomic.view_parent->AddChild(
          view,
          ClampedInsertIndex(atomic.view_child_index,
                             atomic.view_parent->child_count()));
      CONTENT_EDITABLE_LOG << "restore-atomic-view view=" << id()
                           << " node_id=" << atomic.node->id()
                           << " type=" << EditableItemTypeName(atomic.type)
                           << " view_child_index=" << atomic.view_child_index;
    }
  }
  for (auto& raw_text : snapshot.raw_texts) {
    if (raw_text.node) {
      raw_text.node->SetText(raw_text.text);
    }
  }
  RefreshEditableTextView(shadow_node);
  SetEditableSelectionRange(snapshot.selection_base,
                            snapshot.selection_extent);
  return true;
}

void TextView::PushEditableUndoSnapshot(TextShadowNode* shadow_node) {
  if (!shadow_node) {
    return;
  }
  EditableUndoSnapshot snapshot;
  snapshot.selection_base = editable_selection_base_offset_;
  snapshot.selection_extent = editable_selection_extent_offset_;
  for (const auto& item : shadow_node->BuildEditableInlineStream()) {
    if (item.type == EditableInlineStreamItemType::kText && item.node &&
        item.node->IsRawTextShadowNode()) {
      auto* raw_text = static_cast<RawTextShadowNode*>(item.node);
      snapshot.raw_texts.push_back({raw_text, raw_text->EditableText()});
      continue;
    }
    if (!IsAtomicEditableItem(item.type) || !item.node) {
      continue;
    }
    auto* parent = item.node->Parent();
    auto* text_parent =
        parent && (parent->IsTextShadowNode() || parent->IsInlineTextShadowNode())
            ? static_cast<BaseTextShadowNode*>(parent)
            : nullptr;
    auto* view = page_view() ? page_view()->FindViewByViewId(item.node->id())
                             : nullptr;
    auto* view_parent = view ? view->Parent() : nullptr;
    snapshot.atomic_children.push_back(
        {item.type, item.node, text_parent, ShadowChildIndex(text_parent, item.node),
         view_parent, ViewChildIndex(view_parent, view), -1});
  }
  editable_undo_stack_.push_back(std::move(snapshot));
  constexpr size_t kMaxEditableUndoStackSize = 100;
  if (editable_undo_stack_.size() > kMaxEditableUndoStackSize) {
    editable_undo_stack_.erase(editable_undo_stack_.begin());
  }
}

void TextView::RemoveEditableAtomicViews(
    const std::vector<EditableInlineAtomicDelete>& deletes) {
  for (const auto& item : deletes) {
    if (!item.node) {
      continue;
    }
    CONTENT_EDITABLE_LOG << "remove-atomic view=" << id()
                         << " node_id=" << item.node->id()
                         << " type=" << EditableItemTypeName(item.type)
                         << " editable_start=" << item.start
                         << " has_parent=" << (item.node->Parent() != nullptr);
    auto* view = page_view()->FindViewByViewId(item.node->id());
    if (view && view->Parent()) {
      view->Parent()->RemoveChild(view);
    }
    if (item.type == EditableInlineStreamItemType::kAtomicInlineImage) {
      inline_images_index_.erase(item.node->id());
    } else if (item.type == EditableInlineStreamItemType::kAtomicInlineView) {
      inline_views_index_.erase(item.node->id());
    }
    if (auto* parent = item.node->Parent()) {
      parent->RemoveChild(item.node);
    }
  }
}

void TextView::BeginEditableInput() {
  if (!text_input_controller_) {
    return;
  }
  CONTENT_EDITABLE_LOG << "begin-input view=" << id()
                       << " focused=" << IsFocused()
                       << " connected_before="
                       << text_input_controller_->ConnectKeyboard();
  text_input_controller_->SetMultiline(true);
  text_input_controller_->SetClient(id(), KeyboardAction::kMultiLine,
                                    KeyboardInputType::kClassText);
  text_input_controller_->SetEditableTransform(LocalToGlobalTransform());
  SyncEditableInputStateToPlatform();
  text_input_controller_->Show();
  page_view()->RequestInput(this, KeyboardInputType::kClassText,
                            KeyboardAction::kMultiLine);
}

void TextView::EndEditableInput() {
  CONTENT_EDITABLE_LOG << "end-input view=" << id()
                       << " focused=" << IsFocused()
                       << " connected_before="
                       << (text_input_controller_ &&
                           text_input_controller_->ConnectKeyboard());
  page_view()->StopInput(this);
  if (text_input_controller_ && text_input_controller_->ConnectKeyboard()) {
    text_input_controller_->Hide();
    text_input_controller_->ClearClient();
  }
}

void TextView::SyncEditableInputStateToPlatform() {
  if (syncing_editable_input_state_ || !text_input_controller_ ||
      !text_input_controller_->ConnectKeyboard()) {
    return;
  }
  auto* shadow_node = GetEditableTextShadowNode();
  if (!shadow_node) {
    return;
  }
  auto stream_text = GetEditableStreamText(shadow_node);
  auto stream_length = stream_text.length();
  if (!editable_caret_initialized_) {
    editable_caret_offset_ = stream_length;
    editable_selection_base_offset_ = editable_caret_offset_;
    editable_selection_extent_offset_ = editable_caret_offset_;
    editable_caret_initialized_ = true;
  } else {
    editable_caret_offset_ = std::min(editable_caret_offset_, stream_length);
    editable_selection_base_offset_ =
        std::min(editable_selection_base_offset_, stream_length);
    editable_selection_extent_offset_ =
        std::min(editable_selection_extent_offset_, stream_length);
    if (editable_selection_base_offset_ == editable_selection_extent_offset_) {
      editable_caret_offset_ = editable_selection_extent_offset_;
    }
  }
  CONTENT_EDITABLE_LOG << "sync-platform view=" << id()
                       << " text_len=" << stream_text.length()
                       << " selection_base="
                       << editable_selection_base_offset_
                       << " selection_extent="
                       << editable_selection_extent_offset_
                       << " caret=" << editable_caret_offset_;
  TextEditingValue value;
  value.SetText(stream_text,
                TextRange(static_cast<int>(editable_selection_base_offset_),
                          static_cast<int>(editable_selection_extent_offset_)),
                TextRange(0));
  text_input_controller_->SetEditableTransform(LocalToGlobalTransform());
  text_input_controller_->SetEditingState(value);
  UpdateEditableCaretRectToPlatform();
}

void TextView::UpdateEditableCaretRectToPlatform() {
  if (!text_input_controller_ || !text_input_controller_->ConnectKeyboard()) {
    return;
  }
#ifndef ENABLE_CLAY_LITE
  auto text_box = GetRenderText()->GetEndTextPositionTopAndBottom();
  auto rect = text_box.rect;
  if (rect.IsEmpty()) {
    rect = FloatRect(0, 0, 1, 16);
  }
  text_input_controller_->SetCaretRect(rect);
  text_input_controller_->SetComposingRect(rect);
#endif
}

void TextView::MoveEditableCaretToOffset(size_t offset) {
  auto stream_length = GetEditableStreamLength(GetEditableTextShadowNode());
  editable_caret_offset_ = std::min(offset, stream_length);
  editable_selection_base_offset_ = editable_caret_offset_;
  editable_selection_extent_offset_ = editable_caret_offset_;
  editable_caret_initialized_ = true;
#ifndef ENABLE_CLAY_LITE
  selection_start_pos_ = static_cast<int>(
      GetRenderOffsetForEditableOffset(editable_caret_offset_));
  selection_end_pos_ = selection_start_pos_;
  ignore_next_collapsed_render_selection_change_ = false;
  applying_editable_selection_to_render_ = true;
  GetRenderText()->SetSelection(
      TextRange(selection_start_pos_, selection_end_pos_));
  applying_editable_selection_to_render_ = false;
  UpdateContentEditableCaretBlinking();
#endif
  SyncEditableInputStateToPlatform();
}

void TextView::RefreshEditableTextView(TextShadowNode* shadow_node) {
  if (!shadow_node) {
    return;
  }

  MeasureConstraint constraint{
      ContentWidth() > 0 ? ContentWidth() : 0.f,
      ContentWidth() > 0 ? MeasureMode::kDefinite : MeasureMode::kIndefinite,
      ContentHeight() > 0 ? ContentHeight() : 0.f,
      ContentHeight() > 0 ? MeasureMode::kDefinite
                          : MeasureMode::kIndefinite};
  auto result = shadow_node->Measure(constraint);
  std::array<float, 4> paddings = {PaddingTop(), PaddingRight(),
                                   PaddingBottom(), PaddingLeft()};
  std::array<float, 4> borders = {BorderTop(), BorderRight(), BorderBottom(),
                                  BorderLeft()};
  shadow_node->OnLayout(result.width, constraint.width_mode, result.height,
                        constraint.height_mode, paddings, borders);

  std::unique_ptr<Bundle> bundle(shadow_node->MoveBundle());
  if (bundle) {
    bundle->UpdateExtraData(this);
  }
  MarkNeedsLayout();
  Invalidate();
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
  return view ?: this;
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
    if (IsRuntimeContentEditable()) {
      CONTENT_EDITABLE_LOG << "hit-placeholder view=" << id()
                           << " placeholder_id=" << index
                           << " point=(" << point_by_paragraph.x() << ","
                           << point_by_paragraph.y() << ")";
      return this;
    }
    for (auto image_index : inline_images_index_) {
      if (image_index.second == index) {
        return page_view_->FindViewByViewId(image_index.first);
      }
    }
    for (auto view_index : inline_views_index_) {
      if (view_index.second == index) {
        auto view = page_view_->FindViewByViewId(view_index.first);
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
    if (IsRuntimeContentEditable()) {
      return this;
    }
    return nullptr;
  }

  if (IsRuntimeContentEditable()) {
    return this;
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
  if (IsRuntimeContentEditable()) {
    return this;
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

void TextView::ShowSelectionHandle(bool show_start_handle,
                                   bool show_end_handle) {
#ifndef ENABLE_CLAY_LITE
  if (GetRenderText()->IsCollapsed()) {
    return;
  }
  HideSelectionHandle();
  auto stroke_width = page_view()->ConvertFrom<kPixelTypeLogical>(2);
  if (!start_selection_handle_ && show_start_handle) {
    start_selection_handle_ =
        new SelectionHandleView(page_view(), TextSelectionHandleType::kLeft);
    start_selection_handle_->SetHandleMove(
        [this](const FloatPoint& position, SelectionHandleView* view) {
          UpdateSelectionHandle(position, view);
          if (!custom_context_menu_) {
            ShowSelectionPopup();
          }
        });
    auto text_box = GetRenderText()->GetStartTextPositionTopAndBottom();
    auto offset =
        FloatPoint(text_box.GetLeft() + scroll_offset_.x() + stroke_width,
                   text_box.GetTop() + scroll_offset_.y() + stroke_width);
    start_selection_handle_->BuildSelectionHandle(text_box.rect.height(),
                                                  offset);
  }
  if (!end_selection_handle_ && show_end_handle) {
    end_selection_handle_ =
        new SelectionHandleView(page_view(), TextSelectionHandleType::kRight);
    end_selection_handle_->SetHandleMove(
        [this](const FloatPoint& position, SelectionHandleView* view) {
          UpdateSelectionHandle(position, view);
          if (!custom_context_menu_) {
            ShowSelectionPopup();
          }
        });
    auto text_box = GetRenderText()->GetEndTextPositionTopAndBottom();
    auto offset =
        FloatPoint(text_box.GetRight() + scroll_offset_.x() + stroke_width,
                   text_box.GetTop() + scroll_offset_.y() + stroke_width);
    end_selection_handle_->BuildSelectionHandle(text_box.rect.height(), offset);
  }
  if (!selection_handle_container_) {
    selection_handle_container_ =
        new OverlayView(-1, "handle_container", page_view());
    selection_handle_container_->SetOverflow(CSSProperty::OVERFLOW_HIDDEN);
    page_view()->AddChild(selection_handle_container_);
    auto display_rect = GetDisplayRect();
    selection_handle_container_->SetBound(
        display_rect.left() - stroke_width, display_rect.top() - stroke_width,
        display_rect.width() + 2 * stroke_width,
        display_rect.height() + 2 * stroke_width);
    selection_handle_container_->AddChild(start_selection_handle_);
    selection_handle_container_->AddChild(end_selection_handle_);
  }
#endif
}

void TextView::UpdateSelectionHandle(FloatPoint point,
                                     SelectionHandleView* handle_bar) {
#ifndef ENABLE_CLAY_LITE
  if (!handle_bar) {
    ShowSelectionHandle();
  }
  auto stroke_width = page_view()->ConvertFrom<kPixelTypeLogical>(2);
  auto render_text = GetRenderText();
  auto handle_point = BoundsRelativeTo(this).location();
  float delta =
      handle_bar->ProcessHandlePos(handle_point, render_text->GetLeftTextBox(),
                                   render_text->GetRightTextBox());
  point -= BoundsRelativeTo(nullptr).location();
  point.SetY(point.y() - delta);
  auto end_pos = render_text->GetPainter()->GetGlyphPositionAtCoordinate(
      point.x(), point.y());
  auto selection_range = render_text->GetSelectionRange();
  if (handle_bar->GetHandleType() == TextSelectionHandleType::kLeft) {
    render_text->SetSelection(TextRange(selection_range[1], end_pos.first));
    selection_start_pos_ = selection_range[1];
  } else {
    render_text->SetSelection(TextRange(selection_range[0], end_pos.first));
    selection_start_pos_ = selection_range[0];
  }
  selection_end_pos_ = end_pos.first;
  TextBox end_box = GetRenderText()->GetEndTextPositionTopAndBottom();
  BringIntoView(&end_box);

  FloatPoint left_offset;
  FloatPoint right_offset;
  if (selection_end_pos_ > selection_start_pos_) {
    auto left_box = render_text->GetLeftTextBox();
    auto right_box = render_text->GetRightTextBox();
    left_offset =
        FloatPoint(left_box.GetLeft() + scroll_offset_.x() + stroke_width,
                   left_box.GetTop() + scroll_offset_.y() + stroke_width);
    right_offset =
        FloatPoint(right_box.GetRight() + scroll_offset_.x() + stroke_width,
                   right_box.GetTop() + scroll_offset_.y() + stroke_width);
    handle_bar->SetHandleType(TextSelectionHandleType::kRight);
    handle_bar->BuildSelectionHandle(right_box.rect.height(), right_offset);
    if (handle_bar == start_selection_handle_ && end_selection_handle_) {
      end_selection_handle_->SetHandleType(TextSelectionHandleType::kLeft);
      end_selection_handle_->BuildSelectionHandle(left_box.rect.height(),
                                                  left_offset);
    } else if (handle_bar == end_selection_handle_ && start_selection_handle_) {
      start_selection_handle_->SetHandleType(TextSelectionHandleType::kLeft);
      start_selection_handle_->BuildSelectionHandle(left_box.rect.height(),
                                                    left_offset);
    }

  } else if (selection_end_pos_ < selection_start_pos_) {
    auto left_box = render_text->GetLeftTextBox();
    auto right_box = render_text->GetRightTextBox();
    left_offset =
        FloatPoint(left_box.GetLeft() + scroll_offset_.x() + stroke_width,
                   left_box.GetTop() + scroll_offset_.y() + stroke_width);
    right_offset =
        FloatPoint(right_box.GetRight() + scroll_offset_.x() + stroke_width,
                   right_box.GetTop() + scroll_offset_.y() + stroke_width);
    handle_bar->SetHandleType(TextSelectionHandleType::kLeft);
    handle_bar->BuildSelectionHandle(left_box.rect.height(), left_offset);
    if (handle_bar == start_selection_handle_ && end_selection_handle_) {
      end_selection_handle_->SetHandleType(TextSelectionHandleType::kRight);
      end_selection_handle_->BuildSelectionHandle(right_box.rect.height(),
                                                  right_offset);
    } else if (handle_bar == end_selection_handle_ && start_selection_handle_) {
      start_selection_handle_->SetHandleType(TextSelectionHandleType::kRight);
      start_selection_handle_->BuildSelectionHandle(right_box.rect.height(),
                                                    right_offset);
    }
  } else {
    HideSelectionHandle();
    HideSelectionPopup();
  }
#endif
}

void TextView::HideSelectionHandle() {
#ifndef ENABLE_CLAY_LITE
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
  GetRenderText()->SetSelection(
      TextRange(selection_end_pos_, selection_end_pos_));
  UpdateContentEditableCaretBlinking();
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
