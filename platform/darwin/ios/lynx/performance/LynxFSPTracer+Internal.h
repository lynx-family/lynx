// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>

#import <Lynx/LynxFSPTracer.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, LynxFSPMode) {
  LynxFSPModeAxial = 0,
  LynxFSPModeArea = 1,
};

typedef struct LynxFSPConfig {
  /// Content tracking mode for FSP calculation.
  LynxFSPMode mode;

  /// Interval length of a snapshot in seconds.
  /// The tracer will take snapshots of the page content status at this interval.
  NSTimeInterval snapshotInterval;

  /// Hard timeout for the entire tracing process
  /// The tracer won't wait longer than this value to complete the FSP calculation.
  NSTimeInterval hardTimeout;

  /// Graceful timeout to wait when FSP has been reached
  ///
  /// This is useful for cases where the FSP is reached initially but the page
  /// is still loading and any changes happening in the grace period can also
  /// be captured.
  NSTimeInterval gracefulTimeout;

  /// Extended graceful timeout to wait when FSP has been reached and the page is
  /// still loading
  ///
  /// This is useful for cases where the FSP is reached initially but the page
  /// is still loading and any changes happening in the grace period can also
  /// be captured.
  ///
  /// This timeout is used to extend the graceful timeout when there is at least
  /// one ui elemnt that is still loading in the tree.
  NSTimeInterval extendedGracefulTimeout;

  /// DevTool Only.
  uintptr_t tracerConfig;
} LynxFSPConfig;

static const LynxFSPConfig kLynxFSPConfigDefault = {
    .mode = LynxFSPModeAxial,
    .snapshotInterval = 0.016,
    .hardTimeout = 10.0,
    .gracefulTimeout = 0.5,
    .extendedGracefulTimeout = 1.0,
};

// DevTool Only.
@protocol LynxFSPTracerObserver <NSObject>

@required
- (void)fspTracerDidStart:(LynxFSPTracer*)tracer;
- (void)fspTracer:(LynxFSPTracer*)tracer
    didBecomeStable:(BOOL)stable
         withResult:(LynxFSPResult)result;
- (void)fspTracer:(LynxFSPTracer*)tracer
    willStopForReason:(LynxFSPTracerStopReason)reason
           withResult:(LynxFSPResult)result;
- (void)fspTracer:(LynxFSPTracer*)tracer willStartGracefulTimeout:(double)timeout;

@end

@interface LynxFSPTracer () {
  id<LynxFSPTracerObserver> _observer;
}

+ (void)setEnabled:(BOOL)enabled config:(LynxFSPConfig const*)config;

- (void)setObserver:(id<LynxFSPTracerObserver>)observer;

- (void)setTimingCallback:(void (^)(uint64_t timestamp, NSString* key))timingCallback;

// DevTool Only.
- (uintptr_t)opaqueTracer;

@end

NS_ASSUME_NONNULL_END
