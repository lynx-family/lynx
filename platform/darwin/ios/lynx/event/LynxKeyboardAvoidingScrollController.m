// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxKeyboardAvoidingScrollController.h"

#import <Lynx/LynxBaseScrollView+Auto.h>
#import <Lynx/LynxBaseScrollView.h>
#import <Lynx/LynxScrollView.h>
#import <Lynx/LynxUI.h>
#import <Lynx/LynxUIScrollViewInternal.h>
#import <Lynx/LynxUIScroller.h>

#include <math.h>
#import <objc/runtime.h>

static const CGFloat kLynxKeyboardAvoidingScrollEpsilon = 0.5;
static char kLynxKeyboardAvoidingFocusedStateKey;

@interface LynxUI (LynxKeyboardAvoidingScrollTraversal)
- (NSArray<LynxUI *> *)children;
@end

@implementation LynxKeyboardAvoidingTargetInfo
@end

@interface LynxKeyboardAvoidingFocusedState : NSObject
@property(nonatomic, weak, nullable) UIView *focusedView;
@property(nonatomic, assign) CGRect focusedRectInScrollView;
@property(nonatomic, assign) BOOL hasFocusedRect;
@end

@implementation LynxKeyboardAvoidingFocusedState
@end

@interface LynxKeyboardAvoidingScrollController ()

@property(nonatomic, weak, readwrite, nullable) LynxUI *scrollUI;
@property(nonatomic, strong) NSMapTable<id, LynxKeyboardAvoidingTargetInfo *> *targets;
@property(nonatomic, assign) CGFloat baseContentHeight;
@property(nonatomic, assign) CGFloat contentHeightExtra;
@property(nonatomic, assign) CGFloat appliedContentHeight;
@property(nonatomic, assign) CGPoint originalOffset;
@property(nonatomic, assign) BOOL hasOriginalOffset;

@end

@implementation LynxKeyboardAvoidingScrollController

+ (nullable UIView *)firstResponderInView:(nullable UIView *)view {
  if (view == nil) {
    return nil;
  }
  if (view.isFirstResponder) {
    return view;
  }
  for (UIView *subview in view.subviews) {
    UIView *firstResponder = [self firstResponderInView:subview];
    if (firstResponder != nil) {
      return firstResponder;
    }
  }
  return nil;
}

+ (nullable LynxScrollView *)nearestLynxScrollViewForView:(nullable UIView *)view {
  for (UIView *currentView = view; currentView != nil; currentView = currentView.superview) {
    if ([currentView isKindOfClass:LynxScrollView.class]) {
      return (LynxScrollView *)currentView;
    }
  }
  return nil;
}

+ (CGFloat)keyboardAvoidingContentHeightExtraForScrollView:(nullable UIScrollView *)scrollView {
  if ([scrollView isKindOfClass:LynxScrollView.class]) {
    return MAX(0, ((LynxScrollView *)scrollView).keyboardAvoidingContentHeightExtra);
  }
  return 0;
}

+ (nullable LynxKeyboardAvoidingFocusedState *)focusedStateForScrollView:
                                                   (nullable UIScrollView *)scrollView
                                                                  create:(BOOL)create {
  if (scrollView == nil) {
    return nil;
  }
  LynxKeyboardAvoidingFocusedState *state =
      objc_getAssociatedObject(scrollView, &kLynxKeyboardAvoidingFocusedStateKey);
  if (state == nil && create) {
    state = [[LynxKeyboardAvoidingFocusedState alloc] init];
    objc_setAssociatedObject(scrollView, &kLynxKeyboardAvoidingFocusedStateKey, state,
                             OBJC_ASSOCIATION_RETAIN_NONATOMIC);
  }
  return state;
}

+ (BOOL)focusedView:(nullable UIView *)focusedView
    belongsToScrollView:(nullable UIScrollView *)scrollView
          enableScrollY:(BOOL)enableScrollY {
  return focusedView != nil && scrollView != nil && enableScrollY &&
         [focusedView conformsToProtocol:@protocol(UITextInput)] &&
         [self nearestLynxScrollViewForView:focusedView] == scrollView;
}

