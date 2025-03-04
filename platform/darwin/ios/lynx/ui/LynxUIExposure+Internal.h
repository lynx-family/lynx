// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUIExposure.h"

NS_ASSUME_NONNULL_BEGIN

@interface LynxUIExposure ()

- (instancetype)initWithObserver:(LynxGlobalObserver *)observer;
- (BOOL)isLynxViewChanged;
- (void)setObserverFrameRate:(int32_t)rate;
- (void)setEnableCheckExposureOptimize:(BOOL)enableCheckExposureOptimize;

@end

NS_ASSUME_NONNULL_END
