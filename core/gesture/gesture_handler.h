// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_GESTURE_GESTURE_HANDLER_H_
#define CORE_GESTURE_GESTURE_HANDLER_H_

#include <memory>
#include <vector>

#include "core/gesture/gesture_types.h"

namespace lynx::tasm::gesture {

struct GestureInteractionContext {
  GestureDirection direction = GestureDirection::kUndetermined;
  bool has_simultaneous_delta = false;
  Point simultaneous_delta;
};

class GestureArenaImpl;

class GestureHandler {
 public:
  GestureHandler(MemberId member_id, GestureDefinition definition,
                 GestureArenaImpl& host);
  virtual ~GestureHandler();

  GestureHandler(const GestureHandler&) = delete;
  GestureHandler& operator=(const GestureHandler&) = delete;

  virtual void Handle(const InputEvent& event,
                      GestureInteractionContext& context) = 0;
  virtual void HandleSimultaneous(const InputEvent& event, const Point& delta,
                                  GestureInteractionContext& context);
  virtual void HandleTimer(TimerToken token, const InputEvent& event);

  void DispatchTouchCallback(GestureCallbackType callback,
                             const InputEvent& event);
  void Reset();
  void FailFromCompetition();
  void EndFromCommand();
  void CancelForTermination(const InputEvent& event);

  MemberId member_id() const { return key_.member_id; }
  uint32_t gesture_id() const { return key_.gesture_id; }
  GestureType gesture_type() const { return definition_.gesture_type; }
  GestureState state() const { return state_; }
  const GestureRelations& relations() const { return definition_.relations; }
  bool IsSubscribed(GestureCallbackType callback) const;

 protected:
  void Begin(const InputEvent& event);
  void Activate(const InputEvent& event);
  void Update(const InputEvent& event, const Point& delta = {});
  void Fail(const InputEvent& event);
  void End(const InputEvent& event);
  void Cancel(const InputEvent& event);
  TimerToken ScheduleTimer(double delay_ms);
  void CancelTimer(TimerToken& token);
  void TimerFired(TimerToken token);
  bool IsTerminal() const;
  void CancelAllTimers();

  const HandlerKey key_;
  const GestureDefinition definition_;
  GestureArenaImpl& host_;
  GestureState state_ = GestureState::kInit;
  InputEvent last_event_;
  std::vector<TimerToken> timers_;

 private:
  void TransitionTo(GestureState state);
  void Finish(const InputEvent& event, GestureState state);

  bool began_ = false;
  bool started_ = false;
  bool ended_ = false;
};

std::unique_ptr<GestureHandler> CreateGestureHandler(
    MemberId member_id, GestureDefinition definition, GestureArenaImpl& host);

}  // namespace lynx::tasm::gesture

#endif  // CORE_GESTURE_GESTURE_HANDLER_H_
