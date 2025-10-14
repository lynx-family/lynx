// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_GESTURE_MANAGER_H_
#define CLAY_UI_GESTURE_GESTURE_MANAGER_H_

#include <functional>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include "base/include/fml/task_runner.h"
#include "base/include/fml/time/timer.h"
#include "clay/common/service/service.h"
#include "clay/gfx/geometry/float_point.h"
#include "clay/gfx/pixel_helper.h"
#include "clay/shell/common/services/gesture_mediate_service.h"
#include "clay/ui/gesture/arena_manager.h"
#include "clay/ui/gesture/hit_test.h"
#include "clay/ui/gesture/hit_test_responsive_result.h"
#include "clay/ui/gesture/mouse_wheel_phase_handler.h"
#include "clay/ui/gesture/pointer_router.h"
#include "third_party/googletest/googletest/include/gtest/gtest_prod.h"  // nogncheck

namespace clay {

struct PointerEvent;
using SignalEventRoute = std::function<void(const PointerEvent&)>;

// Top level class for gestures recognition.
// It does hit test, collect hit targets and dispatches events to targets.
class GestureManager final : public PixelHelper<kPixelTypeClay>,
                             MouseWheelPhaseHandler::Delegate {
 public:
  enum class ConsumeSlideEventStatus : uint8_t {
    kUndefined,
    kUnconsumed,
    kConsumed,
  };

  explicit GestureManager(
      fml::RefPtr<fml::TaskRunner> task_runner = nullptr,
      std::shared_ptr<ServiceManager> service_manager = nullptr);
  GestureManager(const GestureManager&) = delete;
  GestureManager& operator=(const GestureManager&) = delete;

  bool HandlePointerEvents(HitTestable* root,
                           std::vector<PointerEvent>& events);

  void RegisterSignalRoute(const SignalEventRoute& route);

  PointerRouter& pointer_router() { return pointer_router_; }

  ArenaManager* arena_manager() const { return arena_manager_.get(); }

  void SetPixelRatio(float pixel_ratio) { pixel_ratio_ = pixel_ratio; }

  float DevicePixelRatio() const override { return pixel_ratio_; }

  void SetListenerForNotCaredPointer(
      std::function<void(const PointerEvent&)> cb) {
    arena_manager_->SetListenerForNotCaredPointer(cb);
  }

  const fml::RefPtr<fml::TaskRunner>& GetTaskRunner() { return task_runner_; }

  void OnGestureAccepted(int pointer_id, GestureRecognizerType type);

  const HitTestResponsiveResult& GetHitTestResponsiveResult() const {
    return hit_test_responsive_result_;
  }

  void SendSyntheticWheelEventWithPhaseEnd(const PointerEvent&) override;

  void EndMouseWheelTransactionByForce();

 private:
  bool HandlePointerEvent(HitTestable* root, PointerEvent& event);
  bool ConsumeSlideEvent(const PointerEvent& event);
  void DispatchEvent(const PointerEvent& event, HitTestResult* hit_test_result);
  void DispatchEventByRouter(const PointerEvent& event);

  void ResetHitTestTargetResponsive();
  void UpdateHitTestTargetResponsive(int pointer_id, bool is_down);

 private:
  float pixel_ratio_ = 1.f;
  std::unique_ptr<ArenaManager> arena_manager_;
  PointerRouter pointer_router_;
  // This route is used for purpose of mediating disputes over which listener
  // should handle current signal event when multiple listeners exists,
  // for example, scroll on two nested scrollable view.
  // It stores the first registered target during traversal hit test targets.
  // Once triggered, route will be reset.
  SignalEventRoute signal_event_route_;
  // We need get value from map and judge if it's empty or not set.
  std::map<int, HitTestResult> hit_tests_;
  std::unordered_map<int, GestureRecognizerType> gesture_accepted_map_;
  const fml::RefPtr<fml::TaskRunner> task_runner_;

  FloatPoint down_event_point_;
  int down_event_pointer_id_ = 0;
  bool can_consume_slide_event_ = false;
  ConsumeSlideEventStatus consume_slide_event_status_ =
      ConsumeSlideEventStatus::kUndefined;
  HitTestResponsiveResult hit_test_responsive_result_;
  Puppet<Owner::kUI, GestureMediateService> gesture_mediate_puppet_;
  MouseWheelPhaseHandler mouse_wheel_phase_handler_;

  FRIEND_TEST(ScrollViewTest, NestedScrollGestureOnPC);
};

}  // namespace clay

#endif  // CLAY_UI_GESTURE_GESTURE_MANAGER_H_
