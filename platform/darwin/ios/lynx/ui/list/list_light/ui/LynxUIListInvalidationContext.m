// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxUIListInvalidationContext.h>

@implementation LynxUIListInvalidationContext

- (instancetype)initWithInitialScrollIndex:(NSInteger)index {
  self = [super init];
  if (self) {
    self.listUpdateType = LynxListUpdateTypeInitialScrollIndexUpdate;
    self.initialScrollIndex = index;
  }
  return self;
}

- (instancetype)initWithModelUpdates:(NSDictionary *)updates {
  self = [super init];
  if (self) {
    self.updates = updates;
    self.listUpdateType = LynxListUpdateTypeLayoutSelfSizing;
  }
  return self;
}

- (instancetype)initWithBoundsChange {
  self = [super init];
  if (self) {
    self.listUpdateType = LynxListUpdateTypeScrollBoundsUpdate;
  }
  return self;
}

- (instancetype)initWithGeneralPropsUpdate {
  self = [super init];
  if (self) {
    self.listUpdateType = LynxListUpdateTypeLayoutGeneralPropsUpdate;
  }
  return self;
}

- (instancetype)initWithScrollThresholdsUpdate:(LynxUIListScrollThresholds *)scrollThresholds {
  self = [super init];
  if (self) {
    self.listUpdateType = LynxListUpdateTypeScrollThresholdsUpdate;
    self.scrollThresholds = scrollThresholds;
  }
  return self;
}

- (instancetype)initWithScrollToInfo:(NSInteger)position
                              offset:(CGFloat)offset
                             alignTo:(NSString *)alignTo
                              smooth:(BOOL)smooth {
  self = [super init];
  if (self) {
    self.listUpdateType = LynxListUpdateTypeScrollToPositionUpdate;
    self.scrollToPosition = position;
    self.offset = offset;
    self.smooth = smooth;
    self.alignTo = alignTo;
  }
  return self;
}

@end

@implementation LynxUIListScrollThresholds
@end
