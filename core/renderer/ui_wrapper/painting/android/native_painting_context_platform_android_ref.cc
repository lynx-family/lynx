// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/android/native_painting_context_platform_android_ref.h"

#include <utility>

namespace lynx {
namespace tasm {

NativePaintingCtxAndroidRef::NativePaintingCtxAndroidRef(
    std::unique_ptr<PlatformRendererFactory> view_factory)
    : NativePaintingCtxPlatformRef(std::move(view_factory)) {}

void NativePaintingCtxAndroidRef::GetRootViewLocationOnScreen(
    float location[2]) {
  const auto res =
      static_cast<PlatformRendererAndroidFactory*>(view_factory_.get())
          ->GetContext()
          ->GetRootViewLocationOnScreen();
  location[0] = res[0];
  location[1] = res[1];
}

void NativePaintingCtxAndroidRef::GetScreenSize(float size[2]) {
  const auto res =
      static_cast<PlatformRendererAndroidFactory*>(view_factory_.get())
          ->GetContext()
          ->GetScreenSize();
  size[0] = res[0];
  size[1] = res[1];
}

}  // namespace tasm
}  // namespace lynx
