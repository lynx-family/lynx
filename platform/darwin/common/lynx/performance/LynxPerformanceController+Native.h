// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxPerformanceController.h"

#include "base/include/lynx_actor.h"
#include "core/services/performance/performance_controller.h"

@interface LynxPerformanceController () {
  std::weak_ptr<lynx::shell::LynxActor<lynx::tasm::performance::PerformanceController>>
      _weakNativeActorPtr;
}

- (void)setNativeActor:
    (const std::weak_ptr<lynx::shell::LynxActor<lynx::tasm::performance::PerformanceController>>&)
        nativeActor;

@end
