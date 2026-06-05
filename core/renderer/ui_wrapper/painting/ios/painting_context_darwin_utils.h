// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PAINTING_CONTEXT_DARWIN_UTILS_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PAINTING_CONTEXT_DARWIN_UTILS_H_

#include <string>

#include "base/include/debug/backtrace.h"
#include "base/include/debug/lynx_error.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/renderer/ui_wrapper/painting/ios/painting_context_darwin_utils.h"

#import <Lynx/LynxCallStackUtil.h>
#import <Lynx/LynxPerformanceController.h>

namespace lynx {
namespace tasm {

class PaintingCtxPlatformRef;

class PaintingContextDarwinUtils {
 public:
  template <typename F>
  static void ExecuteSafely(const F& func);

  static void SetPerformanceController(PaintingCtxPlatformRef* platform_ref,
                                       LynxPerformanceController* performance_controller);
};

template <typename F>
void PaintingContextDarwinUtils::ExecuteSafely(const F& func) {
  @try {
    func();
  } @catch (NSException* e) {
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
          instance.AddCustomInfoToError([key UTF8String],
                                        [[customInfo objectForKey:key] UTF8String]);
        }
      }
    }
  }
}

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PAINTING_CONTEXT_DARWIN_UTILS_H_
