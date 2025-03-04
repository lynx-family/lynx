// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxScrollFluency.h"
#import "LynxFluencyMonitor.h"

@implementation LynxScrollFluency {
  LynxFluencyMonitor *_fluencyMonitor;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    _fluencyMonitor = [[LynxFluencyMonitor alloc] init];
  }
  return self;
}

- (void)scrollerWillBeginDragging:(LynxScrollInfo *)info {
  [_fluencyMonitor startWithScrollInfo:info];
}

- (void)scrollerDidEndDragging:(LynxScrollInfo *)info willDecelerate:(BOOL)decelerate {
  if (!decelerate) {
    [_fluencyMonitor stopWithScrollInfo:info];
  }
}

- (void)scrollerDidEndDecelerating:(LynxScrollInfo *)info {
  [_fluencyMonitor stopWithScrollInfo:info];
}

- (void)setFluencyPageconfigProbability:(CGFloat)probability {
  [_fluencyMonitor setFluencyPageconfigProbability:probability];
}

- (BOOL)shouldSendAllScrollEvent {
  return [_fluencyMonitor shouldSendAllScrollEvent];
}

@end
