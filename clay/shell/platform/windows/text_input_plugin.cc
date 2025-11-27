// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/windows/text_input_plugin.h"

#include <windows.h>

#include <algorithm>
#include <codecvt>
#include <cstdint>
#include <regex>
#include <string>
#include <utility>

#include "base/include/string/string_utils.h"
#include "clay/fml/platform/win/wstring_conversion.h"
#include "clay/gfx/geometry/float_point.h"
#include "clay/shell/platform/common/text_editing_delta.h"
#include "clay/shell/platform/windows/flutter_windows_engine.h"

static constexpr char kAffinityDownstream[] = "TextAffinity.downstream";

namespace clay {

void TextInputPlugin::TextHook(const std::u16string& text) {
  if (active_model_ == nullptr) {
    return;
  }
  std::u16string text_before_change =
      lynx::base::U8StringToU16(active_model_->GetText());
  TextRange selection_before_change = active_model_->selection();
  active_model_->AddText(text);

  if (enable_delta_model) {
    TextEditingDelta delta =
        TextEditingDelta(text_before_change, selection_before_change, text);
    SendStateUpdateWithDelta(*active_model_, &delta);
  } else {
    SendStateUpdate(*active_model_);
  }
}

void TextInputPlugin::KeyboardHook(int key, int scancode, int action,
                                   char32_t character, bool extended,
                                   bool was_down) {
  if (active_model_ == nullptr) {
    return;
  }
  if (action == WM_KEYDOWN || action == WM_SYSKEYDOWN) {
    // Most editing keys (arrow keys, backspace, delete, etc.) are handled in
    // the framework, so don't need to be handled at this layer.
    switch (key) {
      case VK_RETURN:
        EnterPressed(active_model_.get());
        break;
      default:
        break;
    }
  }
}

TextInputPlugin::TextInputPlugin(TextInputPluginDelegate* delegate,
                                 FlutterWindowsEngine* engine,
                                 SendEvent send_event)
    : delegate_(delegate),
      engine_(engine),
      active_model_(nullptr),
      send_event_(send_event),
      weak_factory_(this) {}

TextInputPlugin::~TextInputPlugin() {}

void TextInputPlugin::ComposeBeginHook() {
  if (active_model_ == nullptr) {
    return;
  }
  active_model_->BeginComposing();
  if (enable_delta_model) {
    std::string text = active_model_->GetText();
    TextRange selection = active_model_->selection();
    TextEditingDelta delta = TextEditingDelta(text);
    SendStateUpdateWithDelta(*active_model_, &delta);
  } else {
    SendStateUpdate(*active_model_);
  }
}

void TextInputPlugin::ComposeCommitHook() {
  if (active_model_ == nullptr) {
    return;
  }
  std::string text_before_change = active_model_->GetText();
  TextRange selection_before_change = active_model_->selection();
  TextRange composing_before_change = active_model_->composing_range();
  std::string composing_text_before_change = text_before_change.substr(
      composing_before_change.start(), composing_before_change.length());
  active_model_->CommitComposing();

  // We do not trigger SendStateUpdate here.
  //
  // Until a WM_IME_ENDCOMPOSING event, the user is still composing from the OS
  // point of view. Commit events are always immediately followed by another
  // composing event or an end composing event. However, in the brief window
  // between the commit event and the following event, the composing region is
  // collapsed. Notifying the framework of this intermediate state will trigger
  // any framework code designed to execute at the end of composing, such as
  // input formatters, which may try to update the text and send a message back
  // to the engine with changes.
  //
  // This is a particular problem with Korean IMEs, which build up one
  // character at a time in their composing region until a keypress that makes
  // no sense for the in-progress character. At that point, the result
  // character is committed and a compose event is immedidately received with
  // the new composing region.
  //
  // In the case where this event is immediately followed by a composing event,
  // the state will be sent in ComposeChangeHook.
  //
  // In the case where this event is immediately followed by an end composing
  // event, the state will be sent in ComposeEndHook.
}

void TextInputPlugin::ComposeEndHook() {
  if (active_model_ == nullptr) {
    return;
  }
  std::string text_before_change = active_model_->GetText();
  TextRange selection_before_change = active_model_->selection();
  active_model_->CommitComposing();
  active_model_->EndComposing();
  if (enable_delta_model) {
    std::string text = active_model_->GetText();
    TextEditingDelta delta = TextEditingDelta(text);
    SendStateUpdateWithDelta(*active_model_, &delta);
  } else {
    SendStateUpdate(*active_model_);
  }
}

void TextInputPlugin::ComposeChangeHook(const std::u16string& text,
                                        int cursor_pos) {
  if (!!send_event_ && cursor_pos == 0) {
    std::string utf8 = lynx::base::U16StringToU8(text);
    ClayKeyEvent event{
        .struct_size = sizeof(ClayKeyEvent),
        .timestamp = static_cast<double>(
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch())
                .count()),
        .type = kClayKeyEventTypeDown,
        .physical = 0,  // unknown
        .logical = 0,   // unknown
        .character = utf8.data(),
        .synthesized = false,
    };
    auto callback = [](bool handled, void* user_data) -> void {
      // do nothing
    };
    send_event_(event, callback, nullptr);
  }

