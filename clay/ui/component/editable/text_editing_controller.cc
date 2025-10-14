// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/editable/text_editing_controller.h"

#include <string>
#include <vector>

#include "clay/ui/common/text_input_type_traits.h"

namespace clay {

void TextEditingController::AddObserver(Observer* observer) {
  FML_CHECK(observer);
  observers_.emplace_back(observer);
}

void TextEditingController::RemoveObserver(Observer* observer) {
  FML_CHECK(observer);
  auto position = std::find(observers_.begin(), observers_.end(), observer);
  if (position == observers_.end()) {
    FML_DCHECK(false) << "Did you forget add observer?";
    return;
  }
  observers_.erase(position);
}

void TextEditingController::NotifyValueChanged(int type) {
  for (Observer* observer : observers_) {
    observer->OnValueChanged(value_, this);
    if (type & kUserInput) {
      observer->OnUserInput(value_, this);
    }
    if (type & kContent) {
      observer->OnContentChanged(value_, this);
    }
    if (type & kSelection) {
      observer->OnSelectionChanged(value_, this);
    }
  }
}

bool TextEditingController::SetValue(const TextEditingValue& value,
                                     bool trigger_input_event,
                                     bool need_update_remote) {
  need_update_remote_ = need_update_remote;
  if (value == value_) {
    return false;
  }
  int type = ValueChangeType::kNone;
  if (!(value_.selection() == value.selection())) {
    type |= ValueChangeType::kSelection;
    selection_set_ = true;
  }
  if (value_.GetText() != value.GetText()) {
    type |= ValueChangeType::kContent;
  }
  if (trigger_input_event) {
    type |= ValueChangeType::kUserInput;
  }
  value_ = value;
  NotifyValueChanged(type);
  return true;
}

bool TextEditingController::MoveSelectionToEndIfNeeded() {
  if (!selection_set_) {
    value_.SetSelection(TextRange{value_.GetU16Text().length()});
    selection_set_ = true;
    return true;
  }
  return false;
}

};  // namespace clay