+ (CGPoint)clampedContentOffset:(CGPoint)offset
                  forScrollView:(nullable UIScrollView *)scrollView
                    contentSize:(CGSize)contentSize {
  if (scrollView == nil) {
    return offset;
  }
  CGFloat minOffsetY = -scrollView.contentInset.top;
  CGFloat maxOffsetY = MAX(minOffsetY, contentSize.height - scrollView.bounds.size.height +
                                           scrollView.contentInset.bottom);
  offset.y = MIN(MAX(offset.y, minOffsetY), maxOffsetY);
  return offset;
}

+ (void)updateContentSize:(CGSize)contentSize
            forScrollView:(nullable UIScrollView *)scrollView
            enableScrollY:(BOOL)enableScrollY
       contentSizeUpdater:(dispatch_block_t)contentSizeUpdater {
  if (scrollView == nil) {
    return;
  }

  UIView *focusedView = [self firstResponderInView:scrollView];
  BOOL hasFocusedView = [self focusedView:focusedView
                      belongsToScrollView:scrollView
                            enableScrollY:enableScrollY];
  LynxKeyboardAvoidingFocusedState *state = [self focusedStateForScrollView:scrollView create:YES];
  CGFloat contentHeightExtra = [self keyboardAvoidingContentHeightExtraForScrollView:scrollView];
  CGPoint adjustedOffset = scrollView.contentOffset;
  BOOL shouldAdjustOffset = NO;
  CGFloat focusedBottom = 0;
  CGFloat lastFocusedBottom = 0;
  CGFloat deltaBottom = 0;

  if (hasFocusedView) {
    CGRect focusedRectInScrollView = [focusedView convertRect:focusedView.bounds toView:scrollView];
    focusedBottom = CGRectGetMaxY(focusedRectInScrollView);
    lastFocusedBottom = CGRectGetMaxY(state.focusedRectInScrollView);
    deltaBottom = focusedBottom - lastFocusedBottom;
    shouldAdjustOffset = state.hasFocusedRect && state.focusedView == focusedView &&
                         contentHeightExtra > kLynxKeyboardAvoidingScrollEpsilon &&
                         fabs(deltaBottom) >= kLynxKeyboardAvoidingScrollEpsilon;
    if (shouldAdjustOffset) {
      adjustedOffset.y += deltaBottom;
    }
  }

  if (contentSizeUpdater != nil) {
    contentSizeUpdater();
  }

  if (shouldAdjustOffset) {
    CGPoint targetOffset = CGPointMake(scrollView.contentOffset.x, adjustedOffset.y);
    CGPoint clampedOffset = [self clampedContentOffset:targetOffset
                                         forScrollView:scrollView
                                           contentSize:contentSize];
    if (fabs(clampedOffset.y - scrollView.contentOffset.y) >= kLynxKeyboardAvoidingScrollEpsilon) {
      [scrollView setContentOffset:clampedOffset animated:NO];
    }
  }
}

+ (void)updateFocusedInputStateForScrollView:(nullable UIScrollView *)scrollView
                               enableScrollY:(BOOL)enableScrollY {
  LynxKeyboardAvoidingFocusedState *state = [self focusedStateForScrollView:scrollView create:YES];
  UIView *focusedView = [self firstResponderInView:scrollView];
  if (state != nil && [self focusedView:focusedView
                          belongsToScrollView:scrollView
                                enableScrollY:enableScrollY]) {
    state.focusedRectInScrollView = [focusedView convertRect:focusedView.bounds toView:scrollView];
    state.focusedView = focusedView;
    state.hasFocusedRect = YES;
  } else {
    state.focusedView = nil;
    state.focusedRectInScrollView = CGRectZero;
    state.hasFocusedRect = NO;
  }
}

- (instancetype)init {
  if (self = [super init]) {
    _targets = [NSMapTable weakToStrongObjectsMapTable];
  }
  return self;
}

