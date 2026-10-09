// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/gesture/gesture_arena.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "core/gesture/gesture_handler.h"

namespace lynx::tasm::gesture {
namespace {

bool ConfigEquals(const GestureConfig& left, const GestureConfig& right) {
  return left.min_distance == right.min_distance &&
         left.max_distance == right.max_distance &&
         left.min_duration_ms == right.min_duration_ms &&
         left.max_duration_ms == right.max_duration_ms &&
         left.tap_slop == right.tap_slop;
}

bool RelationsEqual(const GestureRelations& left,
                    const GestureRelations& right) {
  return left.simultaneous == right.simultaneous &&
         left.wait_for == right.wait_for &&
         left.continue_with == right.continue_with;
}

bool DefinitionEquals(const GestureDefinition& left,
                      const GestureDefinition& right) {
  return left.gesture_id == right.gesture_id &&
         left.gesture_type == right.gesture_type &&
         ConfigEquals(left.config, right.config) &&
         RelationsEqual(left.relations, right.relations) &&
         left.callbacks == right.callbacks;
}

bool DefinitionsEqual(const std::vector<GestureDefinition>& left,
                      const std::vector<GestureDefinition>& right) {
  return left.size() == right.size() &&
         std::equal(left.begin(), left.end(), right.begin(), DefinitionEquals);
}

std::optional<GestureCallbackType> TouchCallback(InputType type) {
  switch (type) {
    case InputType::kDown:
      return GestureCallbackType::kTouchesDown;
    case InputType::kMove:
      return GestureCallbackType::kTouchesMove;
    case InputType::kUp:
      return GestureCallbackType::kTouchesUp;
    case InputType::kCancel:
      return GestureCallbackType::kTouchesCancel;
    case InputType::kFlingFrame:
      return std::nullopt;
  }
}

}  // namespace

class GestureArenaImpl {
 public:
  explicit GestureArenaImpl(GestureArenaDelegate& delegate)
      : delegate_(delegate) {}

  ~GestureArenaImpl() { Reset(); }

  void ReplaceMemberGestures(MemberId member_id,
                             std::vector<GestureDefinition> definitions) {
    QueueMutation(member_id, std::move(definitions));
    FlushMutations();
  }

  void RemoveMember(MemberId member_id) {
    QueueMutation(member_id, std::nullopt);
    FlushMutations();
  }

  bool ContainsMember(MemberId member_id) const {
    return members_.find(member_id) != members_.end() &&
           invalidated_members_.find(member_id) == invalidated_members_.end();
  }

  bool ContainsGesture(MemberId member_id, uint32_t gesture_id) const {
    if (!ContainsMember(member_id)) {
      return false;
    }
    const auto member = members_.find(member_id);
    return member != members_.end() &&
           member->second.handlers.find(gesture_id) !=
               member->second.handlers.end();
  }

  void HandleInput(const InputEvent& input,
                   const std::vector<MemberId>& response_chain) {
    if (resetting_ || reset_pending_) {
      return;
    }
    if (input.type == InputType::kDown) {
      if (has_sequence_ && !interaction_terminated_) {
        InputEvent cancelled = last_input_;
        cancelled.type = InputType::kCancel;
        cancelled.monotonic_time_ms = input.monotonic_time_ms;
        cancelled.epoch_time_ms = input.epoch_time_ms;
        TerminateInteraction(cancelled, GestureState::kCancel);
      }
      BeginSequence(input, response_chain);
    } else if (!has_sequence_ || input.sequence_id != sequence_id_) {
      return;
    }

    if (input.type != InputType::kDown && interaction_terminated_) {
      return;
    }

    InputEvent event = input;
    last_input_ = event;
    if (event.type != InputType::kFlingFrame) {
      DispatchTouches(event);
    }
    if (interaction_terminated_) {
      return;
    }

    ReevaluateCompetition();
    if (event.type == InputType::kCancel) {
      DispatchWinner(event);
      TerminateInteraction(event, GestureState::kCancel);
      return;
    }

    DispatchWinner(event);
    if (event.type == InputType::kUp) {
      StartOrFinishFling(event);
    } else if (event.type == InputType::kFlingFrame && event.fling_finished) {
      fling_active_ = false;
      TerminateInteraction(event, GestureState::kEnd);
    }
  }

  void HandleTimer(TimerToken token, double monotonic_time_ms,
                   int64_t epoch_time_ms) {
    if (resetting_ || reset_pending_) {
      return;
    }
    const auto timer_it = timers_.find(token);
    if (timer_it == timers_.end()) {
      return;
    }
    const TimerRecord timer = timer_it->second;
    timers_.erase(timer_it);
    if (interaction_terminated_ || timer.sequence_id != sequence_id_ ||
        !IsCurrentGeneration(timer.key.member_id, timer.generation)) {
      return;
    }
    auto* handler = FindHandler(timer.key);
    if (!handler) {
      return;
    }

    InputEvent event = timer.event;
    event.monotonic_time_ms = monotonic_time_ms;
    event.epoch_time_ms = epoch_time_ms;
    CallHandler(timer.key, [token, &event](GestureHandler& current) {
      current.HandleTimer(token, event);
    });
    ReevaluateCompetition();
  }

  void SetGestureState(MemberId member_id, uint32_t gesture_id,
                       GestureStateCommand command) {
    if (resetting_ || reset_pending_ ||
        command == GestureStateCommand::kActive ||
        invalidated_members_.find(member_id) != invalidated_members_.end()) {
      return;
    }
    const HandlerKey key{member_id, gesture_id};
    if (!FindHandler(key)) {
      return;
    }
    CallHandler(key, [command](GestureHandler& handler) {
      if (command == GestureStateCommand::kFail) {
        handler.FailFromCompetition();
      } else if (command == GestureStateCommand::kEnd) {
        handler.EndFromCommand();
      }
    });
    ReevaluateCompetition();
  }

