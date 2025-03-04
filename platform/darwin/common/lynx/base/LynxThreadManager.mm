// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxThreadManager.h"
#import "LynxEnv.h"

@implementation LynxThreadManager

+ (NSMutableDictionary*)queueDictionary {
  static NSMutableDictionary* dic = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    dic = [NSMutableDictionary dictionary];
  });
  return dic;
}

+ (dispatch_queue_t)getCachedQueueWithPrefix:(NSString* _Nonnull)identifier {
  NSMutableDictionary* dic = [self queueDictionary];
  @synchronized(dic) {
    return [dic valueForKey:identifier];
  }
}

+ (dispatch_queue_t)getQueueWithPrefix:(NSString* _Nonnull)identifier {
  NSMutableDictionary* dic = [self queueDictionary];
  @synchronized(dic) {
    dispatch_queue_t queue = [dic valueForKey:identifier];
    if (queue != nil) {
      return queue;
    }
    NSString* fullName = [@"com.lynx." stringByAppendingString:identifier];
    queue = dispatch_queue_create([fullName UTF8String], DISPATCH_QUEUE_CONCURRENT);
    dispatch_set_target_queue(queue, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0));
    [dic setObject:queue forKey:identifier];
    return queue;
  }
}

+ (void)threadRun:(dispatch_block_t)runnable {
  @autoreleasepool {
    [self executeRunloop:runnable];
  }
}

+ (void)executeRunloop:(dispatch_block_t)runnable {
  runnable();
  // get runLoop of current thread and run
  NSRunLoop* currentRunLoop = [NSRunLoop currentRunLoop];
  // check status of thread and its corresponding runLoop
  if (currentRunLoop != nil) {
    [currentRunLoop run];
  }
}

+ (void)createIOSThread:(NSString*)name runnable:(dispatch_block_t)runnable {
  bool shouldCreateNewThread = [LynxEnv sharedInstance].switchRunloopThread;
  if (shouldCreateNewThread) {
    NSThread* newThread = [[NSThread alloc] initWithTarget:self
                                                  selector:@selector(threadRun:)
                                                    object:runnable];
    if (newThread) {
      [newThread start];
    }
  } else {
    dispatch_async([self.class getQueueWithPrefix:name], ^{
      [self executeRunloop:runnable];
    });
  }
}

// judge whether current is run in main queue
+ (BOOL)isMainQueue {
  return dispatch_queue_get_label(DISPATCH_CURRENT_QUEUE_LABEL) ==
         dispatch_queue_get_label(dispatch_get_main_queue());
}

// This method submits a block directly to the dispatch queue, skipping the messageLoop native code
// uses. It might cause the block to be run before some tasks submitted in native messageLoop, even
// if they were submitted earlier.
+ (void)runBlockInMainQueue:(dispatch_block_t _Nonnull)runnable {
  dispatch_async(dispatch_get_main_queue(), runnable);
}

+ (void)runInTargetQueue:(dispatch_queue_t)queue runnable:(dispatch_block_t)runnable {
  dispatch_async(queue, runnable);
}

@end
