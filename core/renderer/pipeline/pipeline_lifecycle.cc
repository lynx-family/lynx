// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/pipeline/pipeline_lifecycle.h"

#include "base/include/log/logging.h"

namespace lynx {
namespace tasm {
bool PipelineLifecycle::AdvanceTo(LifecycleState next_state) {
  DCHECK(CanAdvanceTo(next_state));
  state_ = next_state;
  return true;
}

bool PipelineLifecycle::CanAdvanceTo(LifecycleState next_state) const {
  switch (next_state) {
    case LifecycleState::kUninitialized:
      // We can't advance to kUninitialized, as it's the initial state.
      return false;
    case LifecycleState::kInactive:
      return state_ == LifecycleState::kUninitialized;
    case LifecycleState::kInStyleResolve:
      return state_ == LifecycleState::kInactive ||
             state_ == LifecycleState::kUIOpFlush;
    case LifecycleState::kAfterStyleResolve:
      return state_ == LifecycleState::kInStyleResolve;
    case LifecycleState::kInPerformLayout:
      return state_ == LifecycleState::kAfterStyleResolve;
    case LifecycleState::kAfterPerformLayout:
      return state_ == LifecycleState::kInPerformLayout;
    case LifecycleState::kUIOpFlush:
      return state_ == LifecycleState::kAfterPerformLayout;
    case LifecycleState::kStopped:
      // We can advance to kStopped from anywhere.
      return state_ != LifecycleState::kStopped;
    default:
      return false;
  }
}

bool PipelineLifecycle::IsActive() const {
  return state_ > LifecycleState::kInactive &&
         state_ < LifecycleState::kStopped;
}

bool PipelineLifecycle::IsInStyleResolve() const {
  return state_ == LifecycleState::kInStyleResolve;
}

bool PipelineLifecycle::IsInPerformLayout() const {
  return state_ == LifecycleState::kInPerformLayout;
}

bool PipelineLifecycle::IsUIOpFlush() const {
  return state_ == LifecycleState::kUIOpFlush;
}
}  // namespace tasm
}  // namespace lynx
