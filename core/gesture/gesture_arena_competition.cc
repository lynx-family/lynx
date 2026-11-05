// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/gesture/gesture_arena_impl.h"

namespace lynx::tasm::gesture {

namespace {

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

std::vector<MemberId> GestureArenaImpl::ConvertResponseChain(
    const std::vector<MemberId> &response_chain) const {
  std::vector<MemberId> result;
  for (size_t current_index = 0; current_index < response_chain.size();
       ++current_index) {
    const auto member_id = response_chain[current_index];
    const auto member = members_.find(member_id);
    if (member == members_.end() || member->second.handlers.empty()) {
      continue;
    }

    const GestureRelations *selected_relations = nullptr;
    for (const auto &definition : member->second.definitions) {
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
        if (registry == gesture_members_.end()) {
          continue;
        }
        for (const auto wait_member : registry->second) {
          if (std::find(response_chain.begin() + current_index + 1,
                        response_chain.end(),
                        wait_member) != response_chain.end() &&
              wait_members.insert(wait_member).second) {
            result.push_back(wait_member);
          }
        }
      }
      result.push_back(member_id);
      // Unmatched waitFor leaves the remaining response chain intact.
      if (wait_members.empty()) {
        continue;
      }
    } else {
      result.push_back(member_id);
    }

    // continueWith can promote registered members outside the original hit
    // chain.
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

void GestureArenaImpl::DispatchTouches(const InputEvent &event) {
  const auto callback = TouchCallback(event.type);
  if (!callback) {
    return;
  }
  const auto members = response_chain_;
  for (const auto member_id : members) {
    for (const auto &key : HandlerKeys(member_id)) {
      if (!IsActiveMember(member_id)) {
        break;
      }
      CallHandler(key, [callback, &event](GestureHandler &handler) {
        handler.DispatchTouchCallback(*callback, event);
      });
    }
  }
}

void GestureArenaImpl::DispatchWinner(const InputEvent &event) {
  const auto winner = CurrentWinner();
  if (!winner || !IsActiveMember(*winner)) {
    return;
  }
  context_.has_simultaneous_delta = false;
  context_.simultaneous_delta = {};
  for (const auto &key : HandlerKeys(*winner)) {
    CallHandler(key, [&event, this](GestureHandler &handler) {
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
    for (const auto &key : HandlerKeys(member_id)) {
      CallHandler(
          key, [&event, &simultaneous_delta, this](GestureHandler &handler) {
            handler.HandleSimultaneous(event, simultaneous_delta, context_);
          });
    }
  }
  context_.has_simultaneous_delta = false;
  context_.simultaneous_delta = {};
  ReevaluateCompetition();
}

void GestureArenaImpl::ReevaluateCompetition() {
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
  if (aggregate.state == GestureState::kActive && aggregate.active_gesture_id) {
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

GestureArenaImpl::AggregateState GestureArenaImpl::Aggregate(
    MemberId member_id) const {
  const auto member = members_.find(member_id);
  if (member == members_.end() || member->second.handlers.empty()) {
    return {};
  }
  bool has_begin = false;
  bool has_init = false;
  bool has_fail = false;
  bool has_dormant_fling = false;
  std::optional<uint32_t> active;
  for (const auto &[gesture_id, handler] : member->second.handlers) {
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
  // A dormant fling must not keep a failed touch recognizer in control.
  return {has_dormant_fling && !has_fail ? GestureState::kInit
                                         : GestureState::kFail,
          std::nullopt};
}

void GestureArenaImpl::ApplySameMemberRace(MemberId member_id,
                                           uint32_t active_gesture_id) {
  const auto simultaneous_ids = SameMemberSimultaneousIds(member_id);
  for (const auto &key : HandlerKeys(member_id)) {
    if (key.gesture_id != active_gesture_id &&
        simultaneous_ids.find(key.gesture_id) == simultaneous_ids.end()) {
      CallHandler(
          key, [](GestureHandler &handler) { handler.FailFromCompetition(); });
    }
  }
}

std::unordered_set<uint32_t> GestureArenaImpl::SameMemberSimultaneousIds(
    MemberId member_id) const {
  std::unordered_set<uint32_t> handler_ids;
  std::unordered_set<uint32_t> simultaneous_ids;
  const auto member = members_.find(member_id);
  if (member == members_.end()) {
    return simultaneous_ids;
  }
  for (const auto &[gesture_id, handler] : member->second.handlers) {
    handler_ids.insert(gesture_id);
  }
  for (const auto &definition : member->second.definitions) {
    for (const auto gesture_id : definition.relations.simultaneous) {
      if (handler_ids.find(gesture_id) != handler_ids.end()) {
        simultaneous_ids.insert(gesture_id);
      }
    }
  }
  return simultaneous_ids;
}

void GestureArenaImpl::UpdateSimultaneousMembers() {
  simultaneous_members_.clear();
  const auto winner = CurrentWinner();
  if (!winner || !IsActiveMember(*winner)) {
    return;
  }
  const auto member = members_.find(*winner);
  if (member == members_.end()) {
    return;
  }
  for (const auto &definition : member->second.definitions) {
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

std::optional<size_t> GestureArenaImpl::FindNextWinner(
    std::optional<size_t> current_index) {
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
      for (const auto &key : HandlerKeys(member_id)) {
        CallHandler(key, [](GestureHandler &handler) { handler.Reset(); });
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

}  // namespace lynx::tasm::gesture