- (void)saveTargetForOwner:(nullable id)owner
             avoidKeyboard:(BOOL)avoidKeyboard
                   spacing:(CGFloat)spacing {
  if (owner == nil) {
    return;
  }
  LynxKeyboardAvoidingTargetInfo *targetInfo = [[LynxKeyboardAvoidingTargetInfo alloc] init];
  targetInfo.avoidKeyboard = avoidKeyboard;
  targetInfo.spacing = spacing;
  [self.targets setObject:targetInfo forKey:owner];
}

- (void)resetOriginalOffsetIfInactive {
  if (self.scrollUI != nil) {
    return;
  }
  self.originalOffset = CGPointZero;
  self.hasOriginalOffset = NO;
}

- (nullable LynxKeyboardAvoidingTargetInfo *)targetInfoForOwner:(nullable id)owner {
  if (owner == nil) {
    return nil;
  }
  return [self.targets objectForKey:owner];
}

- (nullable LynxUI *)scrollUIForOwner:(nullable id)owner {
  if (![owner isKindOfClass:LynxUI.class]) {
    return nil;
  }
  LynxUI *ui = [(LynxUI *)owner getParent];
  while (ui != nil) {
    if ([self isVerticalScrollUI:ui]) {
      return ui;
    }
    ui = [ui getParent];
  }
  return nil;
}

- (BOOL)isVerticalScrollUI:(LynxUI *)ui {
  if ([ui isKindOfClass:LynxUIScroller.class]) {
    return ((LynxUIScroller *)ui).enableScrollY;
  }
  if ([ui isKindOfClass:LynxUIScrollViewInternal.class]) {
    UIView *view = ui.view;
    return [view isKindOfClass:LynxBaseScrollView.class] && ((LynxBaseScrollView *)view).vertical;
  }
  return NO;
}

- (UIScrollView *)scrollViewForScrollUI:(LynxUI *)scrollUI {
  UIView *view = scrollUI.view;
  return [view isKindOfClass:UIScrollView.class] ? (UIScrollView *)view : nil;
}

- (LynxUI *)lowestInputUIInScrollUI:(LynxUI *)scrollUI scrollView:(UIScrollView *)scrollView {
  LynxUI *lowestInputUI = nil;
  CGFloat lowestInputBottom = -CGFLOAT_MAX;
  NSMutableArray<LynxUI *> *pendingUIs = [NSMutableArray arrayWithObject:scrollUI];
  while (pendingUIs.count > 0) {
    LynxUI *ui = pendingUIs.lastObject;
    [pendingUIs removeLastObject];
    if (ui != scrollUI && [self isVerticalScrollUI:ui]) {
      continue;
    }

    UIView *view = ui.view;
    LynxKeyboardAvoidingTargetInfo *targetInfo = [self targetInfoForOwner:ui];
    if (view != nil && targetInfo.avoidKeyboard &&
        [view conformsToProtocol:@protocol(UITextInput)]) {
      CGRect inputRectInScrollView = [view convertRect:view.bounds toView:scrollView];
      CGFloat inputBottom = CGRectGetMaxY(inputRectInScrollView);
      if (inputBottom > lowestInputBottom) {
        lowestInputBottom = inputBottom;
        lowestInputUI = ui;
      }
    }

    for (LynxUI *child in ui.children) {
      [pendingUIs addObject:child];
    }
  }
  return lowestInputUI;
}

