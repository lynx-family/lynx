// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, LynxVideoOperationMode) {
  LynxVideoOperationModeQueue = 0,
  LynxVideoOperationModeDirect,
  LynxVideoOperationModeLatest,
};

typedef void (^LynxVideoOperationCompletion)(void);
typedef void (^LynxVideoOperationExecutor)(LynxVideoOperationCompletion completion);
typedef void (^LynxVideoOperationCancelHandler)(void);

@interface LynxVideoOperationScheduler : NSObject

// Main-thread owned. Callers should invoke scheduler APIs from the main thread
// so operation ordering and callbacks remain deterministic.
@property(nonatomic, assign) LynxVideoOperationMode mode;

- (void)enqueueOperationWithName:(NSString *)name executor:(LynxVideoOperationExecutor)executor;
- (void)enqueueOperationWithName:(NSString *)name
                        executor:(LynxVideoOperationExecutor)executor
                   cancelHandler:(nullable LynxVideoOperationCancelHandler)cancelHandler;
- (void)cancelAllOperations;
// Clears scheduler state without invoking cancel handlers. Use cancelAllOperations
// when callers need queued/current operation callbacks to be notified.
- (void)reset;

@end

NS_ASSUME_NONNULL_END
