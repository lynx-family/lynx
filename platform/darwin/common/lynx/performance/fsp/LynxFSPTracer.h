// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <Lynx/LynxMeaningfulContentSnapshot.h>
#import <Lynx/LynxPerformanceObserverProtocol.h>
#import <Lynx/LynxUIMeaningfulContentProtocol.h>

/// The LynxFSPTracer class provides functionality to track Meaningful Content
/// events based on the provided configuration. It allows starting the
/// tracking, filling Meaningful Content information, and retrieving the
/// results.
@interface LynxFSPTracer : NSObject <LynxPerformanceObserverProtocol>

@property(nonatomic, assign, readonly) BOOL enable;

- (void)startWithCaptureHandler:(LynxMeaningfulContentSnapshot* (^)(void))captureHandler;

- (void)stop;

- (void)stopByUserInteraction;

@end