- (CGFloat)baseContentHeightForScrollView:(UIScrollView *)scrollView {
  if (scrollView == nil) {
    return 0;
  }
  CGFloat currentHeight = MAX(0, scrollView.contentSize.height);
  if ([scrollView isKindOfClass:LynxScrollView.class]) {
    CGFloat contentHeightExtra =
        MAX(0, ((LynxScrollView *)scrollView).keyboardAvoidingContentHeightExtra);
    CGFloat baseContentHeight = MAX(0, currentHeight - contentHeightExtra);
    self.baseContentHeight = baseContentHeight;
    self.contentHeightExtra = contentHeightExtra;
    self.appliedContentHeight = currentHeight;
    return baseContentHeight;
  }
  if (self.contentHeightExtra > 0 &&
      fabs(currentHeight - self.appliedContentHeight) < kLynxKeyboardAvoidingScrollEpsilon) {
    return MAX(0, self.baseContentHeight);
  }
  self.baseContentHeight = currentHeight;
  self.contentHeightExtra = 0;
  self.appliedContentHeight = currentHeight;
  return currentHeight;
}

- (CGFloat)coveredHeightForScrollView:(UIScrollView *)scrollView
                        visibleHeight:(CGFloat)visibleHeight {
  if (scrollView == nil) {
    return 0;
  }
  CGFloat originalVisibleHeight =
      MAX(0, scrollView.bounds.size.height - scrollView.contentInset.bottom);
  return MAX(0, originalVisibleHeight - MAX(0, visibleHeight));
}

- (void)captureOriginalOffsetIfNeeded:(UIScrollView *)scrollView {
  if (scrollView == nil || self.hasOriginalOffset) {
    return;
  }
  self.originalOffset = scrollView.contentOffset;
  self.hasOriginalOffset = YES;
}

- (void)setContentHeight:(CGFloat)height forScrollView:(UIScrollView *)scrollView {
  if (scrollView == nil) {
    return;
  }
  CGSize contentSize = scrollView.contentSize;
  contentSize.height = MAX(0, height);
  if (fabs(scrollView.contentSize.height - contentSize.height) <
      kLynxKeyboardAvoidingScrollEpsilon) {
    return;
  }
  if ([scrollView isKindOfClass:LynxBaseScrollView.class]) {
    [(LynxBaseScrollView *)scrollView setScrollContentSize:contentSize];
  } else {
    scrollView.contentSize = contentSize;
  }
}

- (void)setContentHeightExtra:(CGFloat)extra forScrollView:(UIScrollView *)scrollView {
  CGFloat baseContentHeight = [self baseContentHeightForScrollView:scrollView];
  [self setContentHeightExtra:extra
                forScrollView:scrollView
            baseContentHeight:baseContentHeight
                  updateState:YES];
}

- (void)setContentHeightExtra:(CGFloat)extra
                forScrollView:(UIScrollView *)scrollView
            baseContentHeight:(CGFloat)baseContentHeight
                  updateState:(BOOL)updateState {
  if (scrollView == nil) {
    return;
  }
  CGFloat contentHeightExtra = MAX(0, extra);
  if ([scrollView isKindOfClass:LynxScrollView.class]) {
    LynxScrollView *lynxScrollView = (LynxScrollView *)scrollView;
    lynxScrollView.keyboardAvoidingContentHeightExtra = contentHeightExtra;
    [lynxScrollView updateContentSize];
    if (updateState) {
      self.baseContentHeight = baseContentHeight;
      self.contentHeightExtra = contentHeightExtra;
      self.appliedContentHeight = MAX(0, lynxScrollView.contentSize.height);
    }
    return;
  }
  CGFloat appliedContentHeight = baseContentHeight + contentHeightExtra;
  [self setContentHeight:appliedContentHeight forScrollView:scrollView];
  if (updateState) {
    self.baseContentHeight = baseContentHeight;
    self.contentHeightExtra = contentHeightExtra;
    self.appliedContentHeight = appliedContentHeight;
  }
}

- (void)resetContentState {
  self.baseContentHeight = 0;
  self.contentHeightExtra = 0;
  self.appliedContentHeight = 0;
  self.originalOffset = CGPointZero;
  self.hasOriginalOffset = NO;
}

- (void)clear {
  [self clearAnimated:NO duration:0 curve:UIViewAnimationCurveEaseInOut isStale:nil completion:nil];
}

