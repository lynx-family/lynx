// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_ANIMATION_ANIMATION_TYPES_H_
#define CORE_LIST_ANIMATION_ANIMATION_TYPES_H_

#include <cstdint>

#include "base/include/float_comparison.h"

namespace lynx {
namespace list {

using TransactionId = uint64_t;
using AnimationId = uint64_t;
constexpr TransactionId kInvalidTransactionId = 0;
// Used only as a key in the running-animation map. Asynchronous callbacks must
// not cast it back to a pointer and dereference it. Each transaction owns its
// ItemAnimator, and animation_id validation protects against address reuse
// within the same animator.
using AnimationTargetKey = uintptr_t;

constexpr int32_t kDefaultAddAnimationDurationMs = 120;
constexpr int32_t kDefaultRemoveAnimationDurationMs = 120;
constexpr int32_t kDefaultMoveAnimationDurationMs = 250;
constexpr int32_t kDefaultChangeAnimationDurationMs = 250;

// Runtime configuration for List update animations. Duration values are
// expressed in milliseconds.
struct UpdateAnimationConfig {
  bool enable{false};
  int32_t add_duration_ms{kDefaultAddAnimationDurationMs};
  int32_t remove_duration_ms{kDefaultRemoveAnimationDurationMs};
  int32_t move_duration_ms{kDefaultMoveAnimationDurationMs};
};

// A geometric snapshot of an AnimationTarget at a particular layout stage.
class ItemLayoutInfo {
 public:
  bool PositionChanged(const ItemLayoutInfo& other) const {
    return base::FloatsNotEqual(left_, other.left_) ||
           base::FloatsNotEqual(top_, other.top_);
  }

  bool BoundsChanged(const ItemLayoutInfo& other) const {
    return base::FloatsNotEqual(left_, other.left_) ||
           base::FloatsNotEqual(top_, other.top_) ||
           base::FloatsNotEqual(right_, other.right_) ||
           base::FloatsNotEqual(bottom_, other.bottom_);
  }

  float left_{0.f};
  float top_{0.f};
  float right_{0.f};
  float bottom_{0.f};
};

// The pre-layout and post-layout snapshots for one target in an animation
// transaction. Records are keyed by target address; predictive layout and
// matching distinct old and new target objects are not supported.
class ItemAnimationRecord {
 public:
  enum Flag : uint8_t {
    kNone = 0,
    kPre = 1 << 0,
    kPost = 1 << 1,
    kChanged = 1 << 2,
  };

  bool HasFlag(Flag flag) const {
    return (flags & static_cast<uint8_t>(flag)) != 0;
  }

  void AddFlag(Flag flag) { flags |= static_cast<uint8_t>(flag); }

  uint8_t flags{kNone};
  ItemLayoutInfo pre_info;
  ItemLayoutInfo post_info;
};

using ItemAnimationRecordFlag = ItemAnimationRecord::Flag;

enum class ItemAnimationType {
  kAppearance,
  kDisappearance,
  kPersistence,
  kChange,
};

enum class TransactionState {
  // The target snapshot from before the data update has been captured, but
  // layout has not started. An empty snapshot is also valid.
  kHasPreChildrenSnapshot,
  kPreLayoutInfoRecorded,
  kPostLayoutInfoRecorded,
  kHasPrepared,
  // AfterFlush() has entered the animation startup stage. This state is set
  // before RunPendingAnimations() to prevent a reentrant or repeated
  // AfterFlush() call from starting the same batch again.
  kRunning,
  kCancel,
};

// Identifies why a transaction was cancelled. kManagerCleared uses the
// animator's destroy mode and releases deferred holders with the transaction.
// Other reasons use normal cancellation and return deferred holders to the
// adapter for recycling.
enum class AnimationCancelReason {
  kNewDataUpdate,
  kLayoutInvalidated,
  // Used when disabling update animations cancels the active transaction.
  kAnimationDisabled,
  kManagerCleared,
};

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_ANIMATION_ANIMATION_TYPES_H_
