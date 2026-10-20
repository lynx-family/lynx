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
  static void ExecuteSafely(F&& func);

  static void SetPerformanceController(PaintingCtxPlatformRef* platform_ref,
                                       LynxPerformanceController* performance_controller);

 private:
  static void ReportException(NSException* exception);
};

template <typename F>
void PaintingContextDarwinUtils::ExecuteSafely(F&& func) {
  @try {
    func();
  } @catch (NSException* e) {
    ReportException(e);
  }
}

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_PAINTING_CONTEXT_DARWIN_UTILS_H_
