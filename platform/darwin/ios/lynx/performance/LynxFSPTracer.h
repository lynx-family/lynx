// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef enum {
  kLynxFSPTracerStopReasonNone = 0,
  kLynxFSPTracerStopReasonSuccess = 1,
  kLynxFSPTracerStopReasonUserInteraction = 2,
  kLynxFSPTracerStopReasonTimeout = 3,
} LynxFSPTracerStopReason;

typedef struct {
  BOOL success;
  CGSize contentSize;
  NSInteger lastUISign;
  /// unit: lynx::base::CurrentSystemTimeMicroseconds()
  uint64_t lastChangeTimestamp;
} LynxFSPResult;

@class LynxRootUI;
@class LynxUIContext;

@interface LynxFSPTracer : NSObject

+ (BOOL)isEnabled;

- (instancetype)init NS_UNAVAILABLE;

/// Create a tracer instance with ui context and default config.
- (nullable instancetype)initWithUIContext:(LynxUIContext*)context;

/// Whether the tracer is currently tracing.
///
/// Must be called on the main thread.
- (BOOL)isTracing;

/// Begin the FSP tracing process.
///
/// Must be called on the main thread.
- (void)beginTracing:(void (^)(LynxFSPResult result))completion;

/// Stop the FSP tracing process.
///
/// Must be called on the main thread.
- (void)stopTracing:(LynxFSPTracerStopReason)reason;

@end

NS_ASSUME_NONNULL_END
