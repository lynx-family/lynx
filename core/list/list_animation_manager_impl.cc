// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/list/list_animation_manager_impl.h"

#include <utility>
#include <vector>

#include "core/list/decoupled_list_container_impl.h"

namespace lynx {
namespace list {

ListAnimationManagerImpl::ListAnimationManagerImpl(
    ListContainerImpl* list_container_impl)
    : ListAnimationManager(list_container_impl) {}

ListAnimationManagerImpl::~ListAnimationManagerImpl() {
  if (animator_ && animator_->IsRunning()) {
    animator_->Stop();
  }
}

void ListAnimationManagerImpl::UpdateDiffResult(ListAdapterDiffResult result) {
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
    InitializeAnimatorEvent();
  } else if (animator_->IsRunning()) {
    // TODO(dongjiajian): using lynx animator instead of the basic animator.
    // the basic animator won't handle the case of the repeat `start()`.
    // The destroy of the animation will trigger the cancel event of the
    // animation.
    animator_->DestroyAnimation();
    InitializeAnimator();
    InitializeAnimatorEvent();
  }
  animator_->Start();
}

void ListAnimationManagerImpl::SetUpdateAnimation(bool update_animation) {
  if (update_animation_ && !update_animation && animator_ &&
      animator_->IsRunning()) {
    animator_->Stop();
  }
  update_animation_ = update_animation;
}

void ListAnimationManagerImpl::InitializeAnimator() {
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

void ListAnimationManagerImpl::InitializeAnimatorEvent() {
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
  // The destroy of the animation will trigger the cancel event of the
  // animation.
  animator_->RegisterEventCallback(
      [weak_ptr = WeakFromThis()]() {
        if (auto ptr = weak_ptr.get()) {
          ptr->EndAnimation();
        }
      },
      animation::basic::Animation::EventType::Cancel);
}

void ListAnimationManagerImpl::DoAnimationFrame(float progress) {
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
    if (auto child = weak_child.get()) {
      child->DoAnimationFrame(progress);
    }
  }
  list_children_helper->ForEachChild(
      list_children_helper->deferred_destroy_children(),
      [progress](ItemHolder* item_holder) {
        item_holder->DoAnimationFrame(progress);
        return false;
      });
}

void ListAnimationManagerImpl::EndAnimation() {
  // 0. reset `animation_type_` first to avoid recursion of item destroy.
  animation_type_ = ListAnimationType::kNone;

  // 1. Need to destroy child after the animation.
  ListChildrenHelper* list_children_helper =
      list_container_impl_->list_children_helper();
  list_children_helper->ForEachChild(
      list_children_helper->deferred_destroy_children(),
      [](ItemHolder* item_holder) {
        item_holder->EndAnimation();
        return false;
      });
  list_children_helper->ClearDeferredDestroyItemHolder();

  // 2. Because we can't know exactly how many item holders we will create
  // during animation, so we need to save layout infos in advance. Now clean
  list_children_helper->ForEachChild([](ItemHolder* item_holder) {
    item_holder->EndAnimation();
    return false;
  });
}

std::unique_ptr<ListAnimationManager> CreateListAnimationManager(
    ListContainerImpl* list_container_impl) {
  return std::make_unique<ListAnimationManagerImpl>(list_container_impl);
}

}  // namespace list
}  // namespace lynx
