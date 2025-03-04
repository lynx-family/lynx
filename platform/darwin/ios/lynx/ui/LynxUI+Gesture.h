// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxGestureArenaMember.h>
#import <Lynx/LynxNewGestureDelegate.h>
#import <Lynx/LynxUI.h>

NS_ASSUME_NONNULL_BEGIN
@interface LynxUI (Gesture) <LynxGestureArenaMember, LynxNewGestureDelegate>

- (void)consumeInternalGesture:(BOOL)consume;

@end
NS_ASSUME_NONNULL_END
