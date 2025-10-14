// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_EDITABLE_TEXT_EDITING_CONTROLLER_H_
#define CLAY_UI_COMPONENT_EDITABLE_TEXT_EDITING_CONTROLLER_H_

#include <string>
#include <utility>
#include <vector>

#include "clay/ui/common/text_input_type_traits.h"

namespace clay {

class TextEditingController {
 public:
  TextEditingController() = default;
  explicit TextEditingController(const TextEditingValue& value)
      : value_(value) {}
  explicit TextEditingController(TextEditingValue&& value)
      : value_(std::move(value)) {}
  enum ValueChangeType : int {
    kNone = 0,
    kSelection = 1,
    kContent = 2,
    kUserInput = 4,
  };
  class Observer {
   public:
    virtual void OnValueChanged(const TextEditingValue& value,
                                const TextEditingController*) = 0;
    virtual void OnSelectionChanged(const TextEditingValue& value,
                                    const TextEditingController*) = 0;
    virtual void OnUserInput(const TextEditingValue& value,
                             const TextEditingController*) = 0;
    virtual void OnContentChanged(const TextEditingValue& value,
                                  const TextEditingController*) = 0;
  };
  void AddObserver(Observer* observer);

  // As same as common sense, observers must be removed before destroyed.
  // Must NOT RemoveObserver during Observer::OnValueChanged callback!
  void RemoveObserver(Observer* observer);

  void NotifyValueChanged(int type);

  bool SetValue(const TextEditingValue& value, bool trigger_input_event,
                bool need_update_remote);

  bool MoveSelectionToEndIfNeeded();

  const TextEditingValue& GetValue() { return value_; }
  bool NeedUpdateRemote() const { return need_update_remote_; }
  void SetNeedUpdateRemote(bool need_update_remote) {
    need_update_remote_ = need_update_remote;
  }

 private:
  std::vector<Observer*> observers_;
  TextEditingValue value_;
  bool selection_set_ = false;
  bool need_update_remote_ = false;
};

};  // namespace clay

#endif  // CLAY_UI_COMPONENT_EDITABLE_TEXT_EDITING_CONTROLLER_H_
