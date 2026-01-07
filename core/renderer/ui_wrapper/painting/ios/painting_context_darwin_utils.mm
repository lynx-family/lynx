// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/ios/painting_context_darwin_utils.h"

#include "core/renderer/ui_wrapper/painting/ios/native_painting_context_platform_darwin_ref.h"
#include "core/renderer/ui_wrapper/painting/ios/painting_context_darwin.h"

namespace lynx {
namespace tasm {

void PaintingContextDarwinUtils::SetPerformanceController(
    PaintingCtxPlatformRef* platform_ref, LynxPerformanceController* performance_controller) {
  if (platform_ref == nullptr) {
    return;
  }

  if (platform_ref->IsNativePaintingCtxPlatformRef()) {
    static_cast<NativePaintingCtxPlatformDarwinRef*>(platform_ref)
        ->SetPerformanceController(performance_controller);
    return;
  }

  static_cast<PaintingContextDarwinRef*>(platform_ref)
      ->SetPerformanceController(performance_controller);
}

}  // namespace tasm
}  // namespace lynx