  void Reset() {
    if (resetting_) {
      return;
    }
    if (dispatch_depth_ != 0 || flushing_mutations_) {
      reset_pending_ = true;
      interaction_terminated_ = true;
      for (const auto& [member_id, member] : members_) {
        invalidated_members_.insert(member_id);
      }
      return;
    }
    ApplyReset();
  }

 private:
  void ApplyReset() {
    resetting_ = true;
    reset_pending_ = false;
    interaction_terminated_ = true;
    invalidated_members_.clear();
    InputEvent event = last_input_;
    event.type = InputType::kCancel;
    for (const auto member_id : RegisteredMemberIds()) {
      TerminateMember(member_id, event, GestureState::kCancel);
    }
    for (const auto& [token, timer] : timers_) {
      delegate_.CancelTimer(token);
    }
    timers_.clear();
    if (fling_active_) {
      fling_active_ = false;
      delegate_.StopFling();
    }
    members_.clear();
    gesture_members_.clear();
    response_chain_.clear();
    compete_chain_.clear();
    simultaneous_members_.clear();
    active_generations_.clear();
    pending_mutations_.clear();
    invalidated_members_.clear();
    winner_index_.reset();
    has_sequence_ = false;
    interaction_terminated_ = true;
    resetting_ = false;
  }
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

  void QueueMutation(
      MemberId member_id,
      std::optional<std::vector<GestureDefinition>> definitions) {
    if (resetting_ || reset_pending_) {
      return;
    }
    if (definitions) {
      definitions = NormalizeGestureDefinitions(std::move(*definitions));
      if (definitions->empty()) {
        definitions.reset();
      }
    }
    const auto pending = pending_mutations_.find(member_id);
    if (pending != pending_mutations_.end()) {
      const bool same_pending =
          (!definitions && !pending->second.definitions) ||
          (definitions && pending->second.definitions &&
           DefinitionsEqual(*definitions, *pending->second.definitions));
      if (same_pending) {
        return;
      }
      const auto current = members_.find(member_id);
      const bool same_current =
          (!definitions && current == members_.end()) ||
          (definitions && current != members_.end() &&
           DefinitionsEqual(*definitions, current->second.definitions));
      if (same_current) {
        pending_mutations_.erase(pending);
        invalidated_members_.erase(member_id);
        return;
      }
    } else {
      const auto current = members_.find(member_id);
      const bool same_current =
          (!definitions && current == members_.end()) ||
          (definitions && current != members_.end() &&
           DefinitionsEqual(*definitions, current->second.definitions));
      if (same_current) {
        return;
      }
    }
    pending_mutations_.insert_or_assign(
        member_id, PendingMutation{std::move(definitions)});
    invalidated_members_.insert(member_id);
  }

  void FlushMutations() {
    if (dispatch_depth_ != 0 || flushing_mutations_ || resetting_) {
      return;
    }
    if (reset_pending_) {
      ApplyReset();
      return;
    }
    flushing_mutations_ = true;
    while (!pending_mutations_.empty() && !reset_pending_) {
      auto pending = std::move(pending_mutations_);
      pending_mutations_.clear();
      for (auto& [member_id, mutation] : pending) {
        invalidated_members_.erase(member_id);
        if (mutation.definitions && !mutation.definitions->empty()) {
          ApplyReplacement(member_id, std::move(*mutation.definitions));
        } else {
          ApplyRemoval(member_id);
        }
        if (reset_pending_) {
          break;
        }
      }
    }
    flushing_mutations_ = false;
    if (reset_pending_) {
      ApplyReset();
    }
  }

  void ApplyReplacement(MemberId member_id,
                        std::vector<GestureDefinition> definitions) {
    const auto old = members_.find(member_id);
    if (old != members_.end() &&
        DefinitionsEqual(old->second.definitions, definitions)) {
      return;
    }

    ApplyRemoval(member_id);
    MemberRecord record;
    record.generation = next_generation_++;
    record.definitions = std::move(definitions);
    for (const auto& definition : record.definitions) {
      auto handler = CreateGestureHandler(member_id, definition, *this);
      if (handler) {
        record.handlers.insert_or_assign(definition.gesture_id,
                                         std::move(handler));
        gesture_members_[definition.gesture_id].insert(member_id);
      }
    }
    members_.insert_or_assign(member_id, std::move(record));
  }

  void ApplyRemoval(MemberId member_id) {
    const auto it = members_.find(member_id);
    if (it == members_.end()) {
      return;
    }
    const bool was_winner = CurrentWinner() == member_id;
    InputEvent event = last_input_;
    event.type = InputType::kCancel;
    TerminateMember(member_id, event, GestureState::kCancel);
    for (const auto& definition : it->second.definitions) {
      const auto registry = gesture_members_.find(definition.gesture_id);
      if (registry == gesture_members_.end()) {
        continue;
      }
      registry->second.erase(member_id);
      if (registry->second.empty()) {
        gesture_members_.erase(registry);
      }
    }
    members_.erase(member_id);
    active_generations_.erase(member_id);
    simultaneous_members_.erase(member_id);
    if (was_winner && fling_active_) {
      fling_active_ = false;
      delegate_.StopFling();
    }
  }

