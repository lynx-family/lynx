// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/ios/native_painting_context_platform_darwin_ref.h"

namespace lynx {
namespace tasm {

NativePaintingCtxPlatformDarwinRef::NativePaintingCtxPlatformDarwinRef(
    std::unique_ptr<PlatformRendererFactory> view_factory)
    : NativePaintingCtxPlatformRef(std::move(view_factory)) {}

}  // namespace tasm
}  // namespace lynx
