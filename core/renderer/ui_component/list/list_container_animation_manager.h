// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_COMPONENT_LIST_LIST_CONTAINER_ANIMATION_MANAGER_H_
#define CORE_RENDERER_UI_COMPONENT_LIST_LIST_CONTAINER_ANIMATION_MANAGER_H_

#include <memory>
#include <string>
#include <utility>

#include "base/include/fml/memory/weak_ptr.h"
#include "core/animation/lynx_basic_animator/basic_animator.h"
#include "core/renderer/ui_component/list/item_holder.h"
#include "core/renderer/ui_component/list/list_types.h"

namespace lynx {
namespace tasm {
class ListContainerImpl;
class ListContainerAnimationManager : public ItemHolder::AnimationDelegate {
 public:
  explicit ListContainerAnimationManager(ListContainerImpl* container);

  ~ListContainerAnimationManager();

  list::ListContainerAnimationType AnimationType() const override;

  void DeferredDestroyItemHolder(ItemHolder* holder) override;

  void RecycleItemHolder(ItemHolder* holder) override;

  void UpdateDiffResult(list::ListAdapterDiffResult result);

  bool UpdateAnimation() const;

  void SetUpdateAnimation(std::string update_animation);

  void OnLayoutChildren();

  void EndAnimation();

  void InitializeAnimator();

  void DoAnimationFrame(float progress);

 private:
  bool update_animation_{false};
  list::ListContainerAnimationType animation_type_{
      list::ListContainerAnimationType::kNone};
  std::shared_ptr<animation::basic::LynxBasicAnimator> animator_;
  ListContainerImpl* list_container_impl_;

  fml::WeakPtrFactory<ListContainerAnimationManager> weak_factory_{this};
};
}  // namespace tasm

}  // namespace lynx

#endif  // CORE_RENDERER_UI_COMPONENT_LIST_LIST_CONTAINER_ANIMATION_MANAGER_H_