  void BeginSequence(const InputEvent& event,
                     const std::vector<MemberId>& response_chain) {
    if (fling_active_) {
      fling_active_ = false;
      delegate_.StopFling();
    }
    has_sequence_ = true;
    interaction_terminated_ = false;
    sequence_id_ = event.sequence_id;
    last_input_ = event;
    context_ = {};
    winner_index_.reset();
    simultaneous_members_.clear();
    active_generations_.clear();
    response_chain_.clear();
    for (const auto member_id : response_chain) {
      if (ContainsMember(member_id) && delegate_.IsMemberValid(member_id)) {
        response_chain_.push_back(member_id);
      }
    }
    compete_chain_ = ConvertResponseChain(response_chain_);
    for (const auto member_id : response_chain_) {
      ActivateMember(member_id);
    }
    for (const auto member_id : compete_chain_) {
      ActivateMember(member_id);
    }
    winner_index_ = FindNextWinner(std::nullopt);
    UpdateSimultaneousMembers();
  }

  void ActivateMember(MemberId member_id) {
    const auto member = members_.find(member_id);
    if (member == members_.end() ||
        active_generations_.find(member_id) != active_generations_.end()) {
      return;
    }
    active_generations_[member_id] = member->second.generation;
    std::vector<uint32_t> gesture_ids;
    for (const auto& [gesture_id, handler] : member->second.handlers) {
      gesture_ids.push_back(gesture_id);
    }
    for (const auto gesture_id : gesture_ids) {
      const HandlerKey key{member_id, gesture_id};
      CallHandler(key, [](GestureHandler& handler) { handler.Reset(); });
    }
  }

  std::vector<MemberId> ConvertResponseChain(
      const std::vector<MemberId>& response_chain) const {
    std::vector<MemberId> result;
    for (size_t current_index = 0; current_index < response_chain.size();
         ++current_index) {
      const auto member_id = response_chain[current_index];
      const auto member = members_.find(member_id);
      if (member == members_.end() || member->second.handlers.empty()) {
        continue;
      }

      const GestureRelations* selected_relations = nullptr;
      for (const auto& definition : member->second.definitions) {
        if (!definition.relations.wait_for.empty() ||
            !definition.relations.continue_with.empty()) {
          selected_relations = &definition.relations;
          break;
        }
      }
      if (!selected_relations) {
        result.push_back(member_id);
        continue;
      }

      if (!selected_relations->wait_for.empty()) {
        std::unordered_set<MemberId> wait_members;
        for (const auto gesture_id : selected_relations->wait_for) {
          const auto registry = gesture_members_.find(gesture_id);
          if (registry != gesture_members_.end()) {
            wait_members.insert(registry->second.begin(),
                                registry->second.end());
          }
        }
        bool found = false;
        for (size_t index = current_index + 1; index < response_chain.size();
             ++index) {
          if (wait_members.find(response_chain[index]) != wait_members.end()) {
            result.push_back(response_chain[index]);
            found = true;
          }
        }
        result.push_back(member_id);
        if (found) {
          break;
        }
        continue;
      }

      result.push_back(member_id);
      for (const auto gesture_id : selected_relations->continue_with) {
        const auto registry = gesture_members_.find(gesture_id);
        if (registry == gesture_members_.end()) {
          continue;
        }
        result.insert(result.end(), registry->second.begin(),
                      registry->second.end());
      }
      break;
    }
    return result;
  }

  void DispatchTouches(const InputEvent& event) {
    const auto callback = TouchCallback(event.type);
    if (!callback) {
      return;
    }
    const auto members = response_chain_;
    for (const auto member_id : members) {
      for (const auto& key : HandlerKeys(member_id)) {
        CallHandler(key, [callback, &event](GestureHandler& handler) {
          handler.DispatchTouchCallback(*callback, event);
        });
      }
    }
  }

  void DispatchWinner(const InputEvent& event) {
    const auto winner = CurrentWinner();
    if (!winner || !IsActiveMember(*winner)) {
      return;
    }
    context_.has_simultaneous_delta = false;
    context_.simultaneous_delta = {};
    for (const auto& key : HandlerKeys(*winner)) {
      CallHandler(key, [&event, this](GestureHandler& handler) {
        handler.Handle(event, context_);
      });
    }

    const Point simultaneous_delta =
        context_.has_simultaneous_delta ? context_.simultaneous_delta : Point{};
    const auto simultaneous = simultaneous_members_;
    for (const auto member_id : simultaneous) {
      if (!IsActiveMember(member_id)) {
        continue;
      }
      for (const auto& key : HandlerKeys(member_id)) {
        CallHandler(
            key, [&event, &simultaneous_delta, this](GestureHandler& handler) {
              handler.HandleSimultaneous(event, simultaneous_delta, context_);
            });
      }
    }
    context_.has_simultaneous_delta = false;
    context_.simultaneous_delta = {};
    ReevaluateCompetition();
  }

  void ReevaluateCompetition() {
    if (interaction_terminated_) {
      return;
    }
    const auto winner = CurrentWinner();
    if (!winner || !IsActiveMember(*winner)) {
      winner_index_ = FindNextWinner(winner_index_);
      UpdateSimultaneousMembers();
      return;
    }

    auto aggregate = Aggregate(*winner);
    if (aggregate.state == GestureState::kEnd ||
        aggregate.state == GestureState::kCancel) {
      TerminateInteraction(last_input_, aggregate.state);
      return;
    }
    if (aggregate.state == GestureState::kActive &&
        aggregate.active_gesture_id) {
      ApplySameMemberRace(*winner, *aggregate.active_gesture_id);
      aggregate = Aggregate(*winner);
      if (aggregate.state == GestureState::kEnd ||
          aggregate.state == GestureState::kCancel) {
        TerminateInteraction(last_input_, aggregate.state);
        return;
      }
    }
    if (aggregate.state == GestureState::kFail) {
      winner_index_ = FindNextWinner(winner_index_);
    }
    UpdateSimultaneousMembers();
  }

