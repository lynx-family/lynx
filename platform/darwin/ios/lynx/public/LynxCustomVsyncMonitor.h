// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_DARWIN_IOS_LYNX_PUBLIC_LYNXCUSTOMVSYNCMONITOR_H_
#define PLATFORM_DARWIN_IOS_LYNX_PUBLIC_LYNXCUSTOMVSYNCMONITOR_H_

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

/**
 * Delivers the frame start and target times in nanoseconds using the engine's clock origin:
 * std::chrono::duration_cast<std::chrono::nanoseconds>(
 *     std::chrono::steady_clock::now().time_since_epoch()).count().
 * The target time is the expected presentation time of the frame. Convert timestamps from
 * another clock into this timebase before delivery.
 *
 * For CADisplayLink, sample the engine clock above and subtract
 * (CACurrentMediaTime() - displayLink.timestamp) converted from seconds to nanoseconds to obtain
 * the frame start time. Add (displayLink.targetTimestamp - displayLink.timestamp), also converted
 * from seconds to nanoseconds, to obtain the frame target time.
 */
typedef void (^LynxCustomVsyncCallback)(int64_t frameStartTimeNanos, int64_t frameTargetTimeNanos);

/** A host-provided source of frame signals shared by a LynxView and its runtime. */
@protocol LynxCustomVsyncMonitor <NSObject>

/**
 * Register a callback for the next frame signal and return promptly without waiting for it.
 * Registration must be thread-safe: Lynx may call this method from different threads.
 * Deliver the callback exactly once; delivery may be synchronous or on any thread.
 * Lynx dispatches each consumer's work onto its own thread.
 */
- (void)requestVsync:(LynxCustomVsyncCallback)callback;

@end

NS_ASSUME_NONNULL_END

#endif  // PLATFORM_DARWIN_IOS_LYNX_PUBLIC_LYNXCUSTOMVSYNCMONITOR_H_
