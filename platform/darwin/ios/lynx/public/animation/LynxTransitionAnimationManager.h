// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

#import "LynxAnimationInfo.h"

NS_ASSUME_NONNULL_BEGIN
@class LynxUI;
@class LynxAnimationTransformRotation;

@interface LynxTransitionAnimationManager : NSObject

- (instancetype)initWithLynxUI:(LynxUI*)ui;

- (void)assembleTransitions:(NSArray*)params;
- (void)removeTransitionAnimation:(LynxAnimationProp)prop;
- (void)removeAllTransitionAnimation;
- (void)removeAllLayoutTransitionAnimation;

- (BOOL)maybeUpdateBackgroundWithTransitionAnimation:(UIColor*)color;

- (BOOL)maybeUpdateOpacityWithTransitionAnimation:(CGFloat)opacity;

- (BOOL)maybeUpdateFrameWithTransitionAnimation:(CGRect)newFrame
                                    withPadding:(UIEdgeInsets)padding
                                         border:(UIEdgeInsets)border
                                         margin:(UIEdgeInsets)margin;

- (void)performTransitionAnimationsWithBackground:(UIColor*)color callback:(CompletionBlock)block;

- (void)performTransitionAnimationsWithOpacity:(CGFloat)newOpacity callback:(CompletionBlock)block;

- (void)performTransitionAnimationsWithVisibility:(BOOL)show callback:(CompletionBlock)block;

- (void)performTransitionAnimationsWithTransform:(CATransform3D)newTransform
                          transformWithoutRotate:(CATransform3D)newTransformWithoutRotate
                        transformWithoutRotateXY:(CATransform3D)newTransformWithoutRotateXY
                                        rotation:(LynxAnimationTransformRotation*)newRotation
                                        callback:(CompletionBlock)block;
- (BOOL)isTransitionBackgroundColor;
- (BOOL)isTransitionOpacity:(CGFloat)oldOpacity newOpacity:(CGFloat)newOpacity;
- (BOOL)isTransitionVisibility:(BOOL)oldState newState:(BOOL)newState;
- (BOOL)isTransitionTransform:(CATransform3D)oldTransform newTransform:(CATransform3D)newTransform;
- (BOOL)isTransitionTransformRotation:(LynxAnimationTransformRotation*)oldTransformRotation
                 newTransformRotation:(LynxAnimationTransformRotation*)newTransformRotation;

- (void)applyTransitionAnimation;

@end

NS_ASSUME_NONNULL_END
