// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XElement/LynxVideoOperationScheduler.h>

@interface LynxVideoOperation : NSObject
@property(nonatomic, copy) NSString *name;
@property(nonatomic, copy) LynxVideoOperationExecutor executor;
@property(nonatomic, copy, nullable) LynxVideoOperationCancelHandler cancelHandler;
@end

@implementation LynxVideoOperation
@end

@interface LynxVideoOperationScheduler ()
@property(nonatomic, assign) BOOL running;
@property(nonatomic, strong) NSMutableArray<LynxVideoOperation *> *queue;
@property(nonatomic, strong, nullable) LynxVideoOperation *latestOperation;
@property(nonatomic, strong, nullable) LynxVideoOperation *currentOperation;
@end

@implementation LynxVideoOperationScheduler
@synthesize mode = _mode;

- (instancetype)init {
  if (self = [super init]) {
    _mode = LynxVideoOperationModeQueue;
    _queue = [NSMutableArray array];
  }
  return self;
}

- (void)enqueueOperationWithName:(NSString *)name executor:(LynxVideoOperationExecutor)executor {
  [self enqueueOperationWithName:name executor:executor cancelHandler:nil];
}

- (void)enqueueOperationWithName:(NSString *)name
                        executor:(LynxVideoOperationExecutor)executor
                   cancelHandler:(nullable LynxVideoOperationCancelHandler)cancelHandler {
  if (!executor) {
    return;
  }
  if (![NSThread isMainThread]) {
    __weak typeof(self) weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
      [weakSelf enqueueOperationWithName:name executor:executor cancelHandler:cancelHandler];
    });
    return;
  }

  LynxVideoOperation *operation = [[LynxVideoOperation alloc] init];
  operation.name = name ?: @"";
  operation.executor = executor;
  operation.cancelHandler = cancelHandler;

  if (self.mode == LynxVideoOperationModeDirect) {
    operation.executor(^{
    });
    return;
  }

  if (self.mode == LynxVideoOperationModeLatest && self.running) {
    LynxVideoOperation *previousLatestOperation = self.latestOperation;
    if (previousLatestOperation.cancelHandler) {
      previousLatestOperation.cancelHandler();
    }
    self.latestOperation = operation;
    return;
  }

  [self.queue addObject:operation];
  [self startNextOperationIfNeeded];
}

- (void)reset {
  if (![NSThread isMainThread]) {
    NSAssert(NO, @"LynxVideoOperationScheduler reset must be called on main thread.");
    __weak typeof(self) weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
      [weakSelf reset];
    });
    return;
  }

  [self.queue removeAllObjects];
  self.latestOperation = nil;
  self.currentOperation = nil;
  self.running = NO;
}

- (void)cancelAllOperations {
  if (![NSThread isMainThread]) {
    NSAssert(NO, @"LynxVideoOperationScheduler cancelAllOperations must be called on main thread.");
    __weak typeof(self) weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
      [weakSelf cancelAllOperations];
    });
    return;
  }

  NSMutableArray<LynxVideoOperation *> *operations = [NSMutableArray array];
  if (self.currentOperation) {
    [operations addObject:self.currentOperation];
  }
  [operations addObjectsFromArray:self.queue];
  if (self.latestOperation) {
    [operations addObject:self.latestOperation];
  }

  [self reset];

  for (LynxVideoOperation *operation in operations) {
    if (operation.cancelHandler) {
      operation.cancelHandler();
    }
  }
}

- (LynxVideoOperationMode)mode {
  if (![NSThread isMainThread]) {
    NSAssert(NO, @"LynxVideoOperationScheduler mode must be read on main thread.");
  }
  return _mode;
}

- (void)setMode:(LynxVideoOperationMode)mode {
  if (![NSThread isMainThread]) {
    __weak typeof(self) weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
      weakSelf.mode = mode;
    });
    return;
  }
  _mode = mode;
}

- (void)startNextOperationIfNeeded {
  if (self.running) {
    return;
  }

  LynxVideoOperation *operation = nil;
  if (self.mode == LynxVideoOperationModeLatest && self.latestOperation) {
    operation = self.latestOperation;
    self.latestOperation = nil;
  } else if (self.queue.count > 0) {
    operation = self.queue.firstObject;
    [self.queue removeObjectAtIndex:0];
  }

  if (!operation) {
    return;
  }

  self.running = YES;
  self.currentOperation = operation;
  __block BOOL completed = NO;
  __weak typeof(self) weakSelf = self;
  operation.executor(^{
    __strong typeof(weakSelf) strongSelf = weakSelf;
    dispatch_async(dispatch_get_main_queue(), ^{
      if (!strongSelf || completed) {
        return;
      }
      if (strongSelf.currentOperation != operation) {
        return;
      }
      completed = YES;
      strongSelf.currentOperation = nil;
      strongSelf.running = NO;
      [strongSelf startNextOperationIfNeeded];
    });
  });
}

@end
