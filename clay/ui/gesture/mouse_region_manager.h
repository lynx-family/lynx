// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_MOUSE_REGION_MANAGER_H_
#define CLAY_UI_GESTURE_MOUSE_REGION_MANAGER_H_

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <vector>

#include "clay/ui/event/gesture_event.h"
#include "clay/ui/gesture/hit_test.h"
#include "clay/ui/gesture/mouse_cursor_manager.h"

namespace clay {

class HitTestTarget;
class BaseView;

class MouseRegionManager {
 public:
  MouseRegionManager() = default;
  MouseRegionManager(const MouseRegionManager&) = delete;
  MouseRegionManager& operator=(const MouseRegionManager&) = delete;

  using EnterCallback = std::function<void(const PointerEvent&)>;
  using LeaveCallback = std::function<void(const PointerEvent&)>;
  using HoverCallback = std::function<void(const PointerEvent&)>;

  void RegisterEnterCallback(BaseView* target, EnterCallback callback);
  void RegisterLeaveCallback(BaseView* target, LeaveCallback callback);
  void RegisterHoverCallback(BaseView* target, HoverCallback callback);

  void UnregisterCallback(BaseView* target);

  void HandleEvents(BaseView* root, const std::vector<PointerEvent>& events);
  void HandleEvent(BaseView* root, const PointerEvent& event);

  // init sub manager. e.g. MouseCursorManager
  void InitSubManager(
      MouseCursorManager::ActiveCursorCallback active_cursor_callback);

  void AddCursorHolder(BaseView* holder);

 private:
  struct MouseRegionRoute {
    EnterCallback on_enter = nullptr;
    LeaveCallback on_leave = nullptr;
    HoverCallback on_hover = nullptr;
  };

  std::map<BaseView*, MouseRegionRoute> mouse_region_routes_;
  std::list<fml::WeakPtr<BaseView>> prev_chain_;
  std::unique_ptr<MouseCursorManager> mouse_cursor_manager_;
};

}  // namespace clay

#endif  // CLAY_UI_GESTURE_MOUSE_REGION_MANAGER_H_
