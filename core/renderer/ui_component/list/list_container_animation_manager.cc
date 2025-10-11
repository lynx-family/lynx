// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_component/list/list_container_animation_manager.h"

#include <memory>
#include <utility>

#include "core/renderer/ui_component/list/list_container_impl.h"

namespace lynx {
namespace tasm {

ListContainerAnimationManager::ListContainerAnimationManager(
    ListContainerImpl* container)
    : list_container_impl_(container) {}

ListContainerAnimationManager::~ListContainerAnimationManager() {
  if (animator_ && animator_->IsRunning()) {
    animator_->Stop();
  }
}

bool ListContainerAnimationManager::UpdateAnimation() const {
  return update_animation_.value_or(false);
}

void ListContainerAnimationManager::DeferredDestroyItemHolder(
    ItemHolder* holder) {
  list_container_impl_->list_children_helper_->AddDeferredDestroyItemHolder(
      holder);
}

void ListContainerAnimationManager::RecycleItemHolder(ItemHolder* holder) {
  list_container_impl_->list_adapter_->RecycleItemHolder(holder);
}

void ListContainerAnimationManager::UpdateDiffResult(
    list::ListAdapterDiffResult result) {
  if (result == (list::ListAdapterDiffResult::kRemove |
                 list::ListAdapterDiffResult::kUpdate)) {
    animation_type_ = list::ListContainerAnimationType::kRemove;
  } else if (result == (list::ListAdapterDiffResult::kInsert |
                        list::ListAdapterDiffResult::kUpdate) ||
             result == list::ListAdapterDiffResult::kUpdate) {
    animation_type_ = list::ListContainerAnimationType::kInsert;
  } else if (result != list::ListAdapterDiffResult::kNone) {
    animation_type_ = list::ListContainerAnimationType::kUpdate;
  } else {
    return;
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

void ListContainerAnimationManager::InitializeAnimator() {
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

void ListContainerAnimationManager::DoAnimationFrame(float progress) {
  for (auto& it :
       list_container_impl_->list_children_helper_->on_screen_children()) {
    if (it) {
      it->DoAnimationFrame(progress);
    }
  }
  list_container_impl_->list_children_helper_
      ->TraverseDeferredDestroyItemHolder([=](ItemHolder* holder) {
        if (holder) {
          holder->DoAnimationFrame(progress);
        }
      });
}

list::ListContainerAnimationType ListContainerAnimationManager::AnimationType()
    const {
  return animation_type_;
}

void ListContainerAnimationManager::EndAnimation() {
  // 0. reset `animation_type_` first to avoid recursion of item destroy.
  animation_type_ = list::ListContainerAnimationType::kNone;

  // 1. Need to destroy child after the animation.
  list_container_impl_->list_children_helper_
      ->TraverseDeferredDestroyItemHolder(
          [=](ItemHolder* holder) { holder->EndAnimation(); });
  list_container_impl_->list_children_helper_
      ->DestroyDeferredDestroyItemHolder();

  // 2. Because we can't know exactly how many item holders we will create
  // during animation, so we need to save layout infos in advance. Now clean
  list_container_impl_->list_children_helper_->ForEachChild(
      [](ItemHolder* item_holder) {
        if (item_holder) {
          item_holder->EndAnimation();
        }
        return false;
      });
}

void ListContainerAnimationManager::SetUpdateAnimation(bool update_animation) {
  if (!update_animation_.has_value()) {
    update_animation_ = update_animation;
  } else {
    list_container_impl_->list_adapter_->OnErrorOccurred(base::LynxError(
        lynx::error::E_COMPONENT_LIST_SET_UPDATE_ANIMATION_MULTIPLE_TIMES,
        "Update-animation cannot be set multiple times."));
  }
}

}  // namespace tasm

}  // namespace lynx