  AggregateState Aggregate(MemberId member_id) const {
    const auto member = members_.find(member_id);
    if (member == members_.end() || member->second.handlers.empty()) {
      return {};
    }
    bool has_begin = false;
    bool has_init = false;
    bool has_fail = false;
    bool has_dormant_fling = false;
    std::optional<uint32_t> active;
    for (const auto& [gesture_id, handler] : member->second.handlers) {
      switch (handler->state()) {
        case GestureState::kEnd:
          return {GestureState::kEnd, std::nullopt};
        case GestureState::kCancel:
          return {GestureState::kCancel, std::nullopt};
        case GestureState::kActive:
          if (!active || gesture_id < *active) {
            active = gesture_id;
          }
          break;
        case GestureState::kBegin:
          has_begin = true;
          break;
        case GestureState::kInit:
          if (handler->gesture_type() == GestureType::FLING) {
            has_dormant_fling = true;
          } else {
            has_init = true;
          }
          break;
        case GestureState::kFail:
          has_fail = true;
          break;
      }
    }
    if (active) {
      return {GestureState::kActive, active};
    }
    if (has_begin) {
      return {GestureState::kBegin, std::nullopt};
    }
    if (has_init) {
      return {GestureState::kInit, std::nullopt};
    }
    return {has_dormant_fling && !has_fail ? GestureState::kInit
                                           : GestureState::kFail,
            std::nullopt};
  }

  void ApplySameMemberRace(MemberId member_id, uint32_t active_gesture_id) {
    const auto simultaneous_ids = SameMemberSimultaneousIds(member_id);
    for (const auto& key : HandlerKeys(member_id)) {
      if (key.gesture_id != active_gesture_id &&
          simultaneous_ids.find(key.gesture_id) == simultaneous_ids.end()) {
        CallHandler(key, [](GestureHandler& handler) {
          handler.FailFromCompetition();
        });
      }
    }
  }

  std::unordered_set<uint32_t> SameMemberSimultaneousIds(
      MemberId member_id) const {
    std::unordered_set<uint32_t> handler_ids;
    std::unordered_set<uint32_t> simultaneous_ids;
    const auto member = members_.find(member_id);
    if (member == members_.end()) {
      return simultaneous_ids;
    }
    for (const auto& [gesture_id, handler] : member->second.handlers) {
      handler_ids.insert(gesture_id);
    }
    for (const auto& definition : member->second.definitions) {
      for (const auto gesture_id : definition.relations.simultaneous) {
        if (handler_ids.find(gesture_id) != handler_ids.end()) {
          simultaneous_ids.insert(gesture_id);
        }
      }
    }
    return simultaneous_ids;
  }

  void UpdateSimultaneousMembers() {
    simultaneous_members_.clear();
    const auto winner = CurrentWinner();
    if (!winner || !IsActiveMember(*winner)) {
      return;
    }
    const auto member = members_.find(*winner);
    if (member == members_.end()) {
      return;
    }
    for (const auto& definition : member->second.definitions) {
      for (const auto gesture_id : definition.relations.simultaneous) {
        const auto registry = gesture_members_.find(gesture_id);
        if (registry == gesture_members_.end()) {
          continue;
        }
        for (const auto member_id : registry->second) {
          if (member_id != *winner && ContainsMember(member_id) &&
              delegate_.IsMemberValid(member_id)) {
            ActivateMember(member_id);
            simultaneous_members_.insert(member_id);
          }
        }
      }
    }
  }

  std::optional<size_t> FindNextWinner(std::optional<size_t> current_index) {
    if (compete_chain_.empty()) {
      return std::nullopt;
    }
    const size_t start =
        current_index ? (*current_index + 1) % compete_chain_.size() : 0;
    const size_t count =
        current_index ? compete_chain_.size() - 1 : compete_chain_.size();
    for (size_t offset = 0; offset < count; ++offset) {
      const size_t index = (start + offset) % compete_chain_.size();
      const auto member_id = compete_chain_[index];
      if (!IsActiveMember(member_id) ||
          (current_index && member_id == compete_chain_[*current_index])) {
        continue;
      }
      auto aggregate = Aggregate(member_id);
      if (current_index && aggregate.state == GestureState::kFail) {
        for (const auto& key : HandlerKeys(member_id)) {
          CallHandler(key, [](GestureHandler& handler) { handler.Reset(); });
        }
        aggregate = Aggregate(member_id);
      }
      if (aggregate.state == GestureState::kInit ||
          aggregate.state == GestureState::kBegin ||
          aggregate.state == GestureState::kActive) {
        return index;
      }
    }
    return std::nullopt;
  }

  void StartOrFinishFling(const InputEvent& event) {
    if (interaction_terminated_) {
      return;
    }
    if (CurrentWinner()) {
      fling_active_ = true;
      ++dispatch_depth_;
      const bool started = delegate_.StartFling(event.velocity);
      --dispatch_depth_;
      FlushMutations();
      const auto winner = CurrentWinner();
      if (started && fling_active_ && winner && IsActiveMember(*winner)) {
        return;
      }
      if (started && fling_active_) {
        delegate_.StopFling();
      }
      fling_active_ = false;
      if (interaction_terminated_) {
        return;
      }
    }

    InputEvent finished = event;
    finished.type = InputType::kFlingFrame;
    finished.delta = {};
    finished.velocity = {};
    finished.fling_finished = true;
    DispatchWinner(finished);
    if (!interaction_terminated_) {
      TerminateInteraction(finished, GestureState::kEnd);
    }
  }

