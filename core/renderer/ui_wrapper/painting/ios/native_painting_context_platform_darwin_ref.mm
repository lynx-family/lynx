// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/ios/native_painting_context_platform_darwin_ref.h"

#include "core/renderer/ui_wrapper/painting/ios/platform_renderer_context_darwin.h"

namespace lynx {
namespace tasm {

NativePaintingCtxPlatformDarwinRef::NativePaintingCtxPlatformDarwinRef(
    std::unique_ptr<PlatformRendererFactory> view_factory)
    : NativePaintingCtxPlatformRef(std::move(view_factory)) {}

void NativePaintingCtxPlatformDarwinRef::GetRootViewLocationOnScreen(float location[2]) {
  if (location == nullptr) {
    return;
  }
  location[0] = 0.f;
  location[1] = 0.f;

  auto* factory = static_cast<PlatformRendererDarwinFactory*>(view_factory_.get());
  if (factory == nullptr) {
    return;
  }
  auto* context = factory->GetContext();
  if (context == nullptr) {
    return;
  }
  const auto res = context->GetRootViewLocationOnScreen();
  location[0] = res.x;
  location[1] = res.y;
}

void NativePaintingCtxPlatformDarwinRef::GetScreenSize(float size[2]) {
  if (size == nullptr) {
    return;
  }
  size[0] = 0.f;
  size[1] = 0.f;

  auto* factory = static_cast<PlatformRendererDarwinFactory*>(view_factory_.get());
  if (factory == nullptr) {
    return;
  }
  auto* context = factory->GetContext();
  if (context == nullptr) {
    return;
  }
  const auto res = context->GetScreenSize();
  size[0] = res.width;
  size[1] = res.height;
}

}  // namespace tasm
}  // namespace lynx
