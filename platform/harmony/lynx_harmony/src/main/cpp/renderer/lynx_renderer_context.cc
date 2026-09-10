// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/renderer/lynx_renderer_context.h"

#include <utility>

#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/text/paragraph_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/lynx_image_manager.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/ui_base.h"

namespace lynx {
namespace tasm {
namespace harmony {

LynxRendererContext::LynxRendererContext(std::weak_ptr<LynxContext> context)
    : context_(std::move(context)) {}

LynxRendererContext::~LynxRendererContext() = default;

std::shared_ptr<LynxContext> LynxRendererContext::GetLynxContext() const {
  return context_.lock();
}

UIOwner* LynxRendererContext::GetUIOwner() const {
  auto context = context_.lock();
  return context != nullptr ? context->GetUIOwner() : nullptr;
}

void LynxRendererContext::CreateImageManager(int32_t id, const std::string& src,
                                             const ImagePaintInfo& paint_info,
                                             float width, float height,
                                             int32_t event_mask,
                                             int32_t image_key) {
  auto context = GetLynxContext();
  if (context == nullptr) {
    return;
  }
  auto image_manager = std::make_shared<LynxImageManager>(context);
  image_manager->UpdatePaintInfo(paint_info);
  image_managers_[image_key] = image_manager;
  image_manager->RequestImage(id, src, width, height, event_mask);
}

std::shared_ptr<LynxImageManager> LynxRendererContext::GetImageManager(
    int32_t image_key) const {
  auto it = image_managers_.find(image_key);
  return it == image_managers_.end() ? nullptr : it->second;
}

void LynxRendererContext::DestroyImageManager(int32_t image_key) {
  image_managers_.erase(image_key);
}

void LynxRendererContext::UpdateTextBundle(
    int32_t id, fml::RefPtr<ParagraphHarmony> text_bundle) {
  std::lock_guard<std::mutex> lock(text_bundles_mutex_);
  if (text_bundle == nullptr) {
    text_bundles_.erase(id);
    return;
  }
  text_bundles_.insert_or_assign(id, std::move(text_bundle));
}

void LynxRendererContext::DestroyTextBundle(int32_t id) {
  std::lock_guard<std::mutex> lock(text_bundles_mutex_);
  text_bundles_.erase(id);
}

fml::RefPtr<ParagraphHarmony> LynxRendererContext::GetTextBundle(
    int32_t id) const {
  std::lock_guard<std::mutex> lock(text_bundles_mutex_);
  auto it = text_bundles_.find(id);
  return it == text_bundles_.end() ? nullptr : it->second;
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
