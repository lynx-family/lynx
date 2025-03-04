// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTouchHandler.h"

NS_ASSUME_NONNULL_BEGIN

@class LynxGestureArenaManager;

@interface LynxTouchHandler ()

@property(nonatomic) NSMutableArray<LynxWeakProxy *> *touchDeque;
@property(nonatomic) int32_t tapSlop;
@property(nonatomic) BOOL hasMultiTouch;
@property(nonatomic, weak) LynxGestureArenaManager *_Nullable gestureArenaManager;

@property(nonatomic, assign) BOOL enableNewGesture;

- (void)setupVelocityTracker:(UIView *)rootView;
- (void)setEnableTouchRefactor:(BOOL)enable;
- (void)setEnableEndGestureAtLastFingerUp:(BOOL)enable;
- (void)setEnableTouchPseudo:(BOOL)enable;
- (void)setEnableMultiTouch:(BOOL)enable;
- (BOOL)isEnableAndGetMultiTouch;
- (BOOL)isTouchMoving;
- (BOOL)checkOuterGestureChanged:(NSSet<UITouch *> *)touches;
- (NSInteger)setGestureArenaManagerAndGetIndex:(LynxGestureArenaManager *)gestureArenaManager;
- (void)removeGestureArenaManager:(NSInteger)index;
- (void)showMessageOnConsole:(NSString *)msg withLevel:(int32_t)level;

@end

NS_ASSUME_NONNULL_END
