// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxFSPTracer.h>

#include "base/include/lynx_actor.h"
#include "core/public/performance_controller_platform_impl.h"
#include "core/services/performance/performance_controller.h"

using PerformanceControllerActor =
    lynx::shell::LynxActor<lynx::tasm::performance::PerformanceController>;

NS_ASSUME_NONNULL_BEGIN

@interface LynxFSPTracer () {
  std::weak_ptr<PerformanceControllerActor> _nativeWeakActorPtr;
}

/**
 * @brief Sets the native actor for this LynxPerformanceController instance.
 *
 * @param nativeActor The weak pointer to the native actor.
 */
- (void)setNativeActor:(const std::shared_ptr<PerformanceControllerActor>&)nativeActor;

@end
NS_ASSUME_NONNULL_END
