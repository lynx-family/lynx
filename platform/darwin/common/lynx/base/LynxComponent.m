// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxComponent.h"
#import <Lynx/LynxLog.h>

@implementation LynxComponent

- (instancetype)init {
  self = [super init];
  if (self) {
    _children = [NSMutableArray new];
  }
  return self;
}

- (void)insertChild:(LynxComponent *)child atIndex:(NSInteger)index {
  if (child == nil) {
    return;
  }
  [child willMoveToSuperComponent:self];
  @try {
    [_children insertObject:child atIndex:index];
    child.parent = self;
    [self didAddSubComponent:child];
    [child didMoveToSuperComponet];
  } @catch (NSException *exception) {
    LLog(@"[Lynx][ShadowNodeOwner] insertChild at wrong index; %@", exception.reason);
  }
}

- (void)removeChild:(LynxComponent *)child atIndex:(NSInteger)index {
  if (child == nil) {
    return;
  }
  [child willMoveToSuperComponent:nil];
  [self willRemoveComponent:child];
  [_children removeObject:child];
  child.parent = nil;
  [child didMoveToSuperComponet];
}

- (void)didAddSubComponent:(nonnull LynxComponent *)subComponent {
}

- (void)willRemoveComponent:(nonnull LynxComponent *)subComponent {
}

- (void)willMoveToSuperComponent:(nullable LynxComponent *)newSuperComponent {
}

- (void)didMoveToSuperComponet {
}

- (void)propsDidUpdate {
}

- (void)animationPropsDidUpdate {
}

- (void)transformPropsDidUpdate {
}

- (void)onNodeReady {
}

- (void)onNodeRemoved {
}

- (void)onNodeReload {
}
@end
