// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/ios/painting_context_darwin_utils.h"

#include "base/include/base_defines.h"

#include "core/renderer/ui_wrapper/painting/ios/native_painting_context_platform_darwin_ref.h"
#include "core/renderer/ui_wrapper/painting/ios/painting_context_darwin.h"

namespace lynx {
namespace tasm {

// Keep error reporting out of every queued operation's template instance.
BASE_NEVER_INLINE void PaintingContextDarwinUtils::ReportException(NSException* e) {
  std::string msg = [[NSString stringWithFormat:@"%@:%@", [e name], [e reason]] UTF8String];
  if (LYNX_ERROR(error::E_EXCEPTION_PLATFORM, msg, "")) {
    auto& instance = lynx::base::ErrorStorage::GetInstance();
    std::string stack;
    NSString* rawStack = [LynxCallStackUtil getCallStack:e];
    stack = lynx::base::debug::GetBacktraceInfo(stack);
    instance.AddCustomInfoToError("error_stack", stack);
    if (rawStack) {
      instance.AddCustomInfoToError("raw_stack", rawStack.UTF8String);
    }
    NSDictionary* info = [e userInfo];
    NSDictionary* customInfo = [info objectForKey:@"LynxErrorCustomInfo"];
    if (customInfo) {
      for (NSString* key in customInfo) {
        instance.AddCustomInfoToError([key UTF8String], [[customInfo objectForKey:key] UTF8String]);
      }
    }
  }
}

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
