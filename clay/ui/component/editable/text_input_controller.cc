// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/editable/text_input_controller.h"

#include <stdint.h>

#include <algorithm>
#include <memory>
#include <utility>

#include "clay/fml/logging.h"
#include "clay/ui/common/input_client_manager.h"
#include "clay/ui/common/text_input_type_traits.h"
#include "clay/ui/component/editable/editable_view.h"
#include "clay/ui/component/editable/ime_utils.h"
#include "clay/ui/component/page_view.h"

namespace clay {

TextInputController::TextInputController(PageView* page_view, int client_id,
                                         TextInputClient* client)
    : page_view_(page_view),
      client_(client),
      client_id_(client_id),
      weak_factory_(this) {
#if defined(OS_WIN) || defined(OS_MAC) || defined(ENABLE_HEADLESS)
  auto manager = page_view_->GetInputClientManager();
  InputClientManager::TextInputCallback callback;

  callback.on_update_edit_state =
      [weak = weak_factory_.GetWeakPtr()](
          uint64_t selection_base, uint64_t composing_extent,
          const char* selection_affinity, const char* text,
          uint64_t selection_extent, uint64_t composing_base) {
        std::string text_str = std::string(text);
        Affinity selection_affinity_value = Affinity::kDownstream;
        if (selection_affinity != nullptr) {
          if (strcmp(selection_affinity, "TextAffinity.downstream") == 0) {
            selection_affinity_value = Affinity::kDownstream;
          } else if (strcmp(selection_affinity, "TextAffinity.upstream") == 0) {
            selection_affinity_value = Affinity::kUpstream;
          }
        }
        weak->UpdateEditingState(
            text_str,
            TextSelection((int)selection_base, (int)selection_extent,
                          selection_affinity_value),
            TextRange(std::max((int)composing_base, 0),
                      std::max((int)composing_extent, 0)),
            selection_affinity_value);
      };

  callback.on_perform_action = [weak = weak_factory_.GetWeakPtr()]() {
    weak->PerformAction();
  };

  manager->AddClientCallback(client_id, callback);
#endif
}

TextInputController::~TextInputController() {
  page_view_->GetInputClientManager()->RemoveClientCallback(client_id_);
}

void TextInputController::SetClient(int client_id, KeyboardAction input_action,
                                    KeyboardInputType input_type) {
#if defined(OS_WIN) || defined(OS_MAC) || defined(ENABLE_HEADLESS)
  const char* input_type_chr =
      multiline_ ? "TextInputType.multiline" : "TextInputType.text";
  page_view_->SetTextInputClient(client_id, ToKeyboardActionType(input_action),
                                 input_type_chr);
  connect_keyboard_ = true;
#endif
}

void TextInputController::ClearClient() {
#if defined(OS_WIN) || defined(OS_MAC) || defined(ENABLE_HEADLESS)
  page_view_->ClearTextInputClient();
  connect_keyboard_ = false;
#endif
}

void TextInputController::SetEditableTransform(const Transform& transform) {
#if defined(OS_WIN) || defined(OS_MAC) || defined(ENABLE_HEADLESS)
  const auto& matrix = transform.matrix();
  float transform_matrix[16] = {
      matrix.Get(0, 0), matrix.Get(1, 0), matrix.Get(2, 0), matrix.Get(3, 0),
      matrix.Get(0, 1), matrix.Get(1, 1), matrix.Get(2, 1), matrix.Get(3, 1),
      matrix.Get(0, 2), matrix.Get(1, 2), matrix.Get(2, 2), matrix.Get(3, 2),
      matrix.Get(0, 3), matrix.Get(1, 3), matrix.Get(2, 3), matrix.Get(3, 3),
  };
  page_view_->SetEditableTransform(transform_matrix);
#endif
}

void TextInputController::SetEditingState(
    const TextEditingValue& text_editing_value) {
#if defined(OS_WIN) || defined(OS_MAC) || defined(ENABLE_HEADLESS)
  page_view_->SetEditingState(
      (uint64_t)text_editing_value.selection().base(),
      (uint64_t)text_editing_value.composing_range().extent(),
      "TextAffinity.downstream", text_editing_value.GetText(), false,
      (uint64_t)text_editing_value.selection().extent(),
      (uint64_t)text_editing_value.composing_range().base());
#endif
}

void TextInputController::SetCaretRect(FloatRect rect) {
#if defined(OS_WIN) || defined(OS_MAC) || defined(ENABLE_HEADLESS)
  page_view_->SetCaretRect(rect.x(), rect.y(), rect.width(), rect.height());
#endif
}

void TextInputController::SetComposingRect(FloatRect rect) {
#if defined(OS_WIN) || defined(OS_MAC) || defined(ENABLE_HEADLESS)
  page_view_->setMarkedTextRect(rect.x(), rect.y(), rect.width(),
                                rect.height());
#endif
}

void TextInputController::Show() {
#if defined(OS_WIN) || defined(OS_MAC) || defined(ENABLE_HEADLESS)
  page_view_->ShowTextInput();
#endif
}

void TextInputController::Hide() {
#if defined(OS_WIN) || defined(OS_MAC) || defined(ENABLE_HEADLESS)
  page_view_->HideTextInput();
#endif
}

void TextInputController::UpdateEditingState(std::string text,
                                             TextSelection selection,
                                             TextRange composing,
                                             Affinity affinity) {
  client_->UpdateEditingState(text, selection, composing, affinity);
}

void TextInputController::PerformAction() { client_->PerformAction(); }

void TextInputController::SetMultiline(bool multiline) {
  multiline_ = multiline;
}

}  // namespace clay
