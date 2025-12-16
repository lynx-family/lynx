// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/list/decoupled_list_container_animation_manager.h"

#include "core/list/decoupled_list_container_impl.h"

namespace lynx {
namespace list {

ListContainerAnimationManager::ListContainerAnimationManager(
    ListContainerImpl* list_container) {}
//    : list_container_impl_(list_container) {}

ListContainerAnimationManager::~ListContainerAnimationManager() {
  //  if (animator_ && animator_->IsRunning()) {
  //    animator_->Stop();
  //  }
}

}  // namespace list
}  // namespace lynx
