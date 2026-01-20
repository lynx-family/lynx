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

  void SetAttribute(const char* attr_c, const clay::Value& value) override;
  void UpdateEditingState(std::string text, TextSelection selection,
                          TextRange composing, Affinity affinity) override;

  void RequestKeyboard();
  // Add new_content at current selection and replace the old one if selected.
  // If |style| is empty, follow previous style, else create new style run.
  void EditContent(std::string new_content, std::optional<TextStyle> style) {}
  void UpdateLineHeightIfNeeded();

  void SetContent(const std::string& content);
  void SetFontSize(float font_size);
  void SetLineHeight(float line_height);
  void SetSendComposingInput(bool send_composing_input) {
    send_composing_input_ = send_composing_input;
  }

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

 private:
  bool begin_edit_on_focus_ = true;

  bool send_composing_input_ = false;
  // The unit is px
  std::optional<float> line_height_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_EDITABLE_INPUT_VIEW_H_
