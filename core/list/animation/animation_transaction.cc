// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/list/animation/animation_transaction.h"

#include <utility>

#include "core/list/decoupled_item_holder.h"

namespace lynx {
namespace list {

AnimationTransaction::AnimationTransaction(
    TransactionId id, std::vector<WeakAnimationTarget> pre_targets,
    std::unique_ptr<ItemAnimator> item_animator)
    : id_(id),
      state_(TransactionState::kHasPreChildrenSnapshot),
      pre_targets_(std::move(pre_targets)),
      recorder_(std::make_unique<ItemAnimationRecorder>()),
      item_animator_(std::move(item_animator)) {}

AnimationTransaction::~AnimationTransaction() = default;

std::vector<WeakAnimationTarget> AnimationTransaction::TakePreTargets() {
  std::vector<WeakAnimationTarget> pre_targets;
  pre_targets.swap(pre_targets_);
  return pre_targets;
}

void AnimationTransaction::SetDeferredItemHolders(
    std::vector<std::unique_ptr<ItemHolder>> item_holders) {
  deferred_item_holders_ = std::move(item_holders);
}

std::vector<std::unique_ptr<ItemHolder>>
AnimationTransaction::TakeDeferredItemHolders() {
  std::vector<std::unique_ptr<ItemHolder>> deferred_item_holders;
  deferred_item_holders.swap(deferred_item_holders_);
  return deferred_item_holders;
}

}  // namespace list
}  // namespace lynx
