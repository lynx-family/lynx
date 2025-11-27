// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_WINDOWS_TEXT_INPUT_PLUGIN_H_
#define CLAY_SHELL_PLATFORM_WINDOWS_TEXT_INPUT_PLUGIN_H_

#include <array>
#include <map>
#include <memory>
#include <string>

#include "base/include/fml/memory/weak_ptr.h"
#include "clay/gfx/geometry/float_rect.h"
#include "clay/public/clay.h"
#include "clay/shell/platform/common/text_editing_delta.h"
#include "clay/shell/platform/common/text_input_model.h"
#include "clay/shell/platform/windows/keyboard_handler_base.h"
#include "clay/shell/platform/windows/text_input_plugin_delegate.h"

namespace clay {
class FlutterWindowsEngine;

// Implements a text input plugin.
//
// Specifically handles window events within windows.
class TextInputPlugin {
 public:
  using SendEvent = std::function<void(const ClayKeyEvent& /* event */,
                                       ClayKeyEventCallback /* callback */,
                                       void* /* user_data */)>;

  explicit TextInputPlugin(TextInputPluginDelegate* delegate,
                           FlutterWindowsEngine* engine,
                           SendEvent send_event = nullptr);

  virtual ~TextInputPlugin();

  virtual void KeyboardHook(int key, int scancode, int action,
                            char32_t character, bool extended, bool was_down);

  virtual void TextHook(const std::u16string& text);

  virtual void ComposeBeginHook();

  virtual void ComposeCommitHook();

  virtual void ComposeEndHook();

  virtual void ComposeChangeHook(const std::u16string& text, int cursor_pos);

  void SetTextInputClient(int client_id, const char* input_action,
                          const char* input_type);
  void ClearTextInputClient();
  void SetEditableTransform(const float transform[16]);
  void SetEditingState(uint64_t selection_base, uint64_t composing_extent,
                       const char* selection_affinity, const char* text,
                       bool selection_directional, uint64_t selection_extent,
                       uint64_t composing_base);
  void SetMarkedTextRect(float x, float y, float width, float height);
  void ShowTextInput();
  void HideTextInput();
  std::string FilterInput(const std::string& input, const std::string& pattern);

 private:
  // Sends the current state of the given model to the Flutter engine.
  void SendStateUpdate(const TextInputModel& model);

  // Sends the current state of the given model to the Flutter engine.
  void SendStateUpdateWithDelta(const TextInputModel& model,
                                const TextEditingDelta*);

  // Sends an action triggered by the Enter key to the Flutter engine.
  void EnterPressed(TextInputModel* model);

  // Returns the composing rect, or if IME composing mode is not active, the
  // cursor rect in the PipelineOwner root coordinate system.
  FloatRect GetCursorRect() const;

  // The associated |TextInputPluginDelegate|.
  TextInputPluginDelegate* delegate_;

  // The associated |FlutterWindowsEngine|.
  FlutterWindowsEngine* engine_;

  // The active client id.
  int client_id_;

  // The active model. nullptr if not set.
  std::unique_ptr<TextInputModel> active_model_;

  // Whether to enable that the engine sends text input updates to the framework
  // as TextEditingDeltas or as one TextEditingValue.
  // For more information on the delta model, see:
  // https://master-api.flutter.dev/flutter/services/TextInputConfiguration/enableDeltaModel.html
  bool enable_delta_model = false;

  // Keyboard type of the client. See available options:
  // https://api.flutter.dev/flutter/services/TextInputType-class.html
  std::string input_type_;

  // An action requested by the user on the input client. See available options:
  // https://api.flutter.dev/flutter/services/TextInputAction-class.html
  std::string input_action_;

  // The smallest rect, in local coordinates, of the text in the composing
  // range, or of the caret in the case where there is no current composing
  // range.
  FloatRect composing_rect_;

  // A 4x4 matrix that maps from `EditableText` local coordinates to the
  // coordinate system of `PipelineOwner.rootNode`.
  std::array<std::array<float, 4>, 4> editable_text_transform_ = {
      0.0, 0.0, 0.0, 0.0,  //
      0.0, 0.0, 0.0, 0.0,  //
      0.0, 0.0, 0.0, 0.0,  //
      0.0, 0.0, 0.0, 0.0};

  SendEvent send_event_;

  fml::WeakPtrFactory<TextInputPlugin> weak_factory_;
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_WINDOWS_TEXT_INPUT_PLUGIN_H_
