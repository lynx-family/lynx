// Copyright 2017 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_BASE_OBSERVER_OBSERVER_LIST_H_
#define CORE_BASE_OBSERVER_OBSERVER_LIST_H_

#include <list>

#include "core/base/lynx_export.h"

namespace lynx {
namespace base {
class Observer;
class ObserverList {
 public:
  ObserverList() {}

  LYNX_EXPORT_FOR_DEVTOOL void AddObserver(Observer* obs);
  LYNX_EXPORT_FOR_DEVTOOL void RemoveObserver(Observer* obs);
  void Clear();
  LYNX_EXPORT_FOR_DEVTOOL void ForEachObserver();

 private:
  std::list<Observer*> list_;
};
}  // namespace base
}  // namespace lynx

#endif  // CORE_BASE_OBSERVER_OBSERVER_LIST_H_
