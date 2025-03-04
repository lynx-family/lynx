// Copyright 2017 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/observer/observer_list.h"

#include "core/base/observer/observer.h"

namespace lynx {
namespace base {

void ObserverList::AddObserver(Observer* obs) { list_.emplace_back(obs); }

void ObserverList::RemoveObserver(Observer* obs) { list_.remove(obs); }

void ObserverList::ForEachObserver() {
  for (auto it = list_.begin(); it != list_.end();) {
    Observer* obs = *it;
    it = list_.erase(it);
    obs->Update();
  }
}
}  // namespace base
}  // namespace lynx
