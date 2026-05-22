// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_POINTER_ROUTER_H_
#define CLAY_UI_GESTURE_POINTER_ROUTER_H_

#include <functional>
#include <list>
#include <map>
#include <utility>

namespace clay {

struct PointerEvent;

using PointerRoute = std::function<void(const PointerEvent&)>;
// As std::function can not do comparison, so use identifier.
using RouteId = uint64_t;

inline RouteId GenerateRouteId() {
  static RouteId id = 0;
  return id++;
}

class PointerRouter {
 public:
  void AddRoute(int pointer_id, RouteId id, PointerRoute&& route);
  void RemoveRoute(int pointer_id, RouteId id);
  void Route(const PointerEvent& event);

 private:
  // Routes within one pointer_id must be ordered, so use list instead of map.
  std::map<int, std::list<std::pair<RouteId, PointerRoute>>> routes_;
};

}  // namespace clay

#endif  // CLAY_UI_GESTURE_POINTER_ROUTER_H_
