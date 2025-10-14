// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/view_tree_observer.h"

#include <algorithm>

namespace clay {

void ViewTreeObserver::AddOnPaintingListener(OnPaintingListener* listener) {
  if (!listener) {
    return;
  }
  if (std::find(on_painting_listeners_.begin(), on_painting_listeners_.end(),
                listener) != on_painting_listeners_.end()) {
    // Already exists, do not add duplicate.
    return;
  }
  on_painting_listeners_.push_back(listener);
}

void ViewTreeObserver::RemoveOnPaintingListener(OnPaintingListener* listener) {
  if (listener) {
    on_painting_listeners_.remove(listener);
  }
}

void ViewTreeObserver::DispatchOnPainting() {
  if (on_painting_listeners_.empty()) {
    return;
  }
  // Use a temporary list to avoid the list being modified in the loop.
  std::list<OnPaintingListener*> listeners = on_painting_listeners_;
  for (auto& listener : on_painting_listeners_) {
    listener->OnPainting();
  }
}

}  // namespace clay
