// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_NATIVE_PAINTING_CONTEXT_PLATFORM_DARWIN_REF_H_
#define CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_NATIVE_PAINTING_CONTEXT_PLATFORM_DARWIN_REF_H_

#import <Lynx/LynxPerformanceController.h>

#include <memory>

#include "core/renderer/ui_wrapper/painting/native_painting_context_platform_ref.h"

namespace lynx {
namespace tasm {

// TODO: No methods need overriding for now; add overrides if needed in the
// future
class NativePaintingCtxPlatformDarwinRef : public NativePaintingCtxPlatformRef {
 public:
  explicit NativePaintingCtxPlatformDarwinRef(
      std::unique_ptr<PlatformRendererFactory> view_factory);
  ~NativePaintingCtxPlatformDarwinRef() override = default;

  void SetPerformanceController(LynxPerformanceController* controller) {
    perf_controller_ = controller;
  }

 private:
  __weak LynxPerformanceController* perf_controller_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_WRAPPER_PAINTING_IOS_NATIVE_PAINTING_CONTEXT_PLATFORM_DARWIN_REF_H_
