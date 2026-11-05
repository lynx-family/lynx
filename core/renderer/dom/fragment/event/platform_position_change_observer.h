// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_POSITION_CHANGE_OBSERVER_H_
#define CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_POSITION_CHANGE_OBSERVER_H_

#include <array>
#include <cstdint>
#include <optional>
#include <utility>

#include "base/include/fml/task_runner.h"
#include "base/include/vector.h"

namespace lynx::tasm {

class NativePaintingCtxPlatformRef;

// Sends positionchange to targets whose window rect changed. Layout and list
// scroll updates request a coalesced check; while targets exist, the root view
// window location is sampled at the exposure interval to catch host movement.
class PlatformPositionChangeObserver {
 public:
  PlatformPositionChangeObserver(NativePaintingCtxPlatformRef* platform_ref,
                                 fml::RefPtr<fml::TaskRunner> task_runner)
      : platform_ref_(platform_ref), task_runner_(std::move(task_runner)) {}

  void UpdateTarget(int32_t sign, bool listens);
  void Clear();
  void RequestCheck();
  // Sends pending changes now, e.g. before a tap that may read the positions.
  void DispatchPositionChange();

 private:
  void ScheduleRootViewCheck();
  void CheckRootViewLocation();
  // Returns false while the root view has no valid window location.
  bool GetWindowOrigin(float origin[2]);

  NativePaintingCtxPlatformRef* platform_ref_{nullptr};
  fml::RefPtr<fml::TaskRunner> task_runner_;
  // Listener sign -> last window rect, absent until the first valid sample.
  base::OrderedFlatMap<int32_t, std::optional<std::array<float, 4>>> targets_;
  std::array<float, 2> root_view_location_{0.f, 0.f};
  bool check_pending_{false};
  bool root_view_check_pending_{false};
  // Set while the window location is unavailable, e.g. the host is detached.
  bool window_origin_lost_{false};
};

}  // namespace lynx::tasm

#endif  // CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_POSITION_CHANGE_OBSERVER_H_
