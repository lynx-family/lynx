// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxIntersectionObserverModule.h"
#import "LynxContext+Internal.h"
#import "LynxContext.h"
#import "LynxUIIntersectionObserver.h"

using namespace lynx;

@implementation LynxIntersectionObserverModule {
  __weak LynxContext *context_;
}

- (instancetype)initWithLynxContext:(LynxContext *)context {
  self = [super init];
  if (self) {
    context_ = context;
  }

  return self;
}

+ (NSString *)name {
  return @"IntersectionObserverModule";
}

+ (NSDictionary<NSString *, NSString *> *)methodLookup {
  return @{
    @"createIntersectionObserver" :
        NSStringFromSelector(@selector(createIntersectionObserver:inComponent:withOptions:)),
    @"relativeTo" : NSStringFromSelector(@selector(relativeTo:selector:margins:)),
    @"relativeToViewport" : NSStringFromSelector(@selector(relativeToViewport:margins:)),
    @"relativeToScreen" : NSStringFromSelector(@selector(relativeToScreen:margins:)),
    @"observe" : NSStringFromSelector(@selector(observe:selector:withCallbackId:)),
    @"disconnect" : NSStringFromSelector(@selector(disconnect:))
  };
}

typedef void (^LynxIntersectionObserverBlock)(LynxIntersectionObserverModule *);

- (void)runOnUIThreadSafely:(LynxIntersectionObserverBlock)block {
  __weak LynxIntersectionObserverModule *weakSelf = self;
  dispatch_async(dispatch_get_main_queue(), ^{
    if (weakSelf) {
      __strong LynxIntersectionObserverModule *strongSelf = weakSelf;
      if (strongSelf->context_) {
        block(strongSelf);
      }
    }
  });
}

- (void)createIntersectionObserver:(NSInteger)observerId
                       inComponent:(NSString *)componentId
                       withOptions:(NSDictionary *)options {
  NSString *componentSign = kDefaultComponentID;
  if (componentId.length) {
    componentSign = componentId;
  }
  [self runOnUIThreadSafely:^(LynxIntersectionObserverModule *target) {
    LynxUIIntersectionObserverManager *manager = target->context_.intersectionManager;
    if (manager) {
      LynxUIIntersectionObserver *observer =
          [[LynxUIIntersectionObserver alloc] initWithManager:manager
                                                   observerId:observerId
                                                  componentId:componentSign
                                                      options:options];
      [manager addIntersectionObserver:observer];
    }
  }];
}

- (void)relativeTo:(NSInteger)observerId
          selector:(NSString *)selector
           margins:(NSDictionary *)margins {
  [self runOnUIThreadSafely:^(LynxIntersectionObserverModule *target) {
    LynxUIIntersectionObserverManager *manager = target->context_.intersectionManager;
    if (manager) {
      LynxUIIntersectionObserver *observer = [manager getObserverById:observerId];
      if (observer) {
        [observer relativeTo:selector margins:margins];
      }
    }
  }];
}

- (void)relativeToViewport:(NSInteger)observerId margins:(NSDictionary *)margins {
  [self runOnUIThreadSafely:^(LynxIntersectionObserverModule *target) {
    LynxUIIntersectionObserverManager *manager = target->context_.intersectionManager;
    if (manager) {
      LynxUIIntersectionObserver *observer = [manager getObserverById:observerId];
      if (observer) {
        [observer relativeToViewportWithMargins:margins];
      }
    }
  }];
}

- (void)relativeToScreen:(NSInteger)observerId margins:(NSDictionary *)margins {
  [self runOnUIThreadSafely:^(LynxIntersectionObserverModule *target) {
    LynxUIIntersectionObserverManager *manager = target->context_.intersectionManager;
    if (manager) {
      LynxUIIntersectionObserver *observer = [manager getObserverById:observerId];
      if (observer) {
        [observer relativeToScreenWithMargins:margins];
      }
    }
  }];
}

- (void)observe:(NSInteger)observerId
          selector:(NSString *)selector
    withCallbackId:(NSInteger)callbackId {
  [self runOnUIThreadSafely:^(LynxIntersectionObserverModule *target) {
    LynxUIIntersectionObserverManager *manager = target->context_.intersectionManager;
    if (manager) {
      LynxUIIntersectionObserver *observer = [manager getObserverById:observerId];
      if (observer) {
        [observer observe:selector callbackId:callbackId];
      }
    }
  }];
}

- (void)disconnect:(NSInteger)observerId {
  [self runOnUIThreadSafely:^(LynxIntersectionObserverModule *target) {
    LynxUIIntersectionObserverManager *manager = target->context_.intersectionManager;
    if (manager) {
      LynxUIIntersectionObserver *observer = [manager getObserverById:observerId];
      if (observer) {
        [observer disconnect];
      }
    }
  }];
}

@end
