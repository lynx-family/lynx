// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_GESTURE_GESTURE_ARENA_DELEGATE_H_
#define CORE_GESTURE_GESTURE_ARENA_DELEGATE_H_

#include "core/gesture/gesture_types.h"

namespace lynx::tasm::gesture {

class GestureArenaDelegate {
 public:
  virtual ~GestureArenaDelegate() = default;

  virtual bool IsMemberValid(MemberId member_id) const = 0;
  virtual Point ConvertPageToMember(MemberId member_id,
                                    const Point& page_point) const = 0;
  virtual ScrollState GetScrollState(MemberId member_id) const = 0;
  virtual GestureDirection GetScrollDirection(MemberId member_id) const = 0;
  virtual bool CanConsumeGesture(MemberId member_id, GestureDirection direction,
                                 const Point& delta) const = 0;
  virtual bool ShouldConsumeGesture(MemberId member_id) const = 0;
  virtual ScrollResult ScrollBy(MemberId member_id, const Point& delta) = 0;
  virtual void OnGestureRecognized(MemberId member_id) = 0;
  virtual void OnGestureStateChanged(MemberId member_id, uint32_t gesture_id,
                                     GestureState state) = 0;
  virtual void DispatchGestureEvent(const GestureEvent& event) = 0;

  virtual void ScheduleTimer(TimerToken token, double delay_ms) = 0;
  virtual void CancelTimer(TimerToken token) = 0;
  // A successful driver emits content-direction kFlingFrame deltas with the
  // same sequence id until fling_finished is true.
  virtual bool StartFling(const Point& velocity) = 0;
  virtual void StopFling() = 0;
};

}  // namespace lynx::tasm::gesture

#endif  // CORE_GESTURE_GESTURE_ARENA_DELEGATE_H_
