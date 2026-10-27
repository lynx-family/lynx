// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_UNIFIED_GESTURE_HANDLER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_UNIFIED_GESTURE_HANDLER_H_

#include <arkui/ui_input_event.h>

#include <map>
#include <memory>
#include <optional>
#include <vector>

#include "base/include/fml/time/timer.h"
#include "core/gesture/gesture_arena.h"
#include "core/renderer/utils/base/base_def.h"

namespace lynx::tasm::harmony {
class LynxContext;
class UIBase;
class EventTarget;
class GestureArenaMember;
class FlingScroller;

class UnifiedGestureHandler final
    : public gesture::GestureArenaDelegate,
      public std::enable_shared_from_this<UnifiedGestureHandler> {
 public:
  explicit UnifiedGestureHandler(LynxContext* context);
  ~UnifiedGestureHandler() override;
  int ReplaceMember(std::weak_ptr<GestureArenaMember> member);
  void RemoveMember(int member_id);
  bool ContainsMember(int member_id) const;
  bool ContainsGesture(int member_id, uint32_t gesture_id) const;
  void SetGestureState(int member_id, uint32_t gesture_id, int state);
  void SetResponseChain(std::weak_ptr<EventTarget> target);
  void HandleInput(const ArkUI_UIInputEvent* event);
  void SetVelocity(float x, float y);
  void CancelInteraction();
  void Reset();

 private:
  bool IsMemberValid(gesture::MemberId member_id) const override;
  gesture::Point ConvertPageToMember(gesture::MemberId member_id,
                                     const gesture::Point& page) const override;
  gesture::ScrollState GetScrollState(
      gesture::MemberId member_id) const override;
  gesture::GestureDirection GetScrollDirection(
      gesture::MemberId member_id) const override;
  bool CanConsumeGesture(gesture::MemberId member_id,
                         gesture::GestureDirection direction,
                         const gesture::Point& delta) const override;
  bool ShouldConsumeGesture(gesture::MemberId member_id) const override;
  gesture::ScrollResult ScrollBy(gesture::MemberId member_id,
                                 const gesture::Point& delta) override;
  void OnGestureRecognized(gesture::MemberId member_id) override;
  void OnGestureStateChanged(gesture::MemberId member_id, uint32_t gesture_id,
                             gesture::GestureState state) override;
  void DispatchGestureEvent(const gesture::GestureEvent& event) override;
  void ScheduleTimer(gesture::TimerToken token, double delay_ms) override;
  void CancelTimer(gesture::TimerToken token) override;
  bool StartFling(const gesture::Point& velocity) override;
  void StopFling() override;

  std::shared_ptr<UIBase> FindMember(gesture::MemberId member_id) const;

  LynxContext* context_;
  std::map<gesture::MemberId, std::weak_ptr<UIBase>> members_;
  std::vector<gesture::MemberId> response_chain_;
  std::optional<int32_t> active_pointer_;
  gesture::InputEvent last_input_;
  gesture::Point velocity_;
  double native_to_monotonic_ms_ = 0;
  double monotonic_to_epoch_ms_ = 0;
  std::map<gesture::TimerToken, std::unique_ptr<fml::OneshotTimer>> timers_;
  std::unique_ptr<FlingScroller> fling_scroller_;
  bool stopping_fling_ = false;
  gesture::GestureArena arena_;
};

}  // namespace lynx::tasm::harmony
#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_GESTURE_UNIFIED_GESTURE_HANDLER_H_