  void TerminateInteraction(const InputEvent& event, GestureState state) {
    if (interaction_terminated_) {
      return;
    }
    interaction_terminated_ = true;
    if (fling_active_) {
      fling_active_ = false;
      delegate_.StopFling();
    }
    const auto members = ActiveMemberIds();
    for (const auto member_id : members) {
      TerminateMember(member_id, event, state);
    }
    winner_index_.reset();
    simultaneous_members_.clear();
  }

  void TerminateMember(MemberId member_id, const InputEvent& event,
                       GestureState state) {
    for (const auto& key : HandlerKeys(member_id)) {
      CallHandler(key, [&event, state](GestureHandler& handler) {
        if (handler.state() != GestureState::kBegin &&
            handler.state() != GestureState::kActive) {
          return;
        }
        if (state == GestureState::kCancel) {
          handler.CancelForTermination(event);
        } else {
          handler.EndFromCommand();
        }
      });
    }
  }

  std::vector<HandlerKey> HandlerKeys(MemberId member_id) const {
    std::vector<HandlerKey> result;
    const auto member = members_.find(member_id);
    if (member == members_.end()) {
      return result;
    }
    result.reserve(member->second.handlers.size());
    for (const auto& [gesture_id, handler] : member->second.handlers) {
      result.push_back({member_id, gesture_id});
    }
    return result;
  }

  GestureHandler* FindHandler(const HandlerKey& key) {
    const auto member = members_.find(key.member_id);
    if (member == members_.end()) {
      return nullptr;
    }
    const auto handler = member->second.handlers.find(key.gesture_id);
    return handler == member->second.handlers.end() ? nullptr
                                                    : handler->second.get();
  }

  void CallHandler(const HandlerKey& key,
                   const std::function<void(GestureHandler&)>& callback) {
    auto* handler = FindHandler(key);
    if (!handler || invalidated_members_.find(key.member_id) !=
                        invalidated_members_.end()) {
      return;
    }
    ++dispatch_depth_;
    callback(*handler);
    --dispatch_depth_;
    FlushMutations();
  }

  bool IsCurrentGeneration(MemberId member_id, uint64_t generation) const {
    const auto member = members_.find(member_id);
    return member != members_.end() &&
           member->second.generation == generation &&
           invalidated_members_.find(member_id) == invalidated_members_.end();
  }

  bool IsActiveMember(MemberId member_id) const {
    const auto generation = active_generations_.find(member_id);
    return generation != active_generations_.end() &&
           IsCurrentGeneration(member_id, generation->second) &&
           delegate_.IsMemberValid(member_id);
  }

  std::optional<MemberId> CurrentWinner() const {
    if (!winner_index_ || *winner_index_ >= compete_chain_.size()) {
      return std::nullopt;
    }
    return compete_chain_[*winner_index_];
  }

  std::vector<MemberId> RegisteredMemberIds() const {
    std::vector<MemberId> result;
    result.reserve(members_.size());
    for (const auto& [member_id, member] : members_) {
      result.push_back(member_id);
    }
    return result;
  }

  std::vector<MemberId> ActiveMemberIds() const {
    std::vector<MemberId> result;
    result.reserve(active_generations_.size());
    for (const auto& [member_id, generation] : active_generations_) {
      result.push_back(member_id);
    }
    return result;
  }

 public:
  bool IsMemberValid(MemberId member_id) const {
    return ContainsMember(member_id) && delegate_.IsMemberValid(member_id);
  }

  GestureDirection GetScrollDirection(MemberId member_id) const {
    return IsMemberValid(member_id) ? delegate_.GetScrollDirection(member_id)
                                    : GestureDirection::kUndetermined;
  }

  bool CanConsumeGesture(MemberId member_id, GestureDirection direction,
                         const Point& delta) const {
    return IsMemberValid(member_id) &&
           delegate_.CanConsumeGesture(member_id, direction, delta);
  }

  bool ShouldConsumeGesture(MemberId member_id) const {
    return IsMemberValid(member_id) &&
           delegate_.ShouldConsumeGesture(member_id);
  }

  ScrollResult ScrollBy(MemberId member_id, const Point& delta) {
    return IsMemberValid(member_id) ? delegate_.ScrollBy(member_id, delta)
                                    : ScrollResult{{}, delta};
  }

  void OnGestureRecognized(MemberId member_id) {
    if (IsMemberValid(member_id)) {
      delegate_.OnGestureRecognized(member_id);
    }
  }

  void OnGestureStateChanged(const HandlerKey& key, GestureState state) {
    delegate_.OnGestureStateChanged(key.member_id, key.gesture_id, state);
  }

  void Emit(const HandlerKey& key, GestureCallbackType callback,
            const InputEvent& event, const Point& delta) {
    auto* handler = FindHandler(key);
    if (!handler || !IsMemberValid(key.member_id)) {
      return;
    }
    GestureEvent gesture_event;
    gesture_event.member_id = key.member_id;
    gesture_event.gesture_id = key.gesture_id;
    gesture_event.gesture_type = handler->gesture_type();
    gesture_event.callback = callback;
    gesture_event.source = event.type;
    gesture_event.sequence_id = event.sequence_id;
    gesture_event.pointer_id = event.pointer_id;
    gesture_event.timestamp_epoch_ms = event.epoch_time_ms;
    gesture_event.screen = event.screen;
    gesture_event.page = event.page;
    gesture_event.client = event.client;
    gesture_event.local =
        delegate_.ConvertPageToMember(key.member_id, event.page);
    gesture_event.delta = delta;
    gesture_event.scroll = delegate_.GetScrollState(key.member_id);
    if (IsMemberValid(key.member_id)) {
      delegate_.DispatchGestureEvent(gesture_event);
    }
  }

