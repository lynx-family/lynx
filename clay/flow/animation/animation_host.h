// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FLOW_ANIMATION_ANIMATION_HOST_H_
#define CLAY_FLOW_ANIMATION_ANIMATION_HOST_H_

#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

#include "clay/common/element_id.h"
#include "clay/common/service/service.h"
#include "clay/flow/animation/animation_mutator.h"
#include "clay/flow/layers/layer.h"

namespace clay {

class AnimationMutator;

class AnimationHost : public std::enable_shared_from_this<AnimationHost> {
 public:
  AnimationHost();
  ~AnimationHost();

  bool HasAnimations() const;
  bool HasAnimation(uint64_t layer_id) const;
  bool HasAnimationRunning(uint64_t layer_id) const;

  bool DoAnimationFrame(int64_t frame_time);

  void AddAnimationMutator(Layer* animation_layer,
                           const std::shared_ptr<AnimationMutator> mutator);
  const std::shared_ptr<AnimationMutator>& GetAnimationMutator(
      uint64_t layer_id) const;

  void SetServiceManager(std::shared_ptr<clay::ServiceManager> service_manager);
  void ResetServiceManager();

  void AddRetainedLayerId(uint64_t layer_id);
  void MergeAnimations(const AnimationHost* prev_animation_host);

 private:
  // Support reusing layer cross LayerTrees.
  std::set<uint64_t> retained_layer_ids_;
  // Stores all animation mutators for the current LayerTree.
  AnimationMutators animation_mutators_;
};

}  // namespace clay

#endif  // CLAY_FLOW_ANIMATION_ANIMATION_HOST_H_
