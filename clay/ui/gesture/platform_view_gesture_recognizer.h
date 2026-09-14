// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_GESTURE_PLATFORM_VIEW_GESTURE_RECOGNIZER_H_
#define CLAY_UI_GESTURE_PLATFORM_VIEW_GESTURE_RECOGNIZER_H_

#include <map>
#include <memory>

#include "clay/ui/gesture/arena_manager.h"

namespace clay {

// Reserves the arena until the embedded view reports its decision. Raw touch
// delivery remains owned by NativeView, including after this member wins.
class PlatformViewGestureRecognizer final : public ArenaMember {
 public:
  explicit PlatformViewGestureRecognizer(ArenaManager* manager)
      : arena_manager_(manager) {}
  ~PlatformViewGestureRecognizer();

  void AddPointer(const PointerEvent& event);
  bool HasPendingPointer(int pointer_id) const;
  bool UpdateDecision(int pointer_id, GestureDisposition disposition);
  void CancelAll();
  const char* GetMemberTag() const override { return "[platform_view]"; }
  bool IsPlatformGestureMember() const override { return true; }

 protected:
  void OnGestureAccepted(int pointer_id) override;
  void OnGestureRejected(int pointer_id) override;

 private:
  ArenaManager* arena_manager_;
  std::map<int, std::unique_ptr<ArenaEntry>> pointers_;
};

}  // namespace clay

#endif  // CLAY_UI_GESTURE_PLATFORM_VIEW_GESTURE_RECOGNIZER_H_