  if (active_model_ == nullptr) {
    return;
  }
  std::string text_before_change = active_model_->GetText();
  TextRange composing_before_change = active_model_->composing_range();
  active_model_->AddText(text);
  cursor_pos += active_model_->composing_range().extent();
  active_model_->UpdateComposingText(text);
  active_model_->SetSelection(TextRange(cursor_pos, cursor_pos));
  std::string text_after_change = active_model_->GetText();
  if (enable_delta_model) {
    TextEditingDelta delta =
        TextEditingDelta(lynx::base::U8StringToU16(text_before_change),
                         composing_before_change, text);
    SendStateUpdateWithDelta(*active_model_, &delta);
  } else {
    SendStateUpdate(*active_model_);
  }
}

FloatRect TextInputPlugin::GetCursorRect() const {
  FloatPoint transformed_point = {
      composing_rect_.left() * editable_text_transform_[0][0] +
          composing_rect_.top() * editable_text_transform_[1][0] +
          editable_text_transform_[3][0],
      composing_rect_.left() * editable_text_transform_[0][1] +
          composing_rect_.top() * editable_text_transform_[1][1] +
          editable_text_transform_[3][1]};
  return {transformed_point, composing_rect_.size()};
}

void TextInputPlugin::SendStateUpdate(const TextInputModel& model) {
  TextRange selection = model.selection();
  engine_->UpdateEditState(client_id_, selection.base(),
                           model.composing_range().extent(),
                           kAffinityDownstream, model.GetText().c_str(),
                           selection.extent(), model.composing_range().base());
}

void TextInputPlugin::SendStateUpdateWithDelta(const TextInputModel& model,
                                               const TextEditingDelta* delta) {}

void TextInputPlugin::EnterPressed(TextInputModel* model) {
  engine_->PerformInputAction(client_id_);
}

void TextInputPlugin::SetTextInputClient(int client_id,
                                         const char* input_action,
                                         const char* input_type) {
  client_id_ = client_id;
  enable_delta_model = false;
  input_action_ = std::string(input_action);
  input_type_ = std::string(input_type);
  active_model_ = std::make_unique<TextInputModel>();
  delegate_->OnTextInputClientChange(client_id_);
}

void TextInputPlugin::ClearTextInputClient() {
  if (active_model_ != nullptr && active_model_->composing()) {
    active_model_->CommitComposing();
    active_model_->EndComposing();
    SendStateUpdate(*active_model_);
  }
  delegate_->OnResetImeComposing();
  active_model_ = nullptr;
  client_id_ = -1;
  delegate_->OnTextInputClientChange(client_id_);
}

void TextInputPlugin::SetEditableTransform(const float transform[16]) {
  for (int i = 0; i < 16; ++i) {
    editable_text_transform_[i / 4][i % 4] = transform[i];
  }
  FloatRect transformed_rect = GetCursorRect();
  delegate_->OnCursorRectUpdated(transformed_rect);
}

void TextInputPlugin::SetEditingState(uint64_t selection_base,
                                      uint64_t composing_extent,
                                      const char* selection_affinity,
                                      const char* text,
                                      bool selection_directional,
                                      uint64_t selection_extent,
                                      uint64_t composing_base) {
  if (active_model_ == nullptr) {
    FML_LOG(ERROR)
        << "Set editing state has been invoked, but no client is set.";
    return;
  }
  if (!text) {
    FML_LOG(ERROR) << "Set editing state has been invoked, but without text.";
    return;
  }
  auto text_str = std::string(text);
  // Flutter uses -1/-1 for invalid; translate that to 0/0 for the model.
  if ((int)selection_base == -1 && (int)selection_extent == -1) {
    selection_base = selection_extent = 0;
  }
  active_model_->SetText(text_str);
  active_model_->SetSelection(
      TextRange((int)selection_base, (int)selection_extent));

  if ((int)composing_base == -1 && (int)composing_extent == -1) {
    active_model_->EndComposing();
  } else {
    int composing_start = std::min((int)composing_base, (int)composing_extent);
    int cursor_offset = (int)selection_base - composing_start;
    active_model_->SetComposingRange(
        TextRange((int)composing_base, (int)composing_extent), cursor_offset);
  }
  // Add send event to handle text input model modified by clay.
  SendStateUpdate(*active_model_);
}

void TextInputPlugin::SetMarkedTextRect(float x, float y, float width,
                                        float height) {
  composing_rect_ = {{x, y}, {width, height}};
  FloatRect transformed_rect = GetCursorRect();
  delegate_->OnCursorRectUpdated(transformed_rect);
}

void TextInputPlugin::ShowTextInput() {
  // This method is no-ops.
}

void TextInputPlugin::HideTextInput() {
  // This method is no-ops.
}

std::string TextInputPlugin::FilterInput(const std::string& input,
                                         const std::string& pattern) {
  auto w_input = fml::Utf8ToWideString(input);
  auto w_pattern = fml::Utf8ToWideString(pattern);
  std::wregex re(w_pattern);
  auto result = std::regex_replace(w_input, re, L"");
  return fml::WideStringToUtf8(result);
}

}  // namespace clay