  TimerToken ScheduleTimer(const HandlerKey& key, double delay_ms) {
    const auto member = members_.find(key.member_id);
    if (member == members_.end() || !IsMemberValid(key.member_id)) {
      return 0;
    }
    const auto token = next_timer_token_++;
    timers_.insert_or_assign(token, TimerRecord{key, member->second.generation,
                                                sequence_id_, last_input_});
    delegate_.ScheduleTimer(token, std::max(0.0, delay_ms));
    return token;
  }

  void CancelTimer(TimerToken token) {
    if (token == 0) {
      return;
    }
    const auto timer = timers_.find(token);
    if (timer == timers_.end()) {
      return;
    }
    timers_.erase(timer);
    delegate_.CancelTimer(token);
  }

 private:
  GestureArenaDelegate& delegate_;
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

namespace {

class PanGestureHandler : public GestureHandler {
 public:
  using GestureHandler::GestureHandler;

  void Handle(const InputEvent& event,
              GestureInteractionContext& context) override {
    last_event_ = event;
    if (state_ == GestureState::kFail || IsTerminal()) {
      return;
    }
    switch (event.type) {
      case InputType::kDown:
        start_ = event.page;
        Begin(event);
        return;
      case InputType::kMove:
        if (state_ == GestureState::kInit) {
          start_ = event.page;
          Begin(event);
          return;
        }
        if (state_ == GestureState::kBegin &&
            (std::abs(event.page.x - start_.x) >
                 definition_.config.min_distance ||
             std::abs(event.page.y - start_.y) >
                 definition_.config.min_distance)) {
          Activate(event);
        }
        if (state_ == GestureState::kActive) {
          Update(event);
        }
        return;
      case InputType::kUp:
        Fail(event);
        return;
      case InputType::kCancel:
        Cancel(event);
        return;
      case InputType::kFlingFrame:
        return;
    }
  }

 private:
  Point start_;
};

class TapGestureHandler : public GestureHandler {
 public:
  using GestureHandler::GestureHandler;

  void Handle(const InputEvent& event,
              GestureInteractionContext& context) override {
    last_event_ = event;
    if (state_ == GestureState::kFail || IsTerminal()) {
      return;
    }
    switch (event.type) {
      case InputType::kDown:
        start_ = event.page;
        start_time_ms_ = event.monotonic_time_ms;
        Begin(event);
        timer_ = ScheduleTimer(definition_.config.max_duration_ms);
        return;
      case InputType::kMove:
        if (state_ == GestureState::kBegin && ExceedsDistance(event.page)) {
          CancelTimer(timer_);
          Fail(event);
        }
        return;
      case InputType::kUp:
        CancelTimer(timer_);
        if (state_ == GestureState::kBegin) {
          if (ExceedsDistance(event.page) ||
              event.monotonic_time_ms - start_time_ms_ >
                  definition_.config.max_duration_ms) {
            Fail(event);
          } else {
            Activate(event);
            End(event);
          }
        }
        return;
      case InputType::kCancel:
        CancelTimer(timer_);
        Cancel(event);
        return;
      case InputType::kFlingFrame:
        return;
    }
  }

  void HandleTimer(TimerToken token, const InputEvent& event) override {
    if (token != timer_) {
      return;
    }
    timer_ = 0;
    TimerFired(token);
    if (state_ == GestureState::kBegin) {
      Fail(event);
    }
  }

 private:
  bool ExceedsDistance(const Point& point) const {
    return std::abs(point.x - start_.x) > definition_.config.max_distance ||
           std::abs(point.y - start_.y) > definition_.config.max_distance;
  }

  Point start_;
  double start_time_ms_ = 0;
  TimerToken timer_ = 0;
};

class LongPressGestureHandler : public GestureHandler {
 public:
  using GestureHandler::GestureHandler;

  void Handle(const InputEvent& event,
              GestureInteractionContext& context) override {
    last_event_ = event;
    if (state_ == GestureState::kFail || IsTerminal()) {
      return;
    }
    switch (event.type) {
      case InputType::kDown:
        start_ = event.page;
        Begin(event);
        timer_ = ScheduleTimer(definition_.config.min_duration_ms);
        return;
      case InputType::kMove:
        if (state_ == GestureState::kBegin && ExceedsDistance(event.page)) {
          CancelTimer(timer_);
          Fail(event);
        }
        return;
      case InputType::kUp:
        CancelTimer(timer_);
        state_ == GestureState::kActive ? End(event) : Fail(event);
        return;
      case InputType::kCancel:
        CancelTimer(timer_);
        Cancel(event);
        return;
      case InputType::kFlingFrame:
        return;
    }
  }

  void HandleTimer(TimerToken token, const InputEvent& event) override {
    if (token != timer_) {
      return;
    }
    timer_ = 0;
    TimerFired(token);
    if (state_ == GestureState::kBegin) {
      Activate(event);
    }
  }

 private:
  bool ExceedsDistance(const Point& point) const {
    return std::abs(point.x - start_.x) > definition_.config.max_distance ||
           std::abs(point.y - start_.y) > definition_.config.max_distance;
  }

  Point start_;
  TimerToken timer_ = 0;
};

class DefaultGestureHandler : public GestureHandler {
 public:
  using GestureHandler::GestureHandler;

  void Handle(const InputEvent& event,
              GestureInteractionContext& context) override {
    if (event.type != InputType::kFlingFrame) {
      last_event_ = event;
    }
    if (state_ == GestureState::kFail || IsTerminal()) {
      return;
    }
    switch (event.type) {
      case InputType::kDown:
        last_point_ = event.page;
        Begin(event);
        return;
      case InputType::kMove: {
        if (state_ == GestureState::kInit) {
          last_point_ = event.page;
          Begin(event);
          return;
        }
        Point delta{last_point_.x - event.page.x, last_point_.y - event.page.y};
        last_point_ = event.page;
        HandleDelta(event, delta, context);
        return;
      }
      case InputType::kUp:
        return;
      case InputType::kCancel:
        Cancel(event);
        return;
      case InputType::kFlingFrame:
        if (event.fling_finished) {
          if (state_ == GestureState::kActive ||
              state_ == GestureState::kBegin) {
            End(event);
          }
          return;
        }
        HandleDelta(event, event.delta, context);
        return;
    }
  }

