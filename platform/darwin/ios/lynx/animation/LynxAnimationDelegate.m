// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxAnimationDelegate.h"
#import "LynxUI+Internal.h"
#import "LynxUI.h"
@implementation LynxAnimationDelegate

+ (instancetype)initWithDidStart:(DidAnimationStart __nullable)start
                         didStop:(DidAnimationStop)stop {
  LynxAnimationDelegate *delegate = [[LynxAnimationDelegate alloc] init];
  delegate.didStart = start;
  delegate.didStop = stop;
  return delegate;
}

- (void)animationDidStart:(CAAnimation *)animation {
  DidAnimationStart didStartTemp = _didStart;
  // Clear _didStart to ensure that _didStart is executed only once.
  _didStart = nil;
  if (didStartTemp) {
    didStartTemp(animation);
  }
}

- (void)didStop:(CAAnimation *)animation withFlag:(BOOL)flag {
  DidAnimationStop didStopTemp = _didStop;
  // Clear _didStop to ensure that _didStop is executed only once.
  _didStop = nil;
  if (didStopTemp) {
    didStopTemp(animation, flag);
  }
}

- (void)animationDidStop:(CAAnimation *)animation finished:(BOOL)flag {
  [self didStop:animation withFlag:flag];
}

- (void)forceStop {
  [self didStop:nil withFlag:NO];
}

- (id)copyWithZone:(NSZone *)zone {
  LynxAnimationDelegate *delegate = [[LynxAnimationDelegate allocWithZone:zone] init];
  delegate.didStart = self.didStart;
  delegate.didStop = self.didStop;
  return delegate;
}

+ (void)sendAnimationEvent:(LynxUI *)ui
                 eventName:(NSString *)eventName
               eventParams:(NSDictionary *)params {
  __strong LynxUI *strongUI = ui;
  if (!strongUI) {
    return;
  }
  if ([strongUI.eventSet valueForKey:eventName]) {
    [strongUI.context.eventEmitter
        dispatchCustomEvent:[[LynxCustomEvent alloc] initWithName:eventName
                                                       targetSign:strongUI.sign
                                                           params:params]];
  }
}

@end
