// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_EDITABLE_EDITABLE_VIEW_H_
#define CLAY_UI_COMPONENT_EDITABLE_EDITABLE_VIEW_H_

#include <limits>
#include <memory>
#include <optional>
#include <regex>
#include <string>
#include <vector>

#include "base/include/fml/time/time_delta.h"
#include "base/include/fml/time/timer.h"
#include "clay/fml/logging.h"
#include "clay/ui/common/text_input_type_traits.h"
#include "clay/ui/common/text_selection.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/editable/ime_listener.h"
#include "clay/ui/component/editable/text_editing_controller.h"
#include "clay/ui/component/editable/text_input_controller.h"
#include "clay/ui/component/editable/text_span.h"
#include "clay/ui/component/measurable.h"
#include "clay/ui/component/text/layout_context.h"
#include "clay/ui/component/text/text_style.h"
#include "clay/ui/gesture/drag_gesture_recognizer.h"
#include "clay/ui/gesture/multi_tap_gesture_recognizer.h"
#include "clay/ui/lynx_module/lynx_ui_method_registrar.h"
#include "clay/ui/platform/keyboard_types.h"
#include "clay/ui/rendering/editable/render_editable.h"

namespace clay {

using PostPaintCallback = std::function<void()>;
using PostPaintCallbackIdType = int64_t;

class RawTextView;
class RenderEditable;
class TextEditingHistoryState;

struct LayoutContextMeasure : public LayoutContext {
  // Width for text paragraph layout.
  float layout_width = 0.f;
};

class EditableView : public WithTypeInfo<EditableView, BaseView>,
                     public Measurable,
                     public IMEListener,
                     public TextInputClient,
                     public TextInputControllerDelegate,
                     public TextEditingController::Observer,
                     public GestureRecognizer::Delegate {
 public:
  EditableView(int id, std::string tag, PageView* page_view,
               bool is_multiline = false, bool layout_root_candidate = true);
  EditableView(int id, int callback_id, std::string tag, PageView* page_view,
               bool is_multiline = false, bool layout_root_candidate = true);
  ~EditableView();

  friend class TextEditingHistoryState;

  int GetCallbackId() override { return callback_id_; }

  bool IsLayoutRootCandidate() const override { return layout_root_candidate_; }

  void OnLayout(LayoutContext* context) override;
  void Measure(const MeasureConstraint& constraint,
               MeasureResult& result) override;

#ifdef ENABLE_ACCESSIBILITY
  bool EnableAccessibilityElement() const override { return true; }
#endif

  void SetAttribute(const char* attr_c, const clay::Value& value) override;

  // TODO(yulitao): Style should be passed in or using theme manager.
  // Currently use arco design's default styles.
  void InitDefaultStyle();

  // Override observers
  void OnValueChanged(const TextEditingValue& value,
                      const TextEditingController*) override;
  void OnSelectionChanged(const TextEditingValue& value,
                          const TextEditingController*) override;
  void OnContentChanged(const TextEditingValue& value,
                        const TextEditingController*) override{};
  void OnUserInput(const TextEditingValue& value,
                   const TextEditingController*) override;

  void OnContentSizeChanged(const FloatRect& old_rect,
                            const FloatRect& new_rect) override;

  // Called each time tapped, including double-tap's first & second tap.
  void OnGestureTap(const PointerEvent& pointer);
  void OnGestureDoubleTap(const PointerEvent& pointer);
  void OnGestureTripleTap(const PointerEvent& pointer);
  void OnDragStart(const FloatPoint& position);
  void OnDragUpdate(const FloatPoint& position, const FloatSize& delta);

  // Add new_content at current selection and replace the old one if selected.
  // If |style| is empty, follow previous style, else create new style run.
  void EditContent(std::string new_content, std::optional<TextStyle> style) {}

  void SetDirection(int type) override;
  void SetMaxLines(uint32_t max_lines);
  void SetShowSoftInputOnFocus(bool show_soft_input_on_focus);
  void SetContent(const std::string& content);
  void SetFontColor(const Color& color);
  void SetFontSize(float font_size);
  void SetFontWeight(FontWeight font_weight);
  void SetLetterSpacing(float letter_spacing);
  void SetTextAlign(TextAlignment text_alignment);
  void SetReadOnly(bool read_only);
  void SetTextDirection(TextDirection text_direction);
  void SetFontFamily(const std::string& font_family);
  void RelayoutWhenSetFontFamily(const std::string& font_family);

  float LayoutWidth();

  LayoutContextMeasure CreateLayoutContext(const MeasureConstraint& constraint);

  // Lynx module UI method
#define UI_METHOD_LIST_DECLARATION(V) \
  V(setValue)                         \
  V(getValue)                         \
  V(focus)                            \
  V(blur)                             \
  V(setSelectionRange)
  UI_METHOD_LIST_DECLARATION(UI_METHOD_DEF);
#undef UI_METHOD_LIST_DECLARATION
  // Added by clay end

  void UpdateEditingState(std::string text, TextSelection selection,
                          TextRange composing, Affinity affinity) override;
  void PerformAction() override;

  Transform ToGlobalTransform() const;

  void UpdateRemoteStateIfNeeded(
      const TextEditingValue& text_editing_value) override;

  bool MatchAttrSettings(KeywordID attr);

  txt::Paragraph* GetParagraph();
  FloatRect ComputeCaretRect();

  float EstimateHeightWithMaxLines();

  TextRange FindNextOrPrevCharacterSelection(bool is_forward);

  RenderEditable* GetRenderEditable();
  float GetDefaultFontSize() const;
  const TextEditingValue& GetTextEditingValue();

  void SetKeyboardType(KeyboardInputType type) { keyboard_input_type_ = type; }
  void SetKeyboardAction(KeyboardAction action) { keyboard_action_ = action; }

  bool IsPointerAllowed(const GestureRecognizer& gesture_recognizer,
                        const PointerEvent& event) override;

 protected:
  size_t FilterInputTextByType(TextEditingValue* value);
  size_t FilterInputTextByUser(TextEditingValue* value);
  size_t FilterInput(TextEditingValue* value, const std::regex& filter_regex);
  size_t FilterNewLine(TextEditingValue* value);
  size_t FilterMaxLength(TextEditingValue* value);
  // TODO(yulitao): Design formatters to deal with input types.
  using FilterFunc = size_t (EditableView::*)(TextEditingValue*);
  void ApplyFilter(FilterFunc func, bool take_effect);
  void FilterInputIfNeeded(TextEditingValue* value);
  bool OnKeyEventInternal(const KeyEvent* event);

  void DeleteSurroundingText(int before_length, int after_length);

  void SetTextEditingValue(const TextEditingValue&, bool is_composing = false,
                           bool need_update_remote = true);
  void OnCommitText(std::string text) override;

  virtual void BeginEditingIfNeeded();
  void BeginEditing();
  void QuitEditing();

  void LayoutText(LayoutContext* context);
  std::shared_ptr<TextSpan> BuildTextSpan(TextStyle style);

  std::regex user_input_filter_;
  bool editing_ = false;
  std::unique_ptr<txt::Paragraph> paragraph_;
  TextStyle text_style_;
  std::u16string pre_text_value_;

 private:
  void OnComposingText(std::string text) override;
  void OnDeleteSurroundingText(int before_length, int after_length) override;
  void OnKeyboardEvent(std::unique_ptr<KeyEvent> key_event) override;
  bool OnKeyEvent(const KeyEvent* event) override;
  void OnPerformAction(KeyboardAction action) override;
  void OnFinishInput() override;

  void UpdateCaretRectIfNeeded();
  void UpdateComposingRectIfNeeded();

  void RequestKeyboard();

  void FocusHasChanged(bool focused, bool is_leaf) override;

  void DoBackspace();
  void DoDelete();
  void MoveCaret(KeyCode keycode);

  void ToggleCaret();
  void TwinkleCaretPeriodically();

  bool ApplyHotKey(const KeyEvent* key_event);
  void UpdateHotKeyTag(LogicalKeyboardKey key_code, bool is_up);
  void HandleCommandHotKey(LogicalKeyboardKey key_code);
  void HandleCtrlHotKey(LogicalKeyboardKey key_code);
  bool HandleShiftHotKey(LogicalKeyboardKey key_code);
  void HandleWinCtrlAndMacCommandHotKey(LogicalKeyboardKey key_code);
  void ResetGestureRecognizers();
  bool IsMultiline();

  void PostPaint() override;

  // Used to filter redundant handling for tap/double-tap.
  int last_tap_pointer_ = 0;

  bool readonly_ = false;
  // If false, soft keyboard will always be hidden.
  bool show_soft_input_on_focus_ = true;
  KeyboardInputType keyboard_input_type_ = KeyboardInputType::kClassText;
  KeyboardAction keyboard_action_ = KeyboardAction::kDone;
  uint32_t max_lines_ = std::numeric_limits<uint32_t>::max();
  uint32_t max_length_ = std::numeric_limits<uint32_t>::max();

  std::vector<FilterFunc> filter_funcs_;

  // Indicate whether caret is shown or hidden while twinkling.
  // False if hidden.
  bool twinkle_flag_ = false;
  std::unique_ptr<fml::RepeatingTimer> caret_timer_;

  // When has no contents, use this paragraph to measure a default line height.
  std::unique_ptr<txt::Paragraph> template_paragraph_;

  // TODO(wangchen): replace by EditingTextValue
  std::unique_ptr<TextEditingController> text_editing_controller_;
  uint32_t hot_key_tag_ = 0;

  std::unique_ptr<TextInputController> text_input_controller_;
  std::unique_ptr<TextEditingHistoryState> text_editing_history_state_;
  MultiTapGestureRecognizer* multi_tap_recognizer_ = nullptr;
  DragGestureRecognizer* drag_recognizer_ = nullptr;
  int32_t callback_id_ = -1;
  bool is_multiline_;
  bool layout_root_candidate_;
};

class UndoStack {
 public:
  UndoStack() = default;
  std::optional<TextEditingValue> CurrentValue() {
    if (list_.empty()) {
      return std::nullopt;
    }
    return list_[*index_];
  }
  void Push(const TextEditingValue& value) {
    if (list_.empty()) {
      index_ = 0;
      list_.push_back(value);
      return;
    }
    FML_DCHECK(index_ < list_.size() && index_ >= 0);
    const auto& current_value = CurrentValue();
    if (current_value.has_value() && current_value == value) {
      return;
    }
    if (index_.has_value() && index_ != list_.size() - 1) {
      list_.erase(list_.begin() + *index_ + 1, list_.end());
    }
    list_.push_back(value);
    index_ = list_.size() - 1;
  }
  std::optional<TextEditingValue> Undo() {
    if (list_.empty()) {
      return std::nullopt;
    }
    FML_DCHECK(index_ < list_.size() && index_ >= 0);
    if (index_ != 0) {
      index_ = *index_ - 1;
    }
    return CurrentValue();
  }
  std::optional<TextEditingValue> Redo() {
    if (list_.empty()) {
      return std::nullopt;
    }
    FML_DCHECK(index_ < list_.size() && index_ >= 0);
    if (index_ < list_.size() - 1) {
      index_ = *index_ + 1;
    }
    return CurrentValue();
  }

  void Clear() {
    list_.clear();
    index_ = -1;
  }

 private:
  std::vector<TextEditingValue> list_;
  std::optional<int> index_;
};

class TextEditingHistoryState {
 public:
  static constexpr fml::TimeDelta kThrottleDuration =
      fml::TimeDelta::FromMilliseconds(500);
  explicit TextEditingHistoryState(EditableView* editable_view);
  void Undo();
  void Redo();
  // Note(wangchen): If there is a lot of text it may cause memory bloat
  void Push();
  std::unique_ptr<fml::Timer> ThrottledPush();

 private:
  void Update(std::optional<TextEditingValue> new_value);
  UndoStack stack_;
  std::unique_ptr<fml::Timer> throttle_timer_;
  fml::TimeDelta duration_ = kThrottleDuration;
  EditableView* editable_view_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_EDITABLE_EDITABLE_VIEW_H_