  void HandleSimultaneous(const InputEvent& event, const Point& delta,
                          GestureInteractionContext& context) override {
    if (!context.has_simultaneous_delta || state_ == GestureState::kEnd ||
        state_ == GestureState::kCancel || !host_.IsMemberValid(member_id()) ||
        !host_.ShouldConsumeGesture(member_id())) {
      return;
    }
    const auto direction = EffectiveDirection(delta, context);
    if (MatchesMemberDirection(direction) &&
        host_.CanConsumeGesture(member_id(), direction, delta)) {
      host_.ScrollBy(member_id(), delta);
    }
  }

 private:
  GestureDirection EffectiveDirection(
      const Point& delta, GestureInteractionContext& context) const {
    if (context.direction == GestureDirection::kUndetermined) {
      context.direction = std::abs(delta.x) > std::abs(delta.y)
                              ? GestureDirection::kHorizontal
                              : GestureDirection::kVertical;
    }
    return context.direction;
  }

  bool MatchesMemberDirection(GestureDirection direction) const {
    const auto member_direction = host_.GetScrollDirection(member_id());
    return member_direction == GestureDirection::kUndetermined ||
           member_direction == direction;
  }

  void HandleDelta(const InputEvent& event, Point delta,
                   GestureInteractionContext& context) {
    if ((delta.x == 0 && delta.y == 0) || state_ == GestureState::kFail ||
        IsTerminal()) {
      return;
    }
    const auto direction = EffectiveDirection(delta, context);
    if (direction == GestureDirection::kHorizontal) {
      delta.y = 0;
    } else {
      delta.x = 0;
    }
    context.has_simultaneous_delta = true;
    context.simultaneous_delta = delta;

    if (!MatchesMemberDirection(direction) ||
        !host_.ShouldConsumeGesture(member_id()) ||
        !host_.CanConsumeGesture(member_id(), direction, delta)) {
      Fail(event);
      return;
    }
    if (state_ == GestureState::kInit) {
      Begin(event);
    }
    Activate(event);
    if (state_ != GestureState::kActive || !host_.IsMemberValid(member_id())) {
      return;
    }
    host_.ScrollBy(member_id(), delta);
    if (state_ != GestureState::kActive || !host_.IsMemberValid(member_id())) {
      return;
    }
    if (std::abs(delta.x) > definition_.config.tap_slop ||
        std::abs(delta.y) > definition_.config.tap_slop) {
      host_.OnGestureRecognized(member_id());
    }
    Update(event, delta);
  }

  Point last_point_;
};

class FlingGestureHandler : public GestureHandler {
 public:
  using GestureHandler::GestureHandler;

