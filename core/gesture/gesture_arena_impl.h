// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_GESTURE_GESTURE_ARENA_IMPL_H_
#define CORE_GESTURE_GESTURE_ARENA_IMPL_H_

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "core/gesture/gesture_arena.h"

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
                 GestureArenaImpl &host);
  virtual ~GestureHandler();

  GestureHandler(const GestureHandler &) = delete;
  GestureHandler &operator=(const GestureHandler &) = delete;

  virtual void Handle(const InputEvent &event,
                      GestureInteractionContext &context) = 0;
  virtual void HandleSimultaneous(const InputEvent &event, const Point &delta,
                                  GestureInteractionContext &context);
  virtual void HandleTimer(TimerToken token, const InputEvent &event);

  void DispatchTouchCallback(GestureCallbackType callback,
                             const InputEvent &event);
  void Reset();
  void FailFromCompetition();
  void EndFromCommand();
  void CancelForTermination(const InputEvent &event);

  MemberId member_id() const { return key_.member_id; }
  uint32_t gesture_id() const { return key_.gesture_id; }
  GestureType gesture_type() const { return definition_.gesture_type; }
  GestureState state() const { return state_; }
  const GestureRelations &relations() const { return definition_.relations; }
  bool IsSubscribed(GestureCallbackType callback) const;

 protected:
  void Begin(const InputEvent &event);
  void Activate(const InputEvent &event);
  void Update(const InputEvent &event, const Point &delta = {});
  void Fail(const InputEvent &event);
  void End(const InputEvent &event);
  void Cancel(const InputEvent &event);
  TimerToken ScheduleTimer(double delay_ms);
  void CancelTimer(TimerToken &token);
  void TimerFired(TimerToken token);
  bool IsTerminal() const;
  void CancelAllTimers();

  const HandlerKey key_;
  const GestureDefinition definition_;
  GestureArenaImpl &host_;
  GestureState state_ = GestureState::kInit;
  InputEvent last_event_;
  std::vector<TimerToken> timers_;

 private:
  void TransitionTo(GestureState state);
  void Finish(const InputEvent &event, GestureState state);

  bool began_ = false;
  bool started_ = false;
  bool ended_ = false;
};

std::unique_ptr<GestureHandler> CreateGestureHandler(
    MemberId member_id, GestureDefinition definition, GestureArenaImpl &host);

class GestureArenaImpl {
 public:
  explicit GestureArenaImpl(GestureArenaDelegate &delegate)
      : delegate_(delegate) {}

  ~GestureArenaImpl() { Reset(); }

  void ReplaceMemberGestures(MemberId member_id,
                             std::vector<GestureDefinition> definitions);

  void RemoveMember(MemberId member_id);

  bool ContainsMember(MemberId member_id) const;

  bool ContainsGesture(MemberId member_id, uint32_t gesture_id) const;

  void HandleInput(const InputEvent &input,
                   const std::vector<MemberId> &response_chain);

  void HandleTimer(TimerToken token, double monotonic_time_ms,
                   int64_t epoch_time_ms);

  void SetGestureState(MemberId member_id, uint32_t gesture_id,
                       GestureStateCommand command);

  void Reset();

 private:
  void ApplyReset();
  struct MemberRecord {
    uint64_t generation = 0;
    std::vector<GestureDefinition> definitions;
    std::map<uint32_t, std::unique_ptr<GestureHandler>> handlers;
  };

  struct TimerRecord {
    HandlerKey key;
    uint64_t generation = 0;
    uint64_t sequence_id = 0;
    InputEvent event;
  };

  struct PendingMutation {
    std::optional<std::vector<GestureDefinition>> definitions;
  };

  struct AggregateState {
    GestureState state = GestureState::kFail;
    std::optional<uint32_t> active_gesture_id;
  };

  void QueueMutation(MemberId member_id,
                     std::optional<std::vector<GestureDefinition>> definitions);

  void FlushMutations();

  void ApplyReplacement(MemberId member_id,
                        std::vector<GestureDefinition> definitions);

  void ApplyRemoval(MemberId member_id);

  void BeginSequence(const InputEvent &event,
                     const std::vector<MemberId> &response_chain);

  void ActivateMember(MemberId member_id);

  std::vector<MemberId> ConvertResponseChain(
      const std::vector<MemberId> &response_chain) const;

  void DispatchTouches(const InputEvent &event);

  void DispatchWinner(const InputEvent &event);

  void ReevaluateCompetition();

  AggregateState Aggregate(MemberId member_id) const;

  void ApplySameMemberRace(MemberId member_id, uint32_t active_gesture_id);

  std::unordered_set<uint32_t> SameMemberSimultaneousIds(
      MemberId member_id) const;

  void UpdateSimultaneousMembers();

  std::optional<size_t> FindNextWinner(std::optional<size_t> current_index);

  void StartOrFinishFling(const InputEvent &event);

  void TerminateInteraction(const InputEvent &event, GestureState state);

  void TerminateMember(MemberId member_id, const InputEvent &event,
                       GestureState state);

  std::vector<HandlerKey> HandlerKeys(MemberId member_id) const;

  GestureHandler *FindHandler(const HandlerKey &key);

  void CallHandler(const HandlerKey &key,
                   const std::function<void(GestureHandler &)> &callback);

  bool IsCurrentGeneration(MemberId member_id, uint64_t generation) const;

  bool IsActiveMember(MemberId member_id) const;

  std::optional<MemberId> CurrentWinner() const;

  std::vector<MemberId> RegisteredMemberIds() const;

  std::vector<MemberId> ActiveMemberIds() const;

 public:
  bool IsMemberValid(MemberId member_id) const;

  GestureDirection GetScrollDirection(MemberId member_id) const;

  bool CanConsumeGesture(MemberId member_id, GestureDirection direction,
                         const Point &delta) const;

  bool ShouldConsumeGesture(MemberId member_id) const;

  ScrollResult ScrollBy(MemberId member_id, const Point &delta);

  void OnGestureRecognized(MemberId member_id);

  void OnGestureStateChanged(const HandlerKey &key, GestureState state);

  void Emit(const HandlerKey &key, GestureCallbackType callback,
            const InputEvent &event, const Point &delta);

  TimerToken ScheduleTimer(const HandlerKey &key, double delay_ms);

  void CancelTimer(TimerToken token);

 private:
  GestureArenaDelegate &delegate_;
  std::map<MemberId, MemberRecord> members_;
  std::unordered_map<uint32_t, std::set<MemberId>> gesture_members_;
  std::vector<MemberId> response_chain_;
  std::vector<MemberId> compete_chain_;
  std::set<MemberId> simultaneous_members_;
  std::unordered_map<MemberId, uint64_t> active_generations_;
  std::optional<size_t> winner_index_;
  GestureInteractionContext context_;
  std::map<TimerToken, TimerRecord> timers_;
  std::map<MemberId, PendingMutation> pending_mutations_;
  std::unordered_set<MemberId> invalidated_members_;
  InputEvent last_input_;
  uint64_t next_generation_ = 1;
  TimerToken next_timer_token_ = 1;
  uint64_t sequence_id_ = 0;
  size_t dispatch_depth_ = 0;
  bool has_sequence_ = false;
  bool interaction_terminated_ = true;
  bool fling_active_ = false;
  bool flushing_mutations_ = false;
  bool resetting_ = false;
  bool reset_pending_ = false;
};

}  // namespace lynx::tasm::gesture

#endif  // CORE_GESTURE_GESTURE_ARENA_IMPL_H_
