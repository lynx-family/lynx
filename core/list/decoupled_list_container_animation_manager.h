// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_DECOUPLED_LIST_CONTAINER_ANIMATION_MANAGER_H_
#define CORE_LIST_DECOUPLED_LIST_CONTAINER_ANIMATION_MANAGER_H_

#include "core/list/decoupled_item_holder.h"

namespace lynx {
namespace list {

class ListContainerImpl;

class ListContainerAnimationManager
    : public fml::EnableWeakFromThis<ListContainerAnimationManager>,
      public ItemHolder::AnimationDelegate {
 public:
  explicit ListContainerAnimationManager(ListContainerImpl* container);

  ~ListContainerAnimationManager() override;

  ListContainerAnimationType AnimationType() const override {
    return animation_type_;
  }

  void DeferredDestroyItemHolder(ItemHolder* holder) override {}

  void RecycleItemHolder(ItemHolder* holder) override {}

  bool UpdateAnimation() const override { return update_animation_; }

  void UpdateDiffResult(ListAdapterDiffResult result) {}

  void SetUpdateAnimation(bool update_animation) {}

  void EndAnimation() {}

  void DoAnimationFrame(float progress) {}

 private:
  void InitializeAnimator() {}

 private:
  bool update_animation_{false};
  // ListContainerImpl* list_container_impl_{nullptr};
  ListContainerAnimationType animation_type_{ListContainerAnimationType::kNone};
  // std::shared_ptr<animation::basic::LynxBasicAnimator> animator_;
};

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_DECOUPLED_LIST_CONTAINER_ANIMATION_MANAGER_H_
