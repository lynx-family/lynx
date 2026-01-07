// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_LIST_ANIMATION_MANAGER_DEFAULT_H_
#define CORE_LIST_LIST_ANIMATION_MANAGER_DEFAULT_H_

#include "core/list/list_animation_manager.h"

namespace lynx {
namespace list {

class ListAnimationManagerDefault : public ListAnimationManager {
 public:
  ListAnimationManagerDefault(ListContainerImpl* list_container_impl);
  ~ListAnimationManagerDefault() override = default;

  void UpdateDiffResult(ListAdapterDiffResult result) override {}

  void SetUpdateAnimation(bool update_animation) override {}
};

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_LIST_ANIMATION_MANAGER_DEFAULT_H_
