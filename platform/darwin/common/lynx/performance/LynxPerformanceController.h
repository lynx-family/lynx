// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxPerformanceObserverProtocol.h>
#import "LynxMemoryMonitorProtocol.h"

@interface LynxPerformanceController
    : NSObject <LynxMemoryMonitorProtocol, LynxPerformanceObserverProtocol>

@property(nonatomic, weak, readonly) id<LynxPerformanceObserverProtocol> observer;

- (instancetype _Nonnull)initWithObserver:(id<LynxPerformanceObserverProtocol> _Nonnull)observer;

+ (void)setForceEnable:(BOOL)forceEnable;

@end
