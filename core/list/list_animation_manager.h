// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_LIST_ANIMATION_MANAGER_H_
#define CORE_LIST_LIST_ANIMATION_MANAGER_H_

#include <memory>

#include "core/list/decoupled_item_holder.h"
#include "core/list/decoupled_list_types.h"

namespace lynx {
namespace list {

class ListContainerImpl;

class ListAnimationManager : public ItemHolder::AnimationDelegate {
 public:
  ListAnimationManager(ListContainerImpl* list_container_impl);

  ~ListAnimationManager() override = default;

  virtual void UpdateDiffResult(ListAdapterDiffResult result) = 0;

  virtual void SetUpdateAnimation(bool update_animation) = 0;

  ListAnimationType AnimationType() const override { return animation_type_; }

  ItemElementDelegate* GetItemElementDelegate(ItemHolder* item_holder) override;

  void DeferredDestroyItemHolder(ItemHolder* item_holder) override;

  void RecycleItemHolder(ItemHolder* item_holder) override;

  bool UpdateAnimation() const override { return update_animation_; }

 protected:
  bool update_animation_{false};
  ListAnimationType animation_type_{ListAnimationType::kNone};
  ListContainerImpl* list_container_impl_{nullptr};
};

std::unique_ptr<ListAnimationManager> CreateListAnimationManager(
    ListContainerImpl* list_container_impl);

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_LIST_ANIMATION_MANAGER_H_
