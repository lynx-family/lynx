// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/gesture/arena_manager.h"

#include <memory>
#include <utility>

#include "clay/ui/gesture/macros.h"

namespace clay {

std::unique_ptr<ArenaEntry> ArenaManager::Add(
    int pointer_id, fml::WeakPtr<ArenaMember> member) {
  auto& arena = arenas_[pointer_id];
  if (!arena) {
    arena = std::make_unique<Arena>();
  }
  arena->members.emplace_back(member);
  return std::make_unique<ArenaEntry>(this, pointer_id, std::move(member));
}

void ArenaEntry::Resolve(GestureDisposition disposition) {
  GESTURE_LOG << "pointer " << pointer_id_ << " of " << member_->GetMemberTag()
              << member_.get() << " resolved with "
              << (disposition == GestureDisposition::kAccept ? "ACCEPT"
                                                             : "REJECT");
  arena_manager_->Resolve(pointer_id_, member_, disposition);
}

void ArenaManager::Close(const PointerEvent& event) {
  int pointer_id = event.pointer_id;
  recording_events_.emplace(pointer_id, event);
  auto iter = arenas_.find(pointer_id);
  if (iter == arenas_.end()) {
    OnPointerNotCared(event);
    return;
  }

  Arena* arena = iter->second.get();
  FML_DCHECK(arena->is_open);
  arena->is_open = false;
  TryResolve(event, arena);
}

void ArenaManager::Sweep(int pointer_id) {
  auto iter = arenas_.find(pointer_id);
  if (iter == arenas_.end()) {
    // `OnPointerNotCared` has already been notified when `Close`
    return;
  }

  Arena* arena = iter->second.get();
  FML_DCHECK(!arena->is_open);

  if (arena->is_held) {
    arena->has_pending_sweep = true;
    // Arena will be swept when released.
    GESTURE_LOG << "[sweep] arena was held.";
    return;
  }

  std::unique_ptr<Arena> holder = TakeArenaOwnership(pointer_id);
  bool first_member = true;
  for (auto& member : holder->members) {
    if (!member) {
      continue;
    }
    if (first_member) {
      GESTURE_LOG << "[sweep] accept first member: " << member.get()
                  << member->GetMemberTag();
      // Choose first member win. Others reject.
      member->OnGestureAccepted(pointer_id);
      first_member = false;
    } else {
      GESTURE_LOG << "[sweep] reject other members: " << member.get()
                  << member->GetMemberTag();
      member->OnGestureRejected(pointer_id);
    }
  }
}

void ArenaManager::Hold(int pointer_id) {
  auto iter = arenas_.find(pointer_id);
  if (iter == arenas_.end()) {
    return;
  }

  iter->second->is_held = true;
}

void ArenaManager::Release(int pointer_id) {
  auto iter = arenas_.find(pointer_id);
  if (iter == arenas_.end()) {
    GESTURE_LOG << "Invalid release op for pointer " << pointer_id;
    return;
  }

  iter->second->is_held = false;
  if (iter->second->has_pending_sweep) {
    iter->second->has_pending_sweep = false;
    GESTURE_LOG << "Release arena of pointer " << pointer_id;
    Sweep(pointer_id);
  }
}

void ArenaManager::Resolve(int pointer_id,
                           const fml::WeakPtr<ArenaMember>& member,
                           GestureDisposition disposition) {
  auto iter = arenas_.find(pointer_id);
  if (iter == arenas_.end()) {
    GESTURE_LOG << "pointer_id=" << pointer_id << " already been resolved.";
    return;
  }

  Arena* arena = iter->second.get();
  if (!member) {
    arena->RemoveMember(member);
    return;
  }

  GESTURE_LOG << "arena member " << member->GetMemberTag() << member.get()
              << "(pointer=" << pointer_id
              << ") resolve arena with disposition "
              << (disposition == GestureDisposition::kAccept ? "accept"
                                                             : "reject");

  PointerEvent event;
  if (recording_events_.find(pointer_id) != recording_events_.end()) {
    event = recording_events_[pointer_id];
  } else {
    // we pass the point event to TryResolve function in order to let the
    //   OnPointerNotCared callback has a chance to know the positions, then to
    //  find the ignore-focus nodes. in this exectue path , we only exceute the
    // callback when there is no areas, so no nodes gesture will be handled, and
    // the positions is uselesss;
    event = PointerEvent();
    event.pointer_id = pointer_id;
  }
  // FML_DCHECK(arena->members.find(member) != arena->members.end());
  if (disposition == GestureDisposition::kReject) {
    arena->RemoveMember(member);
    GESTURE_LOG << "arena member " << member->GetMemberTag() << member.get()
                << " was removed. Members count: " << arena->members.size();
    member->OnGestureRejected(pointer_id);
    if (!arena->is_open) {
      TryResolve(event, arena);
    }
  } else {
    if (arena->is_open) {
      GESTURE_LOG << "arena wasn't closed. set as eager winner.";
      // If arena is not closed, we need to set the member as eager winner,
      // which will be resolved immediately when the arena is closed. Do not
      // overwrite the previous winner.
      if (!arena->eager_winner) {
        arena->eager_winner = member;
      }
    } else {
      ResolveInFavorOf(pointer_id, arena, member);
    }
  }
}

void ArenaManager::TryResolve(const PointerEvent& event, Arena* arena) {
  int pointer_id = event.pointer_id;
  if (arena->members.size() == 1 && !has_outer_gestures_) {
    ResolveByDefault(pointer_id, arena);
  } else if (arena->members.empty()) {
    GESTURE_LOG << "arena has no members, remove it.";
    arenas_.erase(pointer_id);
    recording_events_.erase(pointer_id);
    OnPointerNotCared(event);
  } else if (arena->eager_winner) {
    ResolveInFavorOf(pointer_id, arena, arena->eager_winner);
  }
}

void ArenaManager::ResolveInFavorOf(
    int pointer_id, Arena* arena,
    const fml::WeakPtr<ArenaMember>& favor_winner) {
  FML_DCHECK(!arena->is_open);
  std::unique_ptr<Arena> holder = TakeArenaOwnership(pointer_id);
  if (!favor_winner) {
    return;
  }
  GESTURE_LOG << "resolve arena in favor of eager winner "
              << favor_winner->GetMemberTag() << favor_winner.get();
  FML_DCHECK(holder.get() == arena);
  for (auto& member : arena->members) {
    if (member.get() != nullptr && favor_winner.get() != nullptr) {
      if (member != favor_winner) {
        member->OnGestureRejected(pointer_id);
      }
    }
  }

  favor_winner->OnGestureAccepted(pointer_id);
}

void ArenaManager::ResolveByDefault(int pointer_id, Arena* arena) {
  GESTURE_LOG << "resolve arena by default";
  FML_DCHECK(arena->members.size() == 1);
  std::unique_ptr<Arena> holder = TakeArenaOwnership(pointer_id);
  FML_DCHECK(holder.get() == arena);
  const auto& default_winner = *arena->members.begin();
  if (default_winner) {
    // Remove first to avoid dead lock.
    default_winner->OnGestureAccepted(pointer_id);
  }
}

std::unique_ptr<Arena> ArenaManager::TakeArenaOwnership(int pointer_id) {
  std::unique_ptr<Arena> owner;
  auto iter = arenas_.find(pointer_id);
  if (iter == arenas_.end()) {
    FML_UNREACHABLE();
    return nullptr;
  }
  owner.swap(iter->second);
  arenas_.erase(iter);
  return owner;
}

void ArenaManager::OnPointerNotCared(const PointerEvent& event) {
  if (on_pointer_not_cared_) {
    on_pointer_not_cared_(event);
  }
}

}  // namespace clay
