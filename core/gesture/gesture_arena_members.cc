// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/gesture/gesture_arena_impl.h"

namespace lynx::tasm::gesture {

namespace {

bool ConfigEquals(const GestureConfig &left, const GestureConfig &right) {
  return left.min_distance == right.min_distance &&
         left.max_distance == right.max_distance &&
         left.min_duration_ms == right.min_duration_ms &&
         left.max_duration_ms == right.max_duration_ms &&
         left.tap_slop == right.tap_slop;
}

bool RelationsEqual(const GestureRelations &left,
                    const GestureRelations &right) {
  return left.simultaneous == right.simultaneous &&
         left.wait_for == right.wait_for &&
         left.continue_with == right.continue_with;
}

bool DefinitionEquals(const GestureDefinition &left,
                      const GestureDefinition &right) {
  return left.gesture_id == right.gesture_id &&
         left.gesture_type == right.gesture_type &&
         ConfigEquals(left.config, right.config) &&
         RelationsEqual(left.relations, right.relations) &&
         left.callbacks == right.callbacks;
}

bool DefinitionsEqual(const std::vector<GestureDefinition> &left,
                      const std::vector<GestureDefinition> &right) {
  return left.size() == right.size() &&
         std::equal(left.begin(), left.end(), right.begin(), DefinitionEquals);
}

}  // namespace

void GestureArenaImpl::ReplaceMemberGestures(
    MemberId member_id, std::vector<GestureDefinition> definitions) {
  QueueMutation(member_id, std::move(definitions));
  FlushMutations();
}

void GestureArenaImpl::RemoveMember(MemberId member_id) {
  QueueMutation(member_id, std::nullopt);
  FlushMutations();
}

bool GestureArenaImpl::ContainsMember(MemberId member_id) const {
  return members_.find(member_id) != members_.end() &&
         invalidated_members_.find(member_id) == invalidated_members_.end();
}

bool GestureArenaImpl::ContainsGesture(MemberId member_id,
                                       uint32_t gesture_id) const {
  if (!ContainsMember(member_id)) {
    return false;
  }
  const auto member = members_.find(member_id);
  return member != members_.end() && member->second.handlers.find(gesture_id) !=
                                         member->second.handlers.end();
}

void GestureArenaImpl::QueueMutation(
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
    // A reentrant update can restore the current definition before it is
    // applied.
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
  // Invalidate dispatch immediately; defer destruction until callbacks unwind.
  pending_mutations_.insert_or_assign(member_id,
                                      PendingMutation{std::move(definitions)});
  invalidated_members_.insert(member_id);
}

void GestureArenaImpl::FlushMutations() {
  if (dispatch_depth_ != 0 || flushing_mutations_ || resetting_) {
    return;
  }
  if (reset_pending_) {
    ApplyReset();
    return;
  }
  flushing_mutations_ = true;
  // Termination callbacks can enqueue more changes; drain without recursion.
  while (!pending_mutations_.empty() && !reset_pending_) {
    auto pending = std::move(pending_mutations_);
    pending_mutations_.clear();
    for (auto &[member_id, mutation] : pending) {
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

void GestureArenaImpl::ApplyReplacement(
    MemberId member_id, std::vector<GestureDefinition> definitions) {
  const auto old = members_.find(member_id);
  if (old != members_.end() &&
      DefinitionsEqual(old->second.definitions, definitions)) {
    return;
  }

  ApplyRemoval(member_id);
  MemberRecord record;
  record.generation = next_generation_++;
  record.definitions = std::move(definitions);
  for (const auto &definition : record.definitions) {
    auto handler = CreateGestureHandler(member_id, definition, *this);
    if (handler) {
      record.handlers.insert_or_assign(definition.gesture_id,
                                       std::move(handler));
      gesture_members_[definition.gesture_id].insert(member_id);
    }
  }
  members_.insert_or_assign(member_id, std::move(record));
}

void GestureArenaImpl::ApplyRemoval(MemberId member_id) {
  const auto it = members_.find(member_id);
  if (it == members_.end()) {
    return;
  }
  const bool was_winner = CurrentWinner() == member_id;
  InputEvent event = last_input_;
  event.type = InputType::kCancel;
  TerminateMember(member_id, event, GestureState::kCancel);
  for (const auto &definition : it->second.definitions) {
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

std::vector<HandlerKey> GestureArenaImpl::HandlerKeys(
    MemberId member_id) const {
  std::vector<HandlerKey> result;
  const auto member = members_.find(member_id);
  if (member == members_.end()) {
    return result;
  }
  result.reserve(member->second.handlers.size());
  for (const auto &[gesture_id, handler] : member->second.handlers) {
    result.push_back({member_id, gesture_id});
  }
  return result;
}

GestureHandler *GestureArenaImpl::FindHandler(const HandlerKey &key) {
  const auto member = members_.find(key.member_id);
  if (member == members_.end()) {
    return nullptr;
  }
  const auto handler = member->second.handlers.find(key.gesture_id);
  return handler == member->second.handlers.end() ? nullptr
                                                  : handler->second.get();
}

bool GestureArenaImpl::IsCurrentGeneration(MemberId member_id,
                                           uint64_t generation) const {
  const auto member = members_.find(member_id);
  return member != members_.end() && member->second.generation == generation &&
         invalidated_members_.find(member_id) == invalidated_members_.end();
}

std::vector<MemberId> GestureArenaImpl::RegisteredMemberIds() const {
  std::vector<MemberId> result;
  result.reserve(members_.size());
  for (const auto &[member_id, member] : members_) {
    result.push_back(member_id);
  }
  return result;
}

}  // namespace lynx::tasm::gesture
