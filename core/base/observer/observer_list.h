// Copyright 2017 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_OBSERVER_OBSERVER_LIST_H_
#define CORE_BASE_OBSERVER_OBSERVER_LIST_H_

#include <list>

#include "base/include/base_export.h"

namespace lynx {
namespace base {
class Observer;
class ObserverList {
 public:
  ObserverList() {}

  BASE_EXPORT_FOR_DEVTOOL void AddObserver(Observer* obs);
  BASE_EXPORT_FOR_DEVTOOL void RemoveObserver(Observer* obs);
  void Clear();
  BASE_EXPORT_FOR_DEVTOOL void ForEachObserver();

 private:
  std::list<Observer*> list_;
};
}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_OBSERVER_OBSERVER_LIST_H_
