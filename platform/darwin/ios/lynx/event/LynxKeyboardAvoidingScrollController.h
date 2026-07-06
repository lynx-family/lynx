// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxUI;

@interface LynxKeyboardAvoidingTargetInfo : NSObject

@property(nonatomic, assign) BOOL avoidKeyboard;
@property(nonatomic, assign) CGFloat spacing;

@end

@interface LynxKeyboardAvoidingScrollController : NSObject

@property(nonatomic, weak, readonly, nullable) LynxUI *scrollUI;

+ (void)updateContentSize:(CGSize)contentSize
            forScrollView:(nullable UIScrollView *)scrollView
            enableScrollY:(BOOL)enableScrollY
       contentSizeUpdater:(dispatch_block_t)contentSizeUpdater;
+ (void)updateFocusedInputStateForScrollView:(nullable UIScrollView *)scrollView
                               enableScrollY:(BOOL)enableScrollY;

- (void)saveTargetForOwner:(nullable id)owner
             avoidKeyboard:(BOOL)avoidKeyboard
                   spacing:(CGFloat)spacing;
- (void)resetOriginalOffsetIfInactive;
- (nullable LynxKeyboardAvoidingTargetInfo *)targetInfoForOwner:(nullable id)owner;
- (nullable LynxUI *)scrollUIForOwner:(nullable id)owner;
- (void)clear;
- (void)clearAnimated:(BOOL)animated
             duration:(NSTimeInterval)duration
                curve:(UIViewAnimationCurve)curve
              isStale:(nullable BOOL (^)(void))isStale
           completion:(nullable dispatch_block_t)completion;
- (BOOL)applyWithOwner:(nullable id)owner
             inputView:(nullable UIView *)inputView
        keyboardHeight:(CGFloat)keyboardHeight
               spacing:(CGFloat)spacing
        targetDistance:(CGFloat)targetDistance
     animationDuration:(NSTimeInterval)animationDuration;

@end

NS_ASSUME_NONNULL_END
