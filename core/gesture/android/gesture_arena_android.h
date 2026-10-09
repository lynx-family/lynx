// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_GESTURE_ANDROID_GESTURE_ARENA_ANDROID_H_
#define CORE_GESTURE_ANDROID_GESTURE_ARENA_ANDROID_H_

#include <jni.h>

#include <vector>

#include "base/include/platform/android/scoped_java_ref.h"
#include "core/gesture/gesture_arena.h"
#include "core/gesture/gesture_arena_delegate.h"

namespace lynx::tasm::gesture {

class GestureArenaAndroid final : public GestureArenaDelegate {
 public:
  GestureArenaAndroid(JNIEnv* env, jobject owner);
  ~GestureArenaAndroid() override = default;

  void ReplaceMemberGestures(MemberId member_id,
                             std::vector<GestureDefinition> definitions);
  void RemoveMember(MemberId member_id);
  void HandleInput(const InputEvent& event,
                   const std::vector<MemberId>& response_chain);
  void HandleTimer(TimerToken token, double monotonic_time_ms,
                   int64_t epoch_time_ms);
  void SetGestureState(MemberId member_id, uint32_t gesture_id,
                       GestureStateCommand command);
  void ResetAndDisconnect(JNIEnv* env);

  bool IsMemberValid(MemberId member_id) const override;
  Point ConvertPageToMember(MemberId member_id,
                            const Point& page_point) const override;
  ScrollState GetScrollState(MemberId member_id) const override;
  GestureDirection GetScrollDirection(MemberId member_id) const override;
  bool CanConsumeGesture(MemberId member_id, GestureDirection direction,
                         const Point& delta) const override;
  bool ShouldConsumeGesture(MemberId member_id) const override;
  ScrollResult ScrollBy(MemberId member_id, const Point& delta) override;
  void OnGestureRecognized(MemberId member_id) override;
  void OnGestureStateChanged(MemberId member_id, uint32_t gesture_id,
                             GestureState state) override;
  void DispatchGestureEvent(const GestureEvent& event) override;
  void ScheduleTimer(TimerToken token, double delay_ms) override;
  void CancelTimer(TimerToken token) override;
  bool StartFling(const Point& velocity) override;
  void StopFling() override;

 private:
  base::android::ScopedWeakGlobalJavaRef<jobject> owner_;
  GestureArena arena_;
};

}  // namespace lynx::tasm::gesture

#endif  // CORE_GESTURE_ANDROID_GESTURE_ARENA_ANDROID_H_