  void Handle(const InputEvent& event,
              GestureInteractionContext& context) override {
    last_event_ = event;
    if (state_ == GestureState::kFail || IsTerminal()) {
      return;
    }
    switch (event.type) {
      case InputType::kDown:
      case InputType::kMove:
        return;
      case InputType::kUp:
        Begin(event);
        return;
      case InputType::kCancel:
        Cancel(event);
        return;
      case InputType::kFlingFrame:
        if (event.fling_finished) {
          state_ == GestureState::kActive ? End(event) : Fail(event);
          return;
        }
        if (state_ == GestureState::kInit) {
          Begin(event);
        }
        Activate(event);
        Update(event, event.delta);
        return;
    }
  }
};

}  // namespace

GestureHandler::GestureHandler(MemberId member_id, GestureDefinition definition,
                               GestureArenaImpl& host)
    : key_{member_id, definition.gesture_id},
      definition_(std::move(definition)),
      host_(host) {}

GestureHandler::~GestureHandler() { CancelAllTimers(); }

void GestureHandler::HandleSimultaneous(const InputEvent& event,
                                        const Point& delta,
                                        GestureInteractionContext& context) {
  Handle(event, context);
}

void GestureHandler::HandleTimer(TimerToken token, const InputEvent& event) {
  TimerFired(token);
}

void GestureHandler::DispatchTouchCallback(GestureCallbackType callback,
                                           const InputEvent& event) {
  if (IsSubscribed(callback)) {
    host_.Emit(key_, callback, event, {});
  }
}

void GestureHandler::Reset() {
  CancelAllTimers();
  state_ = GestureState::kInit;
  last_event_ = {};
  began_ = false;
  started_ = false;
  ended_ = false;
}

void GestureHandler::FailFromCompetition() { Fail(last_event_); }

void GestureHandler::EndFromCommand() { End(last_event_); }

void GestureHandler::CancelForTermination(const InputEvent& event) {
  Cancel(event);
}

bool GestureHandler::IsSubscribed(GestureCallbackType callback) const {
  return std::find(definition_.callbacks.begin(), definition_.callbacks.end(),
                   callback) != definition_.callbacks.end();
}

void GestureHandler::Begin(const InputEvent& event) {
  if (state_ != GestureState::kInit) {
    return;
  }
  last_event_ = event;
  began_ = true;
  TransitionTo(GestureState::kBegin);
  if (IsSubscribed(GestureCallbackType::kBegin)) {
    host_.Emit(key_, GestureCallbackType::kBegin, event, {});
  }
}

void GestureHandler::Activate(const InputEvent& event) {
  if (state_ == GestureState::kInit) {
    Begin(event);
  }
  if (state_ != GestureState::kBegin && state_ != GestureState::kActive) {
    return;
  }
  if (state_ != GestureState::kActive) {
    TransitionTo(GestureState::kActive);
  }
  if (!started_) {
    started_ = true;
    if (IsSubscribed(GestureCallbackType::kStart)) {
      host_.Emit(key_, GestureCallbackType::kStart, event, {});
    }
  }
}

void GestureHandler::Update(const InputEvent& event, const Point& delta) {
  if (state_ == GestureState::kActive &&
      IsSubscribed(GestureCallbackType::kUpdate)) {
    host_.Emit(key_, GestureCallbackType::kUpdate, event, delta);
  }
}

void GestureHandler::Fail(const InputEvent& event) {
  if (state_ == GestureState::kFail || IsTerminal()) {
    return;
  }
  Finish(event, GestureState::kFail);
}

void GestureHandler::End(const InputEvent& event) {
  if (state_ == GestureState::kEnd || state_ == GestureState::kCancel) {
    return;
  }
  Finish(event, GestureState::kEnd);
}

void GestureHandler::Cancel(const InputEvent& event) {
  if (state_ == GestureState::kEnd || state_ == GestureState::kCancel) {
    return;
  }
  Finish(event, GestureState::kCancel);
}

TimerToken GestureHandler::ScheduleTimer(double delay_ms) {
  if ((state_ != GestureState::kBegin && state_ != GestureState::kActive) ||
      !host_.IsMemberValid(member_id())) {
    return 0;
  }
  const auto token = host_.ScheduleTimer(key_, delay_ms);
  if (token == 0) {
    return 0;
  }
  if ((state_ != GestureState::kBegin && state_ != GestureState::kActive) ||
      !host_.IsMemberValid(member_id())) {
    host_.CancelTimer(token);
    return 0;
  }
  timers_.push_back(token);
  return token;
}

void GestureHandler::CancelTimer(TimerToken& token) {
  if (token == 0) {
    return;
  }
  host_.CancelTimer(token);
  TimerFired(token);
  token = 0;
}

void GestureHandler::TimerFired(TimerToken token) {
  timers_.erase(std::remove(timers_.begin(), timers_.end(), token),
                timers_.end());
}

bool GestureHandler::IsTerminal() const {
  return state_ == GestureState::kEnd || state_ == GestureState::kCancel;
}

void GestureHandler::CancelAllTimers() {
  for (const auto token : timers_) {
    host_.CancelTimer(token);
  }
  timers_.clear();
}

void GestureHandler::TransitionTo(GestureState state) {
  state_ = state;
  host_.OnGestureStateChanged(key_, state);
}

void GestureHandler::Finish(const InputEvent& event, GestureState state) {
  CancelAllTimers();
  TransitionTo(state);
  if (began_ && !ended_) {
    ended_ = true;
    if (IsSubscribed(GestureCallbackType::kEnd)) {
      host_.Emit(key_, GestureCallbackType::kEnd, event, {});
    }
  }
}

std::unique_ptr<GestureHandler> CreateGestureHandler(
    MemberId member_id, GestureDefinition definition, GestureArenaImpl& host) {
  switch (definition.gesture_type) {
    case GestureType::PAN:
    case GestureType::NATIVE:
      return std::make_unique<PanGestureHandler>(member_id,
                                                 std::move(definition), host);
    case GestureType::TAP:
      return std::make_unique<TapGestureHandler>(member_id,
                                                 std::move(definition), host);
    case GestureType::LONG_PRESS:
      return std::make_unique<LongPressGestureHandler>(
          member_id, std::move(definition), host);
    case GestureType::DEFAULT:
      return std::make_unique<DefaultGestureHandler>(
          member_id, std::move(definition), host);
    case GestureType::FLING:
      return std::make_unique<FlingGestureHandler>(member_id,
                                                   std::move(definition), host);
    case GestureType::ROTATION:
    case GestureType::PINCH:
      return nullptr;
  }
}

GestureArena::GestureArena(GestureArenaDelegate& delegate)
    : impl_(std::make_unique<GestureArenaImpl>(delegate)) {}

GestureArena::~GestureArena() = default;

void GestureArena::ReplaceMemberGestures(
    MemberId member_id,
    const std::vector<const GestureDetector*>& gesture_detectors) {
  impl_->ReplaceMemberGestures(member_id,
                               NormalizeGestureDetectors(gesture_detectors));
}

void GestureArena::ReplaceMemberGestures(
    MemberId member_id, std::vector<GestureDefinition> gestures) {
  impl_->ReplaceMemberGestures(member_id, std::move(gestures));
}

void GestureArena::RemoveMember(MemberId member_id) {
  impl_->RemoveMember(member_id);
}

bool GestureArena::ContainsMember(MemberId member_id) const {
  return impl_->ContainsMember(member_id);
}

bool GestureArena::ContainsGesture(MemberId member_id,
                                   uint32_t gesture_id) const {
  return impl_->ContainsGesture(member_id, gesture_id);
}

void GestureArena::HandleInput(const InputEvent& event,
                               const std::vector<MemberId>& response_chain) {
  impl_->HandleInput(event, response_chain);
}

void GestureArena::HandleTimer(TimerToken token, double monotonic_time_ms,
                               int64_t epoch_time_ms) {
  impl_->HandleTimer(token, monotonic_time_ms, epoch_time_ms);
}

void GestureArena::SetGestureState(MemberId member_id, uint32_t gesture_id,
                                   GestureStateCommand command) {
  impl_->SetGestureState(member_id, gesture_id, command);
}

void GestureArena::Reset() { impl_->Reset(); }

}  // namespace lynx::tasm::gesture
