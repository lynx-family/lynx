// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_DECOUPLED_LIST_ANIMATION_MANAGER_H_
#define CORE_LIST_DECOUPLED_LIST_ANIMATION_MANAGER_H_

#include <memory>

#include "base/include/fml/memory/weak_ptr.h"
#include "core/animation/lynx_basic_animator/basic_animator.h"
#include "core/list/decoupled_item_holder.h"
#include "core/list/decoupled_list_types.h"

namespace lynx {
namespace list {

class ListContainerImpl;

class ListAnimationManager
    : public fml::EnableWeakFromThis<ListAnimationManager>,
      public ItemHolder::AnimationDelegate {
 public:
  explicit ListAnimationManager(ListContainerImpl* container);

  ~ListAnimationManager() override;

  ListAnimationType AnimationType() const override { return animation_type_; }

  ItemElementDelegate* GetItemElementDelegate(ItemHolder* holder) override;

  void DeferredDestroyItemHolder(ItemHolder* holder) override;

  void RecycleItemHolder(ItemHolder* holder) override;

  bool UpdateAnimation() const override { return update_animation_; }

  void UpdateDiffResult(ListAdapterDiffResult result);

  void SetUpdateAnimation(bool update_animation);

  void DoAnimationFrame(float progress);

 private:
  void InitializeAnimator();

  void EndAnimation();

 private:
  bool update_animation_{false};
  ListContainerImpl* list_container_impl_{nullptr};
  ListAnimationType animation_type_{ListAnimationType::kNone};
  std::shared_ptr<animation::basic::LynxBasicAnimator> animator_;
};

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_DECOUPLED_LIST_ANIMATION_MANAGER_H_