- (void)clearAnimated:(BOOL)animated
             duration:(NSTimeInterval)duration
                curve:(UIViewAnimationCurve)curve
              isStale:(nullable BOOL (^)(void))isStale
           completion:(nullable dispatch_block_t)completion {
  LynxUI *scrollUI = self.scrollUI;
  self.scrollUI = nil;
  if (scrollUI == nil) {
    [self resetContentState];
    if (completion != nil) {
      completion();
    }
    return;
  }
  UIScrollView *scrollView = [self scrollViewForScrollUI:scrollUI];
  if (scrollView == nil) {
    [self resetContentState];
    if (completion != nil) {
      completion();
    }
    return;
  }
  CGPoint offset = self.hasOriginalOffset ? self.originalOffset : scrollView.contentOffset;
  CGFloat baseContentHeight = [self baseContentHeightForScrollView:scrollView];
  CGFloat contentHeightExtra = self.contentHeightExtra;
  CGSize contentSize = scrollView.contentSize;
  contentSize.height = baseContentHeight;
  offset = [self clampedContentOffset:offset forScrollView:scrollView contentSize:contentSize];

  void (^finish)(void) = ^{
    BOOL stale = isStale != nil && isStale();
    if (stale && self.scrollUI == scrollUI) {
      return;
    }
    [self setContentHeightExtra:0
                  forScrollView:scrollView
              baseContentHeight:baseContentHeight
                    updateState:!stale];
    CGPoint finalOffset = [self clampedContentOffset:offset forScrollView:scrollView];
    if (fabs(finalOffset.y - scrollView.contentOffset.y) >= kLynxKeyboardAvoidingScrollEpsilon) {
      [scrollView setContentOffset:finalOffset animated:NO];
    }
    if (stale) {
      if (self.scrollUI == nil) {
        [self resetContentState];
      }
      return;
    }
    [self resetContentState];
    if (completion != nil) {
      completion();
    }
  };

  BOOL shouldAnimateOffset =
      animated && duration > 0 &&
      fabs(offset.y - scrollView.contentOffset.y) >= kLynxKeyboardAvoidingScrollEpsilon;
  if (!shouldAnimateOffset) {
    if (animated && duration > 0 && contentHeightExtra >= kLynxKeyboardAvoidingScrollEpsilon) {
      dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(duration * NSEC_PER_SEC)),
                     dispatch_get_main_queue(), finish);
      return;
    }
    finish();
    return;
  }

  UIViewAnimationOptions options = ((NSUInteger)curve << 16) |
                                   UIViewAnimationOptionBeginFromCurrentState |
                                   UIViewAnimationOptionAllowUserInteraction;
  [UIView animateWithDuration:duration
      delay:0
      options:options
      animations:^{
        [scrollView setContentOffset:offset animated:NO];
      }
      completion:^(__unused BOOL finished) {
        finish();
      }];
}

- (CGPoint)clampedContentOffset:(CGPoint)offset
                  forScrollView:(UIScrollView *)scrollView
                    contentSize:(CGSize)contentSize {
  if (scrollView == nil) {
    return offset;
  }
  CGFloat minOffsetY = -scrollView.contentInset.top;
  CGFloat maxOffsetY = MAX(minOffsetY, contentSize.height - scrollView.bounds.size.height +
                                           scrollView.contentInset.bottom);
  offset.y = MIN(MAX(offset.y, minOffsetY), maxOffsetY);
  return offset;
}

- (CGPoint)clampedContentOffset:(CGPoint)offset forScrollView:(UIScrollView *)scrollView {
  if (scrollView == nil) {
    return offset;
  }
  return [self clampedContentOffset:offset
                      forScrollView:scrollView
                        contentSize:scrollView.contentSize];
}

