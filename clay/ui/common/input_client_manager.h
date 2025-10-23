// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMMON_INPUT_CLIENT_MANAGER_H_
#define CLAY_UI_COMMON_INPUT_CLIENT_MANAGER_H_

#include <functional>
#include <unordered_map>

#include "clay/fml/logging.h"

namespace clay {

class InputClientManager {
 public:
  InputClientManager() = default;
  ~InputClientManager() = default;

  struct TextInputCallback {
    std::function<void(uint64_t selection_base, uint64_t composing_extent,
                       const char* selection_affinity, const char* text,
                       uint64_t selection_extent, uint64_t composing_base)>
        on_update_edit_state;
    std::function<void()> on_perform_action;
  };

  void AddClientCallback(int client_id, const TextInputCallback& callback) {
    text_input_callbacks_[client_id] = callback;
  }

  void RemoveClientCallback(int client_id) {
    text_input_callbacks_.erase(client_id);
  }

  void InvokeUpdateEditState(int client_id, uint64_t selection_base,
                             uint64_t composing_extent,
                             const char* selection_affinity, const char* text,
                             uint64_t selection_extent,
                             uint64_t composing_base) {
    if (text_input_callbacks_.find(client_id) == text_input_callbacks_.end()) {
      FML_LOG(ERROR) << "InputClientManager: client_id not found";
      return;
    }
    text_input_callbacks_[client_id].on_update_edit_state(
        selection_base, composing_extent, selection_affinity, text,
        selection_extent, composing_base);
  }

  void InvokePerformAction(int client_id) {
    if (text_input_callbacks_.find(client_id) == text_input_callbacks_.end()) {
      FML_LOG(ERROR) << "InputClientManager: client_id not found";
      return;
    }
    text_input_callbacks_[client_id].on_perform_action();
  }

 private:
  std::unordered_map<int, TextInputCallback> text_input_callbacks_;
};

}  // namespace clay

#endif  // CLAY_UI_COMMON_INPUT_CLIENT_MANAGER_H_
