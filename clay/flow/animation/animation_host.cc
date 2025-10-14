// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/flow/animation/animation_host.h"

#include <utility>

namespace clay {

AnimationHost::AnimationHost() {}

AnimationHost::~AnimationHost() { animation_mutators_.clear(); }

bool AnimationHost::HasAnimations() const {
  return !retained_layer_ids_.empty() || !animation_mutators_.empty();
}

bool AnimationHost::HasAnimationRunning(uint64_t layer_id) const {
  if (auto search = animation_mutators_.find(layer_id);
      search != animation_mutators_.end()) {
    return search->second->HasAnimationRunning();
  }
  return false;
}

bool AnimationHost::HasAnimation(uint64_t layer_id) const {
  auto search = animation_mutators_.find(layer_id);
  return search != animation_mutators_.end();
}

bool AnimationHost::DoAnimationFrame(int64_t frame_time) {
  bool stopped = true;
  for (auto& mutator : animation_mutators_) {
    if (mutator.second) {
      stopped &= mutator.second->DoAnimationFrame(frame_time);
    }
  }
  return stopped;
}

void AnimationHost::AddAnimationMutator(
    Layer* animation_layer, const std::shared_ptr<AnimationMutator> mutator) {
  animation_layer->SetAnimationHost(shared_from_this());
  uint64_t layer_id = animation_layer->unique_id();
  animation_mutators_.emplace(std::make_pair(layer_id, std::move(mutator)));
}

const std::shared_ptr<AnimationMutator>& AnimationHost::GetAnimationMutator(
    uint64_t layer_id) const {
  auto& mutator = animation_mutators_.at(layer_id);
  FML_DCHECK(layer_id == mutator->layer_id());
  return mutator;
}

void AnimationHost::SetServiceManager(
    std::shared_ptr<clay::ServiceManager> service_manager) {
  for (auto& [layer_id, mutator] : animation_mutators_) {
    mutator->SetServiceManager(service_manager);
  }
}

void AnimationHost::ResetServiceManager() {
  for (auto& [layer_id, mutator] : animation_mutators_) {
    mutator->ResetServiceManager();
  }
}

void AnimationHost::AddRetainedLayerId(uint64_t layer_id) {
  retained_layer_ids_.insert(layer_id);
}

void AnimationHost::MergeAnimations(const AnimationHost* prev_animation_host) {
  if (!prev_animation_host) {
    return;
  }

  // Merge animations for the new layer.
  auto prev_mutators = prev_animation_host->animation_mutators_;
  for (auto& [layer_id, mutator] : animation_mutators_) {
    if (mutator->GetType() == AnimationMutatorType::kPicture) {
      for (const auto& [prev_layer_id, prev_mutator] : prev_mutators) {
        if (prev_mutator->GetType() == AnimationMutatorType::kPicture &&
            mutator->HasSameElementId(prev_mutator)) {
          mutator->SyncProperties(prev_mutator);
        }
      }
    } else {
      if (auto search = prev_mutators.find(layer_id);
          search != prev_mutators.end()) {
        mutator->SyncProperties(search->second);
      }
    }
  }

  // Merge animations for the retained layer.
  std::set<uint64_t> layer_ids;
  layer_ids.swap(retained_layer_ids_);
  for (auto id : layer_ids) {
    if (auto search = prev_mutators.find(id); search != prev_mutators.end()) {
      if (search->second && search->second->HasAnimationRunning()) {
        animation_mutators_.emplace(std::make_pair(id, search->second));
      }
    }
  }
}

}  // namespace clay
