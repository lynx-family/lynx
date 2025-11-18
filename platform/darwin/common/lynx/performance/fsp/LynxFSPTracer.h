// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxUIMeaningfulContentProtocol.h>

// Objective-C wrapper for FSP configuration
@interface LynxFSPConfigOC : NSObject

// Minimum content fill rate in X-axis direction
@property(nonatomic, assign) double minCompletionRateX;
// Minimum content fill rate in Y-axis direction
@property(nonatomic, assign) double minCompletionRateY;
// Acceptable pixel area change rate per millisecond
@property(nonatomic, assign) double acceptablePixelDiffPerMs;
// Minimum time interval to determine stability
@property(nonatomic, assign) double minDiffIntervalMs;

@end

// Objective-C wrapper for FSP tracking results
@interface LynxMeaningfulContentInfo : NSObject

@property(nonatomic, assign) LynxUIMeaningfulContentStatus status;
@property(nonatomic, assign) CGRect rect;
@property(nonatomic, assign) int64_t firstPresentedTimestampMicros;

@end

// Main interface for FSP tracer
@interface LynxFSPTracer : NSObject

// Initialization method, requires FSP configuration
- (instancetype)initWithConfig:(LynxFSPConfigOC *)config;

- (void)Start;

- (void)fill:(NSArray<LynxMeaningfulContentInfo *> *)contentsArray
    containerSize:(CGSize)containerSize;

@end
