// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_LIST_ANIMATION_MANAGER_IMPL_H_
#define CORE_LIST_LIST_ANIMATION_MANAGER_IMPL_H_

#include <memory>

#include "base/include/fml/memory/weak_ptr.h"
#include "core/animation/lynx_basic_animator/basic_animator.h"
#include "core/list/list_animation_manager.h"

namespace lynx {
namespace list {

class ListAnimationManagerImpl
    : public fml::EnableWeakFromThis<ListAnimationManagerImpl>,
      public ListAnimationManager {
 public:
  ListAnimationManagerImpl(ListContainerImpl* list_container_impl);

  ~ListAnimationManagerImpl() override;

  void UpdateDiffResult(ListAdapterDiffResult result) override;

  void SetUpdateAnimation(bool update_animation) override;

 private:
  void InitializeAnimator();

  void InitializeAnimatorEvent();

  void DoAnimationFrame(float progress);

  void EndAnimation();

 private:
  std::shared_ptr<animation::basic::LynxBasicAnimator> animator_{nullptr};
};

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_LIST_ANIMATION_MANAGER_IMPL_H_
