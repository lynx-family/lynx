// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_ARENA_H_
#define CLAY_UI_GESTURE_ARENA_H_

#include <list>

#include "base/include/fml/memory/weak_ptr.h"

namespace clay {

class ArenaMember;

struct Arena {
  void RemoveMember(const fml::WeakPtr<ArenaMember>& member) {
    for (auto iter = members.begin(); iter != members.end(); ++iter) {
      if (iter->get() == member.get()) {
        members.erase(iter);
        break;
      }
    }
  }
  // Order matters because when sweeping arena, the first member will win.
  std::list<fml::WeakPtr<ArenaMember>> members;
  // Close arena after all members entered in.
  bool is_open = true;
  // Hold the arena to prevent from closing.
  bool is_held = false;
  // If arena is held when sweep, |has_pending_sweep| should be set to true.
  bool has_pending_sweep = false;
  // Temp record the member who declared to be accepted.
  fml::WeakPtr<ArenaMember> eager_winner;
};

}  // namespace clay

#endif  // CLAY_UI_GESTURE_ARENA_H_
