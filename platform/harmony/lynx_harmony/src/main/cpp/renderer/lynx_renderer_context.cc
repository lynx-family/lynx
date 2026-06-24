// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/renderer/lynx_renderer_context.h"

#include <utility>

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/lynx_image_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"

namespace lynx {
namespace tasm {
namespace harmony {

LynxRendererContext::LynxRendererContext(UIOwner* ui_owner)
    : ui_owner_(ui_owner) {}

LynxContext* LynxRendererContext::GetLynxContext() const {
  return ui_owner_ != nullptr ? ui_owner_->Context() : nullptr;
}

void LynxRendererContext::CreateImageManager(int32_t id, const std::string& src,
                                             const ImagePaintInfo& paint_info,
                                             float width, float height,
                                             int32_t event_mask,
                                             bool disable_default_resize) {
  if (ui_owner_ == nullptr || ui_owner_->Context() == nullptr) {
    return;
  }
  auto image_manager = std::make_shared<LynxImageManager>(ui_owner_->Context());
  image_manager->UpdatePaintInfo(paint_info);
  image_managers_[id] = image_manager;
  auto target_it = image_manager_targets_.find(id);
  if (target_it != image_manager_targets_.end()) {
    image_manager->SetTarget(target_it->second);
    if (auto target = target_it->second.lock()) {
      target->Invalidate();
    } else {
      image_manager_targets_.erase(target_it);
    }
  }
  image_manager->RequestImage(id, src, width, height, event_mask);
}

std::shared_ptr<LynxImageManager> LynxRendererContext::GetImageManager(
    int32_t id) const {
  auto it = image_managers_.find(id);
  return it == image_managers_.end() ? nullptr : it->second;
}

void LynxRendererContext::RegisterImageManagerTarget(
    int32_t id, std::weak_ptr<UIBase> target) {
  image_manager_targets_[id] = target;
  auto image_manager = GetImageManager(id);
  if (image_manager != nullptr) {
    image_manager->SetTarget(target);
  }
}

void LynxRendererContext::UnregisterImageManagerTarget(int32_t id) {
  image_manager_targets_.erase(id);
  auto image_manager = GetImageManager(id);
  if (image_manager != nullptr) {
    image_manager->SetTarget({});
  }
}

void LynxRendererContext::UpdatePlatformExtraBundle(
    int32_t id, fml::RefPtr<fml::RefCountedThreadSafeStorage> platform_bundle) {
  if (platform_bundle == nullptr) {
    platform_extra_bundles_.erase(id);
    return;
  }
  platform_extra_bundles_[id] = std::move(platform_bundle);
  auto target_it = platform_extra_bundle_targets_.find(id);
  if (target_it != platform_extra_bundle_targets_.end()) {
    if (auto target = target_it->second.lock()) {
      target->Invalidate();
    } else {
      platform_extra_bundle_targets_.erase(target_it);
    }
  }
}

fml::RefPtr<fml::RefCountedThreadSafeStorage>
LynxRendererContext::TakePlatformExtraBundle(int32_t id) {
  auto it = platform_extra_bundles_.find(id);
  if (it == platform_extra_bundles_.end()) {
    return nullptr;
  }
  auto platform_bundle = std::move(it->second);
  platform_extra_bundles_.erase(it);
  return platform_bundle;
}

void LynxRendererContext::UpdateTextBundle(
    int32_t id, fml::RefPtr<fml::RefCountedThreadSafeStorage> text_bundle) {
  if (text_bundle == nullptr) {
    DestroyTextBundle(id);
    return;
  }
  text_bundles_[id] = std::move(text_bundle);
  auto target_it = platform_extra_bundle_targets_.find(id);
  if (target_it != platform_extra_bundle_targets_.end()) {
    if (auto target = target_it->second.lock()) {
      target->Invalidate();
    } else {
      platform_extra_bundle_targets_.erase(target_it);
    }
  }
}

void LynxRendererContext::DestroyTextBundle(int32_t id) {
  text_bundles_.erase(id);
  auto target_it = platform_extra_bundle_targets_.find(id);
  if (target_it != platform_extra_bundle_targets_.end()) {
    auto target = target_it->second.lock();
    platform_extra_bundle_targets_.erase(target_it);
    if (target != nullptr) {
      target->Invalidate();
    }
  }
}

fml::RefPtr<fml::RefCountedThreadSafeStorage>
LynxRendererContext::GetTextBundle(int32_t id) const {
  auto it = text_bundles_.find(id);
  return it == text_bundles_.end() ? nullptr : it->second;
}

void LynxRendererContext::RegisterPlatformExtraBundleTarget(
    int32_t id, std::weak_ptr<UIBase> target) {
  platform_extra_bundle_targets_[id] = std::move(target);
}

void LynxRendererContext::UnregisterPlatformExtraBundleTarget(int32_t id) {
  platform_extra_bundle_targets_.erase(id);
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
