// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/harmony/native_painting_context_harmony.h"

#include "core/renderer/ui_wrapper/painting/harmony/native_painting_context_platform_harmony_ref.h"
#include "core/renderer/ui_wrapper/painting/harmony/platform_renderer_harmony.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/renderer/lynx_renderer_context.h"

namespace lynx {
namespace tasm {

NativePaintingCtxHarmony::NativePaintingCtxHarmony() = default;

NativePaintingCtxHarmony::NativePaintingCtxHarmony(
    const std::shared_ptr<harmony::LynxContext>& context) {
  if (context == nullptr) {
    return;
  }
  renderer_context_ = std::make_shared<harmony::LynxRendererContext>(context);
  platform_ref_ = std::make_shared<NativePaintingCtxPlatformHarmonyRef>(
      std::make_unique<PlatformRendererHarmonyFactory>(renderer_context_));
}

NativePaintingCtxHarmony::~NativePaintingCtxHarmony() {
  if (auto ref = std::static_pointer_cast<NativePaintingCtxPlatformRef>(
          platform_ref_)) {
    ref->Destroy();
  }
  platform_ref_.reset();
  renderer_context_.reset();
}

std::unique_ptr<pub::Value> NativePaintingCtxHarmony::GetTextInfo(
    const std::string& content, const pub::Value& info) {
  return nullptr;
}

std::vector<float> NativePaintingCtxHarmony::getBoundingClientOrigin(int id) {
  return {};
}

std::vector<float> NativePaintingCtxHarmony::getWindowSize(int id) {
  return {};
}

std::vector<float> NativePaintingCtxHarmony::GetRectToWindow(int id) {
  return {};
}

std::vector<float> NativePaintingCtxHarmony::GetRectToLynxView(int64_t id) {
  return {};
}

std::vector<float> NativePaintingCtxHarmony::ScrollBy(int64_t id, float width,
                                                      float height) {
  return {};
}

void NativePaintingCtxHarmony::Invoke(
    int64_t id, const std::string& method, const pub::Value& params,
    const std::function<void(int32_t, const pub::Value&)>& callback) {}

void NativePaintingCtxHarmony::EnqueueInvoke(
    int64_t id, const std::string& method, const pub::Value& params,
    const std::function<void(int32_t, const pub::Value&)>& callback) {}

int32_t NativePaintingCtxHarmony::GetTagInfo(const std::string& tag_name) {
  return 0;
}

bool NativePaintingCtxHarmony::IsFlatten(
    base::MoveOnlyClosure<bool, bool> func) {
  return false;
}

bool NativePaintingCtxHarmony::NeedAnimationProps() { return false; }

void NativePaintingCtxHarmony::CreatePlatformRenderer(
    int id, PlatformRendererType type, const fml::RefPtr<PropBundle>& init_data,
    const PlatformRendererInitConfig& init_config) {}

void NativePaintingCtxHarmony::CreatePlatformExtendedRenderer(
    int id, const base::String& tag_name,
    const fml::RefPtr<PropBundle>& init_data,
    const PlatformRendererInitConfig& init_config) {}

void NativePaintingCtxHarmony::EnqueueDisplayList(int id, DisplayList list) {}

fml::RefPtr<PaintImage> NativePaintingCtxHarmony::CreateImage(
    int id, base::String src, const ImagePaintInfo& paint_info, float width,
    float height, int32_t event_mask, bool disable_default_resize) {
  return nullptr;
}

void NativePaintingCtxHarmony::UpdatePlatformEventBundle(
    int id, PlatformEventBundle bundle) {}

}  // namespace tasm
}  // namespace lynx
