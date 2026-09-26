// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/gesture/arena_manager.h"

#include <memory>
#include <utility>

#include "clay/ui/gesture/macros.h"

namespace clay {
namespace {

void RemoveInvalidMembers(Arena* arena) {
  for (auto iter = arena->members.begin(); iter != arena->members.end();) {
    if (!*iter) {
      iter = arena->members.erase(iter);
    } else {
      ++iter;
    }
  }

  if (!arena->eager_winner) {
    arena->eager_winner.reset();
  }
}

}  // namespace

std::unique_ptr<ArenaEntry> ArenaManager::Add(
    int pointer_id, fml::WeakPtr<ArenaMember> member) {
  auto iter = arenas_.find(pointer_id);
  if (iter != arenas_.end()) {
    RemoveInvalidMembers(iter->second.get());
    // Only drop stale arena when it has no remaining valid members.
    // Dropping a closed arena that still has members can cause unresolved
    // callbacks (TryResolve / OnGesture*) and state pollution if `pointer_id`
    // is reused before the previous members finish resolving.
    if (iter->second->members.empty()) {
      GESTURE_LOG << "drop stale arena before add, pointer: " << pointer_id
                  << ", is_open: " << iter->second->is_open
                  << ", is_held: " << iter->second->is_held;
      arenas_.erase(iter);
    } else {
      FML_DCHECK(iter->second->is_open)
          << "Unexpected non-empty closed arena before add, pointer: "
          << pointer_id << ", members: " << iter->second->members.size();
    }
  }

  auto& arena = arenas_[pointer_id];
  if (!arena) {
    arena = std::make_unique<Arena>();
  }
  arena->members.emplace_back(member);
  return std::make_unique<ArenaEntry>(this, pointer_id, std::move(member));
}

bool ArenaEntry::Resolve(GestureDisposition disposition) {
  GESTURE_LOG << "pointer " << pointer_id_ << " of " << member_->GetMemberTag()
              << member_.get() << " resolved with "
              << (disposition == GestureDisposition::kAccept ? "ACCEPT"
                                                             : "REJECT");
  return arena_manager_->Resolve(pointer_id_, member_, disposition);
}

#if OS_HARMONY
void ArenaEntry::DeferToThisMember() {
  arena_manager_->DeferToMember(pointer_id_, member_);
}

bool ArenaEntry::IsPlatformArbitrationActive() const {
  auto it = arena_manager_->arenas_.find(pointer_id_);
  return member_ && it != arena_manager_->arenas_.end() &&
         !it->second->is_open && it->second->deferred_member == member_;
}

void ArenaManager::DeferToMember(int pointer_id,
                                 const fml::WeakPtr<ArenaMember>& member) {
  auto it = arenas_.find(pointer_id);
  FML_DCHECK(member);

  it->second->deferred_member = member;
}
#endif

void ArenaManager::Close(const PointerEvent& event) {
  int pointer_id = event.pointer_id;
  auto iter = arenas_.find(pointer_id);
  if (iter == arenas_.end()) {
    return;
  }

  Arena* arena = iter->second.get();
  RemoveInvalidMembers(arena);
  if (!arena->is_open) {
    TryResolve(pointer_id, arena);
    return;
  }

  arena->is_open = false;
  if (arena->deferred_member && arena->members.size() == 1) {
    FML_DCHECK(arena->members.front() == arena->deferred_member);
    // No cross-layer competition. Remove the provisional platform member
    // without declaring a winner or starting native recognizer observation.
    auto holder = TakeArenaOwnership(pointer_id);
    for (auto& member : holder->members) {
      if (member) {
        member->OnGestureRejected(pointer_id);
      }
    }
    return;
  }
  TryResolve(pointer_id, arena);
}

void ArenaManager::Sweep(int pointer_id) {
  auto iter = arenas_.find(pointer_id);
  if (iter == arenas_.end()) {
    // `OnPointerNotCared` has already been notified when `Close`
    return;
  }

  Arena* arena = iter->second.get();
  FML_DCHECK(!arena->is_open);

  if (arena->is_held || arena->deferred_member) {
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

bool ArenaManager::Resolve(int pointer_id,
                           const fml::WeakPtr<ArenaMember>& member,
                           GestureDisposition disposition) {
  auto iter = arenas_.find(pointer_id);
  if (iter == arenas_.end()) {
    GESTURE_LOG << "pointer_id=" << pointer_id << " already been resolved.";
    return false;
  }

  Arena* arena = iter->second.get();
  if (!member) {
    arena->RemoveMember(member);
    return false;
  }

  GESTURE_LOG << "arena member " << member->GetMemberTag() << member.get()
              << "(pointer=" << pointer_id
              << ") resolve arena with disposition "
              << (disposition == GestureDisposition::kAccept ? "accept"
                                                             : "reject");

  // FML_DCHECK(arena->members.find(member) != arena->members.end());
  if (disposition == GestureDisposition::kReject) {
    const bool releases_deferred_member = arena->deferred_member == member;
    if (releases_deferred_member) {
      arena->deferred_member.reset();
    }
    if (arena->eager_winner == member) {
      arena->eager_winner.reset();
    }
    arena->RemoveMember(member);
    GESTURE_LOG << "arena member " << member->GetMemberTag() << member.get()
                << " was removed. Members count: " << arena->members.size();
    member->OnGestureRejected(pointer_id);
    if (!arena->is_open) {
      TryResolve(pointer_id, arena);
      if (releases_deferred_member) {
        auto pending = arenas_.find(pointer_id);
        if (pending != arenas_.end() && pending->second->has_pending_sweep) {
          Sweep(pointer_id);
        }
      }
    }
    return false;
  } else {
    const bool waits_for_platform =
        arena->deferred_member && arena->deferred_member != member;
    if (arena->deferred_member == member) {
      arena->deferred_member.reset();
      // The platform's decision takes precedence over deferred local requests.
      arena->eager_winner = member;
    }
    if (arena->is_open || arena->deferred_member) {
      GESTURE_LOG << "arena cannot resolve yet. set as eager winner.";
      // If arena is not closed, we need to set the member as eager winner,
      // which will be resolved immediately when the arena is closed. A
      // deferred platform member also keeps the winner pending until the
      // platform responds. Do not overwrite the previous winner.
      if (!arena->eager_winner) {
        arena->eager_winner = member;
      }
    } else {
      ResolveInFavorOf(pointer_id, arena, member);
    }
    return waits_for_platform;
  }
}

void ArenaManager::TryResolve(int pointer_id, Arena* arena) {
  if (arena->deferred_member) {
    return;
  }
  if (arena->members.size() == 1 && !has_outer_gestures_) {
    ResolveByDefault(pointer_id, arena);
  } else if (arena->members.empty()) {
    GESTURE_LOG << "arena has no members, remove it.";
    arenas_.erase(pointer_id);
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

}  // namespace clay
