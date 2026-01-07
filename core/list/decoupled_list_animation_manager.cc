// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/list/decoupled_list_animation_manager.h"

#include <utility>
#include <vector>

#include "core/list/decoupled_list_container_impl.h"

namespace lynx {
namespace list {

ListAnimationManager::ListAnimationManager(
    ListContainerImpl* list_container_impl)
    : list_container_impl_(list_container_impl) {}

ListAnimationManager::~ListAnimationManager() {
  if (animator_ && animator_->IsRunning()) {
    animator_->Stop();
  }
}

ItemElementDelegate* ListAnimationManager::GetItemElementDelegate(
    ItemHolder* holder) {
  return list_container_impl_->list_adapter()->GetItemElementDelegate(holder);
}

void ListAnimationManager::DeferredDestroyItemHolder(ItemHolder* holder) {
  ListChildrenHelper* list_children_helper =
      list_container_impl_->list_children_helper();
  list_children_helper->AddChild(
      list_children_helper->deferred_destroy_children(), holder);
}

void ListAnimationManager::RecycleItemHolder(ItemHolder* holder) {
  list_container_impl_->list_adapter()->RecycleItemHolder(holder);
}

void ListAnimationManager::UpdateDiffResult(ListAdapterDiffResult result) {
  if (result == ListAdapterDiffResult::kNone) {
    return;
  }
  if (animator_ && animator_->IsRunning()) {
    animator_->Stop();
  }
  if (result ==
      (ListAdapterDiffResult::kRemove | ListAdapterDiffResult::kUpdate)) {
    animation_type_ = ListAnimationType::kRemove;
  } else if (result == (ListAdapterDiffResult::kInsert |
                        ListAdapterDiffResult::kUpdate) ||
             result == ListAdapterDiffResult::kUpdate) {
    animation_type_ = ListAnimationType::kInsert;
  } else {
    animation_type_ = ListAnimationType::kUpdate;
  }
  if (!animator_) {
    InitializeAnimator();
    animator_->RegisterCustomCallback(
        [weak_ptr = WeakFromThis()](float progress) {
          if (auto ptr = weak_ptr.get()) {
            ptr->DoAnimationFrame(progress);
          }
        });
    animator_->RegisterEventCallback(
        [weak_ptr = WeakFromThis()]() {
          if (auto ptr = weak_ptr.get()) {
            ptr->EndAnimation();
          }
        },
        animation::basic::Animation::EventType::End);
  }
  animator_->Start();
}

void ListAnimationManager::InitializeAnimator() {
  starlight::AnimationData data;
  data.duration = 300;
  data.fill_mode = starlight::AnimationFillModeType::kForwards;
  starlight::TimingFunctionData timing_function_data;
  timing_function_data.timing_func = starlight::TimingFunctionType::kEaseIn;
  data.timing_func = timing_function_data;
  // basic animator requires a shared_ptr.
  animator_ =
      std::make_shared<animation::basic::LynxBasicAnimator>(std::move(data));
}

void ListAnimationManager::DoAnimationFrame(float progress) {
  ListChildrenHelper* list_children_helper =
      list_container_impl_->list_children_helper();
  const auto& on_screen_children = list_children_helper->on_screen_children();
  std::vector<fml::WeakPtr<ItemHolder>> on_screen_children_snapshot;
  on_screen_children_snapshot.reserve(on_screen_children.size());
  for (auto* child : on_screen_children) {
    if (child) {
      on_screen_children_snapshot.emplace_back(child->WeakFromThis());
    }
  }
  for (auto& weak_child : on_screen_children_snapshot) {
    if (auto* child = weak_child.get()) {
      child->DoAnimationFrame(progress);
    }
  }
  list_children_helper->ForEachChild(
      list_children_helper->deferred_destroy_children(),
      [progress](ItemHolder* holder) {
        if (holder) {
          holder->DoAnimationFrame(progress);
        }
        return false;
      });
}

void ListAnimationManager::EndAnimation() {
  // 0. reset `animation_type_` first to avoid recursion of item destroy.
  animation_type_ = ListAnimationType::kNone;

  // 1. Need to destroy child after the animation.
  ListChildrenHelper* list_children_helper =
      list_container_impl_->list_children_helper();
  list_children_helper->ForEachChild(
      list_children_helper->deferred_destroy_children(),
      [](ItemHolder* holder) {
        if (holder) {
          holder->EndAnimation();
        }
        return false;
      });
  list_children_helper->ClearDeferredDestroyItemHolder();

  // 2. Because we can't know exactly how many item holders we will create
  // during animation, so we need to save layout infos in advance. Now clean
  list_children_helper->ForEachChild([](ItemHolder* item_holder) {
    if (item_holder) {
      item_holder->EndAnimation();
    }
    return false;
  });
}

void ListAnimationManager::SetUpdateAnimation(bool update_animation) {
  if (update_animation_ && !update_animation && animator_ &&
      animator_->IsRunning()) {
    animator_->Stop();
  }
  update_animation_ = update_animation;
}

}  // namespace list
}  // namespace lynx
