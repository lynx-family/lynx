// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/gesture/platform_view_gesture_recognizer.h"

#include <utility>

namespace clay {

PlatformViewGestureRecognizer::~PlatformViewGestureRecognizer() { CancelAll(); }

void PlatformViewGestureRecognizer::AddPointer(const PointerEvent& event) {
  auto pointer = event.pointer_id;
  auto entry = arena_manager_->Add(pointer, weak_factory_.GetWeakPtr());
  entry->DeferToThisMember();
  pointers_.emplace(pointer, std::move(entry));
}

bool PlatformViewGestureRecognizer::HasPendingPointer(int pointer_id) const {
  auto it = pointers_.find(pointer_id);
  if (it == pointers_.end()) {
    return false;
  }
  // Down is dispatched to ArkUI after the Clay arena closes. A lone platform
  // candidate must not start observation, nor report a decision before Close.
  return it->second->IsPlatformArbitrationActive();
}

bool PlatformViewGestureRecognizer::UpdateDecision(
    int pointer_id, GestureDisposition disposition) {
  auto it = pointers_.find(pointer_id);
  if (it == pointers_.end() || !it->second->IsPlatformArbitrationActive()) {
    return false;
  }
  it->second->Resolve(disposition);
  return true;
}

void PlatformViewGestureRecognizer::OnGestureAccepted(int pointer_id) {
  pointers_.erase(pointer_id);
}

void PlatformViewGestureRecognizer::OnGestureRejected(int pointer_id) {
  pointers_.erase(pointer_id);
}

void PlatformViewGestureRecognizer::CancelAll() {
  while (!pointers_.empty()) {
    auto it = pointers_.begin();
    auto entry = std::move(it->second);
    pointers_.erase(it);
    // Detach/destruction withdraws pending members from the arena.
    entry->Resolve(GestureDisposition::kReject);
  }
}

}  // namespace clay
