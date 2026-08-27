// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/harmony/native_painting_context_harmony.h"

namespace lynx::tasm {

NativePaintingCtxHarmony::NativePaintingCtxHarmony() = default;

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

}  // namespace lynx::tasm
