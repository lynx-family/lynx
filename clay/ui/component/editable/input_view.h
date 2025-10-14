// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_EDITABLE_INPUT_VIEW_H_
#define CLAY_UI_COMPONENT_EDITABLE_INPUT_VIEW_H_

#include <map>
#include <string>

#include "clay/ui/component/base_view.h"
#include "clay/ui/component/editable/editable_view.h"

namespace clay {

class TextEditingHistoryState;

class InputView : public WithTypeInfo<InputView, EditableView> {
 public:
  InputView(int id, PageView* page_view, bool is_multiline = false,
            bool layout_root_candidate = true);
  InputView(int id, int callback_id, PageView* page_view,
            bool is_multiline = false, bool layout_root_candidate = true);
  ~InputView() = default;

  void OnLayout(LayoutContext* context) override;

  void SetAttribute(const char* attr_c, const clay::Value& value) override;

  bool OnKeyEventInternal(const KeyEvent* key_event);
  void OnGestureTap(const PointerEvent& pointer);
  void UpdateEditingState(std::string text, TextSelection selection,
                          TextRange composing, Affinity affinity) override;

  void RequestKeyboard();

  void BeginEditingIfNeeded() override;
  void BeginEditing();
  // Add new_content at current selection and replace the old one if selected.
  // If |style| is empty, follow previous style, else create new style run.
  void EditContent(std::string new_content, std::optional<TextStyle> style) {}
  void UpdateLineHeightIfNeeded();

  void SetDisabled(bool disabled);
  void SetContent(const std::string& content);
  void SetFontSize(float font_size);
  void SetLineHeight(float line_height);
  void SetPlaceholder(const std::string& placeholder);
  void SetPlaceholderFontSize(float font_size);
  void SetPlaceholderFontWeight(FontWeight font_weight);
  void SetPlaceholderColor(const Color& color);

  // Lynx module UI method
#define UI_METHOD_LIST_DECLARATION(V) \
  V(addText)                          \
  V(sendDelEvent)                     \
  V(controlKeyBoard)                  \
  V(setInputFilter)                   \
  V(select)                           \
  V(beginEdit)                        \
  V(quitEdit)
  UI_METHOD_LIST_DECLARATION(UI_METHOD_DEF);
#undef UI_METHOD_LIST_DECLARATION

  double PlaceholderHeight() { return placeholder_height_; }

 private:
  bool disabled_ = false;

  std::string placeholder_;
  std::optional<float> placeholder_font_size_;
  std::optional<FontWeight> placeholder_font_weight_;
  std::optional<Color> placeholder_color_;
  bool begin_edit_on_focus_ = true;

  bool send_composing_input_ = false;
  float placeholder_height_ = 0.f;

  // The unit is px
  std::optional<float> line_height_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_EDITABLE_INPUT_VIEW_H_
