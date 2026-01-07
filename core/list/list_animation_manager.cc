// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/list/list_animation_manager.h"

#include "core/list/decoupled_list_container_impl.h"

namespace lynx {
namespace list {

ListAnimationManager::ListAnimationManager(
    ListContainerImpl* list_container_impl)
    : list_container_impl_(list_container_impl) {}

ItemElementDelegate* ListAnimationManager::GetItemElementDelegate(
    ItemHolder* item_holder) {
  return list_container_impl_->list_adapter()->GetItemElementDelegate(
      item_holder);
}

void ListAnimationManager::DeferredDestroyItemHolder(ItemHolder* item_holder) {
  ListChildrenHelper* list_children_helper =
      list_container_impl_->list_children_helper();
  list_children_helper->AddChild(
      list_children_helper->deferred_destroy_children(), item_holder);
}

void ListAnimationManager::RecycleItemHolder(ItemHolder* item_holder) {
  list_container_impl_->list_adapter()->RecycleItemHolder(item_holder);
}

}  // namespace list
}  // namespace lynx
