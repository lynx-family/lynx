// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/gesture/pointer_router.h"

#include <algorithm>

#include "clay/fml/logging.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/gesture/macros.h"

namespace clay {

void PointerRouter::AddRoute(int pointer_id, RouteId id, PointerRoute&& route) {
  routes_[pointer_id].emplace_back(id, std::move(route));
}

void PointerRouter::RemoveRoute(int pointer_id, RouteId id) {
  auto list_iter = routes_.find(pointer_id);
  if (list_iter == routes_.end()) {
    // Maybe gesture recognizer has been destructed.
    return;
  }

  std::list<std::pair<RouteId, PointerRoute>>& route_list = list_iter->second;
  auto route_iter = route_list.begin();
  bool erased = false;
  for (; route_iter != route_list.end(); ++route_iter) {
    if (route_iter->first == id) {
      route_list.erase(route_iter);
      erased = true;
      break;
    }
  }
  FML_DCHECK(erased);

  if (route_list.empty()) {
    routes_.erase(list_iter);
  }
}

void PointerRouter::Route(const PointerEvent& event) {
  auto list_iter = routes_.find(event.pointer_id);
  if (list_iter == routes_.end()) {
    return;
  }

  // Copy route list in case it will be removed in callback.
  auto route_list = list_iter->second;
  auto& list_ref = list_iter->second;
  for (std::pair<RouteId, PointerRoute>& route_pair : route_list) {
    // verify that the route still exists in the list
    if (std::any_of(list_ref.begin(), list_ref.end(),
                    [&route_pair](auto& pair) {
                      return pair.first == route_pair.first;
                    })) {
      route_pair.second(event);
    }
  }
}

}  // namespace clay
