// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_OBSERVER_OWNED_OBSERVER_LIST_H_
#define CORE_OBSERVER_OWNED_OBSERVER_LIST_H_

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace lynx {
namespace observer {

template <typename Observer>
class OwnedObserverList {
 public:
  OwnedObserverList() = default;
  ~OwnedObserverList() = default;

  OwnedObserverList(const OwnedObserverList&) = delete;
  OwnedObserverList& operator=(const OwnedObserverList&) = delete;
  OwnedObserverList(OwnedObserverList&&) = default;
  OwnedObserverList& operator=(OwnedObserverList&&) = default;

  void Add(std::unique_ptr<Observer> observer) {
    if (observer) {
      observers_.emplace_back(std::move(observer));
    }
  }

  bool empty() const { return observers_.empty(); }
  size_t size() const { return observers_.size(); }

  template <typename Callback>
  void ForEach(Callback&& callback) {
    for (const auto& observer : observers_) {
      callback(*observer);
    }
  }

 private:
  std::vector<std::unique_ptr<Observer>> observers_;
};

}  // namespace observer
}  // namespace lynx

#endif  // CORE_OBSERVER_OWNED_OBSERVER_LIST_H_
