// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_EDITABLE_TEXT_INPUT_CONTROLLER_H_
#define CLAY_UI_COMPONENT_EDITABLE_TEXT_INPUT_CONTROLLER_H_

#include <string>

#include "base/include/fml/memory/weak_ptr.h"
#include "clay/gfx/geometry/float_rect.h"
#include "clay/gfx/geometry/transform.h"
#include "clay/ui/common/text_input_type_traits.h"
#include "clay/ui/common/text_selection.h"
#include "clay/ui/component/editable/ime_utils.h"
#include "clay/ui/event/key_event.h"

namespace clay {

class PageView;

enum class TextAffinity {
  kDownstream,
  kUpstream,
};

class TextInputClient {
 public:
  virtual void UpdateEditingState(std::string text, TextSelection selection,
                                  TextRange composing, Affinity affinity) = 0;
  virtual void PerformAction() = 0;
};

class TextInputController {
 public:
  explicit TextInputController(PageView* page_view, int client_id,
                               TextInputClient* client);
  ~TextInputController();

  void SetClient(int client_id, KeyboardAction input_action,
                 KeyboardInputType input_type);
  void ClearClient();
  void SetEditableTransform(const Transform& transform);
  void SetEditingState(const TextEditingValue& text_editing_value);
  void SetCaretRect(FloatRect rect);
  void SetComposingRect(FloatRect rect);
  void Show();
  void Hide();
  void InputFilterAsync(const std::string& input, const std::string& pattern,
                        std::function<void(const std::string&)> callback);

  void UpdateEditingState(std::string text, TextSelection selection,
                          TextRange composing, Affinity affinity);
  void PerformAction();

  bool ConnectKeyboard() { return connect_keyboard_; }

  void SetMultiline(bool multiline);

 private:
  [[maybe_unused]] PageView* page_view_ = nullptr;
  TextInputClient* client_ = nullptr;
  [[maybe_unused]] int client_id_;
  bool connect_keyboard_ = false;
  bool multiline_ = true;

  fml::WeakPtrFactory<TextInputController> weak_factory_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_EDITABLE_TEXT_INPUT_CONTROLLER_H_
