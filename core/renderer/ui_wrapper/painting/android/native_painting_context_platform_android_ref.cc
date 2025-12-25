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

}  // namespace tasm
}  // namespace lynx
