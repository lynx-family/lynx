// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMMON_UTILS_VALUE_CHANGE_NOTIFIER_H_
#define CLAY_UI_COMMON_UTILS_VALUE_CHANGE_NOTIFIER_H_

#include <unordered_set>
#include <utility>

#include "clay/fml/logging.h"

namespace clay {

// An observer pattern util class for monitor changes of a given value.
// The value being observed must correctly implements operator ==.
// Notice: Used for single-thread only.
template <typename T>
class ValueChangeNotifier {
 public:
  ValueChangeNotifier() = default;
  explicit ValueChangeNotifier(const T& value) : value_(value) {}
  explicit ValueChangeNotifier(T&& value) : value_(std::move(value)) {}

  class Observer {
   public:
    virtual void OnValueChanged(const T& value, const ValueChangeNotifier*) = 0;
  };

  void AddObserver(Observer* observer) {
    FML_CHECK(observer);
#ifndef NDEBUG
    FML_DCHECK(!notifying_) << "Mustn't add observer during OnValueChanged.";
#endif
    observers_.emplace(observer);
  }

  // As same as common sense, observers must be removed before destroyed.
  // Must NOT RemoveObserver during Observer::OnValueChanged callback!
  void RemoveObserver(Observer* observer) {
    FML_CHECK(observer);
#ifndef NDEBUG
    FML_DCHECK(!notifying_) << "Mustn't remove observer during OnValueChanged.";
#endif
    if (!observers_.erase(observer)) {
      FML_DCHECK(false) << "Did you forget add observer?";
    }
  }

  void NotifyValueChanged() {
#ifndef NDEBUG
    notifying_ = true;
#endif
    for (Observer* observer : observers_) {
      observer->OnValueChanged(value_, this);
    }
#ifndef NDEBUG
    notifying_ = false;
#endif
  }

  bool SetValue(const T& value) {
    if (value == value_) {
      return false;
    }
    value_ = value;
    NotifyValueChanged();
    return true;
  }

  T GetValue() { return value_; }

 private:
  std::unordered_set<Observer*> observers_;
  T value_;
#ifndef NDEBUG
  bool notifying_ = false;
#endif
};

}  // namespace clay

#endif  // CLAY_UI_COMMON_UTILS_VALUE_CHANGE_NOTIFIER_H_
