// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_ARENA_MEMBER_H_
#define CLAY_UI_GESTURE_ARENA_MEMBER_H_

#include "base/include/fml/memory/weak_ptr.h"

namespace clay {

class ArenaMember {
 public:
  ArenaMember() : weak_factory_(this) {}
  // Callbacks when it has been accepted or rejected.
  virtual void OnGestureAccepted(int pointer_id) = 0;
  virtual void OnGestureRejected(int pointer_id) = 0;
  // Useful for debug
  virtual const char* GetMemberTag() const = 0;

 protected:
  fml::WeakPtrFactory<ArenaMember> weak_factory_;
};

}  // namespace clay

#endif  // CLAY_UI_GESTURE_ARENA_MEMBER_H_
