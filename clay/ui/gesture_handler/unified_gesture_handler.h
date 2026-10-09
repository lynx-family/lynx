// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_HANDLER_UNIFIED_GESTURE_HANDLER_H_
#define CLAY_UI_GESTURE_HANDLER_UNIFIED_GESTURE_HANDLER_H_

#include <cstddef>
#include <map>
#include <memory>
#include <optional>

#include "base/include/fml/memory/weak_ptr.h"
#include "base/include/fml/time/timer.h"
#include "clay/ui/gesture/hit_test.h"
#include "clay/ui/gesture/velocity_tracker.h"
#include "clay/ui/gesture_handler/gesture_detector.h"
#include "core/gesture/gesture_arena.h"

namespace clay {

namespace unified_gesture = lynx::tasm::gesture;
class BaseView;
class FlingScroller;
class PageView;

class UnifiedGestureHandler final
    : public unified_gesture::GestureArenaDelegate,
      public std::enable_shared_from_this<UnifiedGestureHandler> {
 public:
  explicit UnifiedGestureHandler(PageView* page_view);
  ~UnifiedGestureHandler() override;

  int ReplaceMember(BaseView* view, const GestureMap& detectors);
  void RemoveMember(int member_id);
  void SetGestureState(int member_id, uint32_t gesture_id, int state);
  bool ContainsGesture(int member_id, uint32_t gesture_id) const;
  void HandlePointerEvent(const PointerEvent& event, const HitTestResult& hits);
  void SetVelocity(FloatPoint velocity);
  void Reset();

 private:
  class EntryScope {
   public:
    explicit EntryScope(UnifiedGestureHandler& owner);
    ~EntryScope();

   private:
    UnifiedGestureHandler& owner_;
  };

  bool IsMemberValid(unified_gesture::MemberId member_id) const override;
  unified_gesture::Point ConvertPageToMember(
      unified_gesture::MemberId member_id,
      const unified_gesture::Point& page) const override;
  unified_gesture::ScrollState GetScrollState(
      unified_gesture::MemberId member_id) const override;
  unified_gesture::GestureDirection GetScrollDirection(
      unified_gesture::MemberId member_id) const override;
  bool CanConsumeGesture(unified_gesture::MemberId member_id,
                         unified_gesture::GestureDirection direction,
                         const unified_gesture::Point& delta) const override;
  bool ShouldConsumeGesture(unified_gesture::MemberId member_id) const override;
  unified_gesture::ScrollResult ScrollBy(
      unified_gesture::MemberId member_id,
      const unified_gesture::Point& delta) override;
  void OnGestureRecognized(unified_gesture::MemberId member_id) override;
  void OnGestureStateChanged(unified_gesture::MemberId member_id,
                             uint32_t gesture_id,
                             unified_gesture::GestureState state) override;
  void DispatchGestureEvent(
      const unified_gesture::GestureEvent& event) override;
  void ScheduleTimer(unified_gesture::TimerToken token,
                     double delay_ms) override;
  void CancelTimer(unified_gesture::TimerToken token) override;
  bool StartFling(const unified_gesture::Point& velocity) override;
  void StopFling() override;

  BaseView* FindMember(unified_gesture::MemberId member_id) const;
  unified_gesture::Point ToLogical(float x, float y) const;
  void LeaveEntry();
  void ResetNow();

  PageView* page_view_;
  std::map<unified_gesture::MemberId, fml::WeakPtr<BaseView>> members_;
  int next_member_id_ = 1;
  VelocityTracker velocity_tracker_;
  std::optional<int64_t> active_pointer_;
  unified_gesture::InputEvent last_input_;
  double native_to_monotonic_ms_ = 0;
  double monotonic_to_epoch_ms_ = 0;
  std::map<unified_gesture::TimerToken, std::unique_ptr<fml::OneshotTimer>>
      timers_;
  std::unique_ptr<FlingScroller> fling_scroller_;
  bool stopping_fling_ = false;
  size_t entry_depth_ = 0;
  bool reset_requested_ = false;
  bool reset_complete_ = false;
  unified_gesture::GestureArena arena_;
};

}  // namespace clay

#endif  // CLAY_UI_GESTURE_HANDLER_UNIFIED_GESTURE_HANDLER_H_
