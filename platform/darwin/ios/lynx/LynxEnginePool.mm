// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxEnginePool.h"
#import <Foundation/Foundation.h>
#import <Lynx/LynxLog.h>
#import "LynxTraceEventDef.h"

#import <Lynx/LynxTemplateRender.h>
#import "LynxUIRendererProtocol.h"

@interface LynxEnginePool ()

@property(nonatomic, strong) NSMutableDictionary<id, NSMutableArray<LynxEngine *> *> *cachedEngines;

@end

@implementation LynxEnginePool

+ (instancetype)sharedInstance {
  static LynxEnginePool *instance = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    instance = [[LynxEnginePool alloc] init];
  });
  return instance;
}

- (instancetype)init {
  if (self = [super init]) {
    _cachedEngines = [NSMutableDictionary dictionary];
  }
  return self;
}

- (void)registerReuseEngine:(LynxEngine *)engine {
  if (!engine || !engine.templateBundle) {
    return;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LYNX_ENGINE_POOL_REGISTER_ENGINE);
  @synchronized(self) {
    NSMutableArray<LynxEngine *> *engineQueue =
        [self getEngineQueueWithBundle:engine.templateBundle];
    [engineQueue addObject:engine];
    [engine setEngineQueueRef:engineQueue];
    _LogI(@"LynxEnginePool: Registered engine %p, %lu", engine, [engineQueue count]);
  }
}

- (LynxEngine *)pollEngineWithRender:(LynxTemplateBundle *)templateBundle {
  if (!templateBundle) {
    return nil;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LYNX_ENGINE_POOL_POLL_ENGINE);
  NSMutableArray<LynxEngine *> *engineQueue = [self getEngineQueueWithBundle:templateBundle];
  LynxEngine *reusedEngine = nil;
  @synchronized(self) {
    for (NSUInteger i = 0; i < engineQueue.count; i++) {
      LynxEngine *engine = engineQueue[i];
      if ([engine canBeReused]) {
        [engineQueue removeObjectAtIndex:i];
        [engine detachEngine];
        _LogI(@"LynxEnginePool: Polled engine successfully %@", reusedEngine);
        return engine;
      }
    }
  }
  _LogI(@"LynxEnginePool: Failed to poll engine");
  return nil;
}

- (NSMutableArray<LynxEngine *> *)getEngineQueueWithBundle:(id)bundle {
  @synchronized(self) {
    NSString *bundleKey = [NSString stringWithFormat:@"%p", bundle];
    NSMutableArray<LynxEngine *> *engineQueue = self.cachedEngines[bundleKey];
    if (!engineQueue) {
      engineQueue = [NSMutableArray array];
      self.cachedEngines[bundleKey] = engineQueue;
    }
    return engineQueue;
  }
}

@end
