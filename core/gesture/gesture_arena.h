// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_GESTURE_GESTURE_ARENA_H_
#define CORE_GESTURE_GESTURE_ARENA_H_

#include <memory>
#include <vector>

#include "core/gesture/gesture_arena_delegate.h"
#include "core/gesture/gesture_config.h"

namespace lynx::tasm::gesture {

class GestureArenaImpl;

class GestureArena {
 public:
  explicit GestureArena(GestureArenaDelegate& delegate);
  ~GestureArena();

  GestureArena(const GestureArena&) = delete;
  GestureArena& operator=(const GestureArena&) = delete;

  void ReplaceMemberGestures(
      MemberId member_id,
      const std::vector<const GestureDetector*>& gesture_detectors);
  void ReplaceMemberGestures(MemberId member_id,
                             std::vector<GestureDefinition> gestures);
  void RemoveMember(MemberId member_id);
  bool ContainsMember(MemberId member_id) const;
  bool ContainsGesture(MemberId member_id, uint32_t gesture_id) const;

  void HandleInput(const InputEvent& event,
                   const std::vector<MemberId>& response_chain = {});
  void HandleTimer(TimerToken token, double monotonic_time_ms,
                   int64_t epoch_time_ms);
  void SetGestureState(MemberId member_id, uint32_t gesture_id,
                       GestureStateCommand command);
  void Reset();

 private:
  std::unique_ptr<GestureArenaImpl> impl_;
};

}  // namespace lynx::tasm::gesture

#endif  // CORE_GESTURE_GESTURE_ARENA_H_
