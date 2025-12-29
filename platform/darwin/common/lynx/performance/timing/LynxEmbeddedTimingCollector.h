// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxPerformanceObserverProtocol.h>

NS_ASSUME_NONNULL_BEGIN

@interface LynxEmbeddedTimingCollector : NSObject <LynxPerformanceObserverProtocol>
@property(nonatomic, weak, readonly, nullable) id<LynxPerformanceObserverProtocol> observer;

- (instancetype)initWithObserver:(id<LynxPerformanceObserverProtocol>)observer;
- (void)setTiming:(uint64_t)timestamp key:(NSString *)key;

@end

NS_ASSUME_NONNULL_END
