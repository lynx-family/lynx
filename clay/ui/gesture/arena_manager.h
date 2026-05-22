// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_ARENA_MANAGER_H_
#define CLAY_UI_GESTURE_ARENA_MANAGER_H_

#include <functional>
#include <map>
#include <memory>

#include "clay/fml/logging.h"
#include "clay/ui/event/gesture_event.h"
#include "clay/ui/gesture/arena.h"
#include "clay/ui/gesture/arena_member.h"

namespace clay {

enum class GestureDisposition { kAccept, kReject };

class ArenaManager;

// Entry for resolve gesture.
// It acts like a delegate for ArenaManager.
class ArenaEntry {
 public:
  ArenaEntry(ArenaManager* arena_manager, int pointer_id,
             fml::WeakPtr<ArenaMember> member)
      : arena_manager_(arena_manager),
        pointer_id_(pointer_id),
        member_(member) {
    FML_DCHECK(arena_manager_ && member_);
  }

  // Interface for operating arena manager.
  void Resolve(GestureDisposition disposition);

 private:
  ArenaManager* arena_manager_;
  int pointer_id_;
  fml::WeakPtr<ArenaMember> member_;
};

class ArenaManager final {
 public:
  ArenaManager() = default;
  ArenaManager(const ArenaManager&) = delete;
  ArenaManager& operator=(const ArenaManager&) = delete;

  std::unique_ptr<ArenaEntry> Add(int pointer_id,
                                  fml::WeakPtr<ArenaMember> member);
  void Close(const PointerEvent& event);
  void Sweep(int pointer_id);
  void Hold(int pointer_id);
  void Release(int pointer_id);

  void SetListenerForNotCaredPointer(
      std::function<void(const PointerEvent&)> cb) {
    on_pointer_not_cared_ = cb;
  }

  void SetHasOuterGestures(bool value) { has_outer_gestures_ = value; }

 private:
  friend class ArenaEntry;
  void Resolve(int pointer_id, const fml::WeakPtr<ArenaMember>& member,
               GestureDisposition disposition);

  // Called in Resolve when someone rejected
  void TryResolve(const PointerEvent& event, Arena* arena);
  // Called when only one member left (not rejected), make it winner.
  void ResolveByDefault(int pointer_id, Arena* arena);
  // Reject all members except the favor one
  void ResolveInFavorOf(int pointer_id, Arena* arena,
                        const fml::WeakPtr<ArenaMember>& favor_winner);

  std::unique_ptr<Arena> TakeArenaOwnership(int pointer_id);

  void OnPointerNotCared(const PointerEvent& event);

 private:
  // For those primary pointers which haven't been converted to a gesture that
  // some view cares.
  std::function<void(const PointerEvent&)> on_pointer_not_cared_;
  std::map<int, std::unique_ptr<Arena>> arenas_;
  std::map<int, PointerEvent> recording_events_;
  bool has_outer_gestures_ = false;
};

}  // namespace clay

#endif  // CLAY_UI_GESTURE_ARENA_MANAGER_H_
