// Copyright 2017 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_OBSERVER_OBSERVER_H_
#define CORE_BASE_OBSERVER_OBSERVER_H_

namespace lynx {
namespace base {
class Observer {
 public:
  Observer() : previous_(nullptr), next_(nullptr) {}
  virtual ~Observer() {}
  virtual void Update() = 0;
  friend class ObserverList;

 private:
  Observer* previous_;
  Observer* next_;
};
}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_OBSERVER_OBSERVER_H_
