// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_MOUSE_REGION_MANAGER_H_
#define CLAY_UI_GESTURE_MOUSE_REGION_MANAGER_H_

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "build/build_config.h"
#if defined(OS_WIN) || defined(OS_MAC)
#include "clay/gfx/animation/animation_handler.h"
#endif
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/gesture/hit_test.h"
#include "clay/ui/gesture/mouse_cursor_manager.h"

namespace clay {

class HitTestTarget;
class BaseView;

class MouseRegionManager
#if defined(OS_WIN) || defined(OS_MAC)
    : private AnimationHandler::AnimationFrameCallback
#endif
{
 public:
  MouseRegionManager() = default;
  ~MouseRegionManager();
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
#if defined(OS_WIN) || defined(OS_MAC)
  void HandlePointerEventBefore(BaseView* root, const PointerEvent& event);
  void HandlePointerEventAfter(BaseView* root, const PointerEvent& event);
  void RefreshPointerEventTarget(BaseView* root, const PointerEvent& event);
  void SchedulePointerEventTargetRefresh(BaseView* root);
  const PointerEvent* GetLastPointerEvent(const PointerEvent& event) const;
#endif

  // init sub manager. e.g. MouseCursorManager
  void InitSubManager(
      MouseCursorManager::ActiveCursorCallback active_cursor_callback);

  void AddCursorHolder(BaseView* holder);

  void ForceUpdateCursor();
#if defined(OS_WIN) || defined(OS_MAC)
  void Reset();
#endif

 private:
  struct MouseRegionRoute {
    EnterCallback on_enter = nullptr;
    LeaveCallback on_leave = nullptr;
    HoverCallback on_hover = nullptr;
  };

  using ViewChain = std::list<fml::WeakPtr<BaseView>>;

  ViewChain BuildViewChain(BaseView* root, const PointerEvent& event,
                           BaseView** top_view = nullptr) const;
#if defined(OS_WIN) || defined(OS_MAC)
  using PointerKey = std::pair<PointerEvent::DeviceType, int>;
  bool DoAnimationFrame(int64_t, bool = true) override;
  void UpdatePointerChain(const PointerEvent& event,
                          const ViewChain& view_chain);
  void RefreshPointerEventTargets(BaseView* root);
  static int GetTargetSign(const ViewChain& chain);
#endif

  std::map<BaseView*, MouseRegionRoute> mouse_region_routes_;
  std::list<fml::WeakPtr<BaseView>> prev_chain_;
#if defined(OS_WIN) || defined(OS_MAC)
  std::map<PointerKey, ViewChain> pointer_chains_;
  std::map<PointerKey, PointerEvent> last_pointer_events_;
  std::set<PointerKey> implicitly_captured_pointers_;
  fml::WeakPtr<BaseView> pending_refresh_root_;
#endif
  std::unique_ptr<MouseCursorManager> mouse_cursor_manager_;
};

}  // namespace clay

#endif  // CLAY_UI_GESTURE_MOUSE_REGION_MANAGER_H_
