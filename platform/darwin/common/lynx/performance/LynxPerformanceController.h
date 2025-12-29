// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxFSPTracer.h>
#import <Lynx/LynxPerformanceObserverProtocol.h>
#import "LynxMemoryMonitorProtocol.h"
#import "LynxTimingCollectorProtocol.h"

NS_ASSUME_NONNULL_BEGIN
/**
 * @brief Manages performance data collection and observation. This class acts as a central point
 * for performance-related events, conforming to timing collection, memory monitoring, and
 * performance observation protocols.
 */
@interface LynxPerformanceController : NSObject <LynxTimingCollectorProtocol,
                                                 LynxMemoryMonitorProtocol,
                                                 LynxPerformanceObserverProtocol>
/** The observer that will receive performance event notifications. */
@property(nonatomic, weak, readonly, nullable) id<LynxPerformanceObserverProtocol> observer;
@property(nonatomic, strong, readonly) LynxFSPTracer *fspTracer;

/**
 * @brief Initializes a LynxPerformanceController instance with a specified observer.
 *
 * @param observer The observer that will receive performance event notifications.
 * @return An initialized LynxPerformanceController instance.
 */
- (instancetype)initWithObserver:(id<LynxPerformanceObserverProtocol>)observer;
- (void)setEmbeddedModeEnabled:(BOOL)enable;

@end

NS_ASSUME_NONNULL_END