- (BOOL)applyWithOwner:(nullable id)owner
             inputView:(nullable UIView *)inputView
        keyboardHeight:(CGFloat)keyboardHeight
               spacing:(CGFloat)spacing
        targetDistance:(CGFloat)targetDistance
     animationDuration:(NSTimeInterval)animationDuration {
  if (inputView == nil) {
    return NO;
  }
  LynxUI *scrollUI = [self scrollUIForOwner:owner];
  if (scrollUI == nil) {
    return NO;
  }
  if (self.scrollUI != nil && self.scrollUI != scrollUI) {
    [self clear];
  }
  self.scrollUI = scrollUI;

  UIScrollView *scrollView = [self scrollViewForScrollUI:scrollUI];
  if (scrollView == nil || scrollView.bounds.size.height <= 0) {
    return YES;
  }
  [self captureOriginalOffsetIfNeeded:scrollView];

  CGFloat baseContentHeight = [self baseContentHeightForScrollView:scrollView];
  CGFloat minOffsetY = -scrollView.contentInset.top;
  CGFloat rawMaxOffsetYWithoutExtra =
      baseContentHeight - scrollView.bounds.size.height + scrollView.contentInset.bottom;
  CGFloat maxOffsetYWithoutExtra = MAX(minOffsetY, rawMaxOffsetYWithoutExtra);
  CGFloat currentOffsetY = scrollView.contentOffset.y;
  CGRect inputRectInScrollView = [inputView convertRect:inputView.bounds toView:scrollView];
  CGFloat visibleHeight = scrollView.bounds.size.height - scrollView.contentInset.bottom;
  if (keyboardHeight > 0 && scrollView.window != nil) {
    CGRect scrollViewRectInScreen = [scrollView convertRect:scrollView.bounds toView:nil];
    CGFloat keyboardTop = UIScreen.mainScreen.bounds.size.height - keyboardHeight;
    visibleHeight = MIN(visibleHeight, keyboardTop - CGRectGetMinY(scrollViewRectInScreen));
  }
  CGFloat coveredHeight = [self coveredHeightForScrollView:scrollView visibleHeight:visibleHeight];
  CGFloat desiredOffsetY = CGRectGetMaxY(inputRectInScrollView) + spacing - MAX(0, visibleHeight);

  LynxUI *lowestInputUI = [self lowestInputUIInScrollUI:scrollUI scrollView:scrollView];
  UIView *lowestInputView = lowestInputUI.view;
  CGFloat lowestSpacing = spacing;
  LynxKeyboardAvoidingTargetInfo *lowestTargetInfo = [self targetInfoForOwner:lowestInputUI];
  if (lowestTargetInfo != nil) {
    lowestSpacing = lowestTargetInfo.spacing;
  }
  if (lowestInputView == nil) {
    lowestInputView = inputView;
  }
  CGRect lowestInputRectInScrollView = [lowestInputView convertRect:lowestInputView.bounds
                                                             toView:scrollView];
  CGFloat lowestDesiredOffsetY =
      CGRectGetMaxY(lowestInputRectInScrollView) + lowestSpacing - MAX(0, visibleHeight);
  CGFloat requiredExtra = coveredHeight;
  if (lowestDesiredOffsetY > maxOffsetYWithoutExtra) {
    requiredExtra = MAX(requiredExtra, lowestDesiredOffsetY - rawMaxOffsetYWithoutExtra);
  }
  [self setContentHeightExtra:requiredExtra forScrollView:scrollView];

  CGFloat maxOffsetY = MAX(minOffsetY, rawMaxOffsetYWithoutExtra + requiredExtra);
  CGFloat targetOffsetY = currentOffsetY;
  if (targetDistance > kLynxKeyboardAvoidingScrollEpsilon) {
    targetOffsetY = MIN(MAX(desiredOffsetY, minOffsetY), maxOffsetY);
  }
  if (fabs(targetOffsetY - currentOffsetY) < kLynxKeyboardAvoidingScrollEpsilon) {
    return YES;
  }

  CGPoint targetOffset = scrollView.contentOffset;
  targetOffset.y = targetOffsetY;
  if (animationDuration <= 0) {
    [scrollView setContentOffset:targetOffset animated:NO];
    return YES;
  }
  [scrollView setContentOffset:targetOffset animated:YES];
  return YES;
}

@end
