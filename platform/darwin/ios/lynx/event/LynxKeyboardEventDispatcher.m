// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBaseScrollView+Auto.h>
#import <Lynx/LynxBaseScrollView.h>
#import <Lynx/LynxKeyboardEventDispatcher.h>
#include <Lynx/LynxLog.h>
#import <Lynx/LynxUI.h>
#import <Lynx/LynxUIScrollViewInternal.h>
#import <Lynx/LynxUIScroller.h>
#import <Lynx/LynxWeakProxy.h>
#import <UIKit/UIKit.h>
#include <math.h>

#define KEYBOARD_STATUS_CHANGED "keyboardstatuschanged"
static const NSTimeInterval kLynxKeyboardDefaultAnimationDuration = 0.3;
static const CGFloat kLynxKeyboardAvoidEpsilon = 0.5;

@protocol LynxKeyboardEventAnimationObserver <LynxKeyboardEventObserver>

- (void)keyboardWillShow:(CGFloat)keyboardHeight
       animationDuration:(NSTimeInterval)duration
          animationCurve:(UIViewAnimationCurve)curve;
- (void)keyboardWillHideWithAnimationDuration:(NSTimeInterval)duration
                               animationCurve:(UIViewAnimationCurve)curve;

@end

@interface LynxUI (LynxKeyboardAvoidingTraversal)
- (NSArray<LynxUI *> *)children;
@end

@interface LynxKeyboardAvoidingTargetInfo : NSObject
@property(nonatomic, assign) BOOL avoidKeyboard;
@property(nonatomic, assign) CGFloat spacing;
@end

@implementation LynxKeyboardAvoidingTargetInfo
@end

@interface LynxKeyboardAvoidingContext : NSObject

@property(nonatomic, weak) id activeOwner;
@property(nonatomic, weak) id lastEventOwner;
@property(nonatomic, weak) UIView *rootView;
@property(nonatomic, weak) UIView *inputView;
@property(nonatomic, weak) LynxUI *keyboardAvoidingScrollUI;
@property(nonatomic, strong)
    NSMapTable<id, LynxKeyboardAvoidingTargetInfo *> *keyboardAvoidingTargets;
@property(nonatomic, copy) dispatch_block_t finalHideBlock;
@property(nonatomic, assign) CGFloat keyboardAvoidingScrollBaseContentHeight;
@property(nonatomic, assign) CGFloat keyboardAvoidingScrollContentHeightExtra;
@property(nonatomic, assign) CGFloat keyboardAvoidingScrollAppliedContentHeight;
@property(nonatomic, assign) CGFloat spacing;
@property(nonatomic, assign) CGFloat keyboardHeight;
@property(nonatomic, assign) CGFloat currentAvoidDistance;
@property(nonatomic, assign) BOOL avoidKeyboard;
@property(nonatomic, assign) BOOL keyboardVisible;
@property(nonatomic, assign) NSUInteger keyboardHideGeneration;
@property(nonatomic, assign) NSTimeInterval animationDuration;
@property(nonatomic, assign) UIViewAnimationCurve animationCurve;

- (void)inputDidBeginEditingForOwner:(id)owner
                            rootView:(UIView *)rootView
                           inputView:(UIView *)inputView
                       avoidKeyboard:(BOOL)avoidKeyboard
                             spacing:(CGFloat)spacing;
- (void)inputDidEndEditingForOwner:(id)owner finalHideBlock:(dispatch_block_t)finalHideBlock;
- (void)inputDidLayoutForOwner:(id)owner
                      rootView:(UIView *)rootView
                     inputView:(UIView *)inputView
                 avoidKeyboard:(BOOL)avoidKeyboard
                       spacing:(CGFloat)spacing;
- (void)avoidKeyboardPropsDidChangeForOwner:(id)owner
                                   rootView:(UIView *)rootView
                                  inputView:(UIView *)inputView
                              avoidKeyboard:(BOOL)avoidKeyboard
                                    spacing:(CGFloat)spacing;
- (void)keyboardWillShowForOwner:(id)owner
                        rootView:(UIView *)rootView
                       inputView:(UIView *)inputView
                   avoidKeyboard:(BOOL)avoidKeyboard
                         spacing:(CGFloat)spacing
                          height:(CGFloat)height
                        duration:(NSTimeInterval)duration
                           curve:(UIViewAnimationCurve)curve;
- (void)keyboardWillHideForOwner:(id)owner
                        duration:(NSTimeInterval)duration
                           curve:(UIViewAnimationCurve)curve
                  finalHideBlock:(dispatch_block_t)finalHideBlock;
- (void)updateActiveOwner:(id)owner
                 rootView:(UIView *)rootView
                inputView:(UIView *)inputView
            avoidKeyboard:(BOOL)avoidKeyboard
                  spacing:(CGFloat)spacing;
- (void)updateAnimationWithDuration:(NSTimeInterval)duration curve:(UIViewAnimationCurve)curve;
- (void)updateAvoidDistance;
- (void)applyAvoidDistance:(CGFloat)targetDistance;
- (void)applyRootAvoidDistance:(CGFloat)targetDistance;
- (BOOL)applyScrollViewAvoidDistance:(CGFloat)targetDistance;
- (LynxUI *)keyboardAvoidingScrollUIForActiveOwner;
- (LynxUI *)keyboardAvoidingScrollUIForOwner:(id)owner;
- (BOOL)isVerticalKeyboardAvoidingScrollUI:(LynxUI *)ui;
- (UIScrollView *)scrollViewForKeyboardAvoidingScrollUI:(LynxUI *)scrollUI;
- (LynxUI *)lowestKeyboardAvoidingInputUIInScrollUI:(LynxUI *)scrollUI
                                         scrollView:(UIScrollView *)scrollView;
- (void)saveKeyboardAvoidingTargetForOwner:(id)owner
                             avoidKeyboard:(BOOL)avoidKeyboard
                                   spacing:(CGFloat)spacing;
- (LynxKeyboardAvoidingTargetInfo *)keyboardAvoidingTargetInfoForOwner:(id)owner;
- (CGFloat)keyboardAvoidingBaseContentHeightForScrollView:(UIScrollView *)scrollView;
- (void)setKeyboardAvoidingContentHeight:(CGFloat)height forScrollView:(UIScrollView *)scrollView;
- (void)setKeyboardAvoidingContentHeightExtra:(CGFloat)extra
                                forScrollView:(UIScrollView *)scrollView;
- (void)resetKeyboardAvoidingScrollContentState;
- (void)clearKeyboardAvoidingScrollView;

@end

@implementation LynxKeyboardAvoidingContext

- (instancetype)init {
  if (self = [super init]) {
    _animationDuration = kLynxKeyboardDefaultAnimationDuration;
    _animationCurve = UIViewAnimationCurveEaseInOut;
    _keyboardAvoidingTargets = [NSMapTable weakToStrongObjectsMapTable];
  }
  return self;
}

- (void)inputDidBeginEditingForOwner:(id)owner
                            rootView:(UIView *)rootView
                           inputView:(UIView *)inputView
                       avoidKeyboard:(BOOL)avoidKeyboard
                             spacing:(CGFloat)spacing {
  [self updateActiveOwner:owner
                 rootView:rootView
                inputView:inputView
            avoidKeyboard:avoidKeyboard
                  spacing:spacing];
  self.lastEventOwner = owner;
  if (self.keyboardVisible || self.keyboardHeight > 0) {
    [self updateAvoidDistance];
  }
}

- (void)inputDidEndEditingForOwner:(id)owner finalHideBlock:(dispatch_block_t)finalHideBlock {
  if (owner == self.activeOwner) {
    self.lastEventOwner = owner;
    self.finalHideBlock = finalHideBlock;
    dispatch_async(dispatch_get_main_queue(), ^{
      if (owner != self.activeOwner) {
        return;
      }
      if (self.inputView != nil && self.inputView.isFirstResponder) {
        return;
      }
      [self clearKeyboardAvoidingScrollView];
    });
  }
}

- (void)inputDidLayoutForOwner:(id)owner
                      rootView:(UIView *)rootView
                     inputView:(UIView *)inputView
                 avoidKeyboard:(BOOL)avoidKeyboard
                       spacing:(CGFloat)spacing {
  [self saveKeyboardAvoidingTargetForOwner:owner avoidKeyboard:avoidKeyboard spacing:spacing];
  if (owner == self.activeOwner && inputView.isFirstResponder &&
      (self.keyboardVisible || self.keyboardHeight > 0)) {
    [self updateActiveOwner:owner
                   rootView:rootView
                  inputView:inputView
              avoidKeyboard:avoidKeyboard
                    spacing:spacing];
    [self updateAvoidDistance];
  }
}

- (void)avoidKeyboardPropsDidChangeForOwner:(id)owner
                                   rootView:(UIView *)rootView
                                  inputView:(UIView *)inputView
                              avoidKeyboard:(BOOL)avoidKeyboard
                                    spacing:(CGFloat)spacing {
  [self saveKeyboardAvoidingTargetForOwner:owner avoidKeyboard:avoidKeyboard spacing:spacing];
  if (owner == self.activeOwner && inputView.isFirstResponder &&
      (self.keyboardVisible || self.keyboardHeight > 0)) {
    [self updateActiveOwner:owner
                   rootView:rootView
                  inputView:inputView
              avoidKeyboard:avoidKeyboard
                    spacing:spacing];
    [self updateAvoidDistance];
  }
}

- (void)keyboardWillShowForOwner:(id)owner
                        rootView:(UIView *)rootView
                       inputView:(UIView *)inputView
                   avoidKeyboard:(BOOL)avoidKeyboard
                         spacing:(CGFloat)spacing
                          height:(CGFloat)height
                        duration:(NSTimeInterval)duration
                           curve:(UIViewAnimationCurve)curve {
  self.keyboardVisible = YES;
  self.keyboardHeight = height;
  [self updateAnimationWithDuration:duration curve:curve];
  [self updateActiveOwner:owner
                 rootView:rootView
                inputView:inputView
            avoidKeyboard:avoidKeyboard
                  spacing:spacing];
  self.lastEventOwner = owner;
  [self updateAvoidDistance];
}

- (void)keyboardWillHideForOwner:(id)owner
                        duration:(NSTimeInterval)duration
                           curve:(UIViewAnimationCurve)curve
                  finalHideBlock:(dispatch_block_t)finalHideBlock {
  if (owner != self.activeOwner && owner != self.lastEventOwner) {
    return;
  }
  [self updateAnimationWithDuration:duration curve:curve];
  if (finalHideBlock != nil) {
    self.finalHideBlock = finalHideBlock;
  }
  NSUInteger generation = ++self.keyboardHideGeneration;
  dispatch_async(dispatch_get_main_queue(), ^{
    if (generation != self.keyboardHideGeneration) {
      return;
    }
    if (self.inputView != nil && self.inputView.isFirstResponder) {
      self.keyboardVisible = YES;
      [self updateAvoidDistance];
      return;
    }
    dispatch_block_t block = self.finalHideBlock;
    self.keyboardVisible = NO;
    self.keyboardHeight = 0;
    [self clearKeyboardAvoidingScrollView];
    [self applyRootAvoidDistance:0];
    if (block != nil) {
      block();
    }
    self.activeOwner = nil;
    self.lastEventOwner = nil;
    self.inputView = nil;
    self.finalHideBlock = nil;
  });
}

- (void)updateActiveOwner:(id)owner
                 rootView:(UIView *)rootView
                inputView:(UIView *)inputView
            avoidKeyboard:(BOOL)avoidKeyboard
                  spacing:(CGFloat)spacing {
  [self saveKeyboardAvoidingTargetForOwner:owner avoidKeyboard:avoidKeyboard spacing:spacing];
  if (owner != self.activeOwner) {
    LynxUI *newScrollUI = avoidKeyboard ? [self keyboardAvoidingScrollUIForOwner:owner] : nil;
    if (self.keyboardAvoidingScrollUI != nil && self.keyboardAvoidingScrollUI != newScrollUI) {
      [self clearKeyboardAvoidingScrollView];
    }
  }
  self.activeOwner = owner;
  self.rootView = rootView;
  self.inputView = inputView;
  self.avoidKeyboard = avoidKeyboard;
  self.spacing = spacing;
}

- (void)updateAnimationWithDuration:(NSTimeInterval)duration curve:(UIViewAnimationCurve)curve {
  self.animationDuration = duration > 0 ? duration : kLynxKeyboardDefaultAnimationDuration;
  self.animationCurve = curve;
}

- (CGFloat)targetAvoidDistance {
  if (!self.avoidKeyboard || self.inputView == nil || !self.inputView.isFirstResponder ||
      self.keyboardHeight <= 0) {
    return 0;
  }
  CGRect inputRect = [self.inputView convertRect:self.inputView.bounds toView:nil];
  CGFloat inputBottomBeforeAvoid = CGRectGetMaxY(inputRect) + self.currentAvoidDistance;
  CGFloat bottomToScreen = UIScreen.mainScreen.bounds.size.height - inputBottomBeforeAvoid;
  CGFloat gap = self.keyboardHeight - bottomToScreen + self.spacing;
  return MAX(0, gap);
}

- (void)updateAvoidDistance {
  [self applyAvoidDistance:[self targetAvoidDistance]];
}

- (void)applyAvoidDistance:(CGFloat)targetDistance {
  if (!self.avoidKeyboard) {
    [self clearKeyboardAvoidingScrollView];
    [self applyRootAvoidDistance:0];
    return;
  }
  if ([self applyScrollViewAvoidDistance:targetDistance]) {
    return;
  }
  [self clearKeyboardAvoidingScrollView];
  [self applyRootAvoidDistance:targetDistance];
}

- (LynxUI *)keyboardAvoidingScrollUIForActiveOwner {
  return [self keyboardAvoidingScrollUIForOwner:self.activeOwner];
}

- (LynxUI *)keyboardAvoidingScrollUIForOwner:(id)owner {
  if (![owner isKindOfClass:LynxUI.class]) {
    return nil;
  }
  LynxUI *ui = [(LynxUI *)owner getParent];
  while (ui != nil) {
    if ([self isVerticalKeyboardAvoidingScrollUI:ui]) {
      return ui;
    }
    ui = [ui getParent];
  }
  return nil;
}

- (BOOL)isVerticalKeyboardAvoidingScrollUI:(LynxUI *)ui {
  if ([ui isKindOfClass:LynxUIScroller.class]) {
    return ((LynxUIScroller *)ui).enableScrollY;
  }
  if ([ui isKindOfClass:LynxUIScrollViewInternal.class]) {
    UIView *view = ui.view;
    return [view isKindOfClass:LynxBaseScrollView.class] && ((LynxBaseScrollView *)view).vertical;
  }
  return NO;
}

- (UIScrollView *)scrollViewForKeyboardAvoidingScrollUI:(LynxUI *)scrollUI {
  UIView *view = scrollUI.view;
  return [view isKindOfClass:UIScrollView.class] ? (UIScrollView *)view : nil;
}

- (LynxUI *)lowestKeyboardAvoidingInputUIInScrollUI:(LynxUI *)scrollUI
                                         scrollView:(UIScrollView *)scrollView {
  LynxUI *lowestInputUI = nil;
  CGFloat lowestInputBottom = -CGFLOAT_MAX;
  NSMutableArray<LynxUI *> *pendingUIs = [NSMutableArray arrayWithObject:scrollUI];
  while (pendingUIs.count > 0) {
    LynxUI *ui = pendingUIs.lastObject;
    [pendingUIs removeLastObject];
    if (ui != scrollUI && [self isVerticalKeyboardAvoidingScrollUI:ui]) {
      continue;
    }

    UIView *view = ui.view;
    LynxKeyboardAvoidingTargetInfo *targetInfo = [self keyboardAvoidingTargetInfoForOwner:ui];
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

- (void)saveKeyboardAvoidingTargetForOwner:(id)owner
                             avoidKeyboard:(BOOL)avoidKeyboard
                                   spacing:(CGFloat)spacing {
  if (owner == nil) {
    return;
  }
  LynxKeyboardAvoidingTargetInfo *targetInfo = [[LynxKeyboardAvoidingTargetInfo alloc] init];
  targetInfo.avoidKeyboard = avoidKeyboard;
  targetInfo.spacing = spacing;
  [self.keyboardAvoidingTargets setObject:targetInfo forKey:owner];
}

- (LynxKeyboardAvoidingTargetInfo *)keyboardAvoidingTargetInfoForOwner:(id)owner {
  if (owner == nil) {
    return nil;
  }
  return [self.keyboardAvoidingTargets objectForKey:owner];
}

- (CGFloat)keyboardAvoidingBaseContentHeightForScrollView:(UIScrollView *)scrollView {
  if (scrollView == nil) {
    return 0;
  }
  CGFloat currentHeight = MAX(0, scrollView.contentSize.height);
  if (self.keyboardAvoidingScrollContentHeightExtra > 0 &&
      fabs(currentHeight - self.keyboardAvoidingScrollAppliedContentHeight) <
          kLynxKeyboardAvoidEpsilon) {
    return MAX(0, self.keyboardAvoidingScrollBaseContentHeight);
  }
  self.keyboardAvoidingScrollBaseContentHeight = currentHeight;
  self.keyboardAvoidingScrollContentHeightExtra = 0;
  self.keyboardAvoidingScrollAppliedContentHeight = currentHeight;
  return currentHeight;
}

- (void)setKeyboardAvoidingContentHeight:(CGFloat)height forScrollView:(UIScrollView *)scrollView {
  if (scrollView == nil) {
    return;
  }
  CGSize contentSize = scrollView.contentSize;
  contentSize.height = MAX(0, height);
  if (fabs(scrollView.contentSize.height - contentSize.height) < kLynxKeyboardAvoidEpsilon) {
    return;
  }
  if ([scrollView isKindOfClass:LynxBaseScrollView.class]) {
    [(LynxBaseScrollView *)scrollView setScrollContentSize:contentSize];
  } else {
    scrollView.contentSize = contentSize;
  }
}

- (void)setKeyboardAvoidingContentHeightExtra:(CGFloat)extra
                                forScrollView:(UIScrollView *)scrollView {
  if (scrollView == nil) {
    return;
  }
  CGFloat baseContentHeight = [self keyboardAvoidingBaseContentHeightForScrollView:scrollView];
  CGFloat contentHeightExtra = MAX(0, extra);
  CGFloat appliedContentHeight = baseContentHeight + contentHeightExtra;
  [self setKeyboardAvoidingContentHeight:appliedContentHeight forScrollView:scrollView];
  self.keyboardAvoidingScrollBaseContentHeight = baseContentHeight;
  self.keyboardAvoidingScrollContentHeightExtra = contentHeightExtra;
  self.keyboardAvoidingScrollAppliedContentHeight = appliedContentHeight;
}

- (void)resetKeyboardAvoidingScrollContentState {
  self.keyboardAvoidingScrollBaseContentHeight = 0;
  self.keyboardAvoidingScrollContentHeightExtra = 0;
  self.keyboardAvoidingScrollAppliedContentHeight = 0;
}

- (void)clearKeyboardAvoidingScrollView {
  LynxUI *scrollUI = self.keyboardAvoidingScrollUI;
  self.keyboardAvoidingScrollUI = nil;
  if (scrollUI == nil) {
    [self resetKeyboardAvoidingScrollContentState];
    return;
  }
  UIScrollView *scrollView = [self scrollViewForKeyboardAvoidingScrollUI:scrollUI];
  if (scrollView == nil) {
    [self resetKeyboardAvoidingScrollContentState];
    return;
  }
  [self setKeyboardAvoidingContentHeightExtra:0 forScrollView:scrollView];
  [self resetKeyboardAvoidingScrollContentState];
  CGFloat minOffsetY = -scrollView.contentInset.top;
  CGFloat maxOffsetY =
      MAX(minOffsetY, scrollView.contentSize.height - scrollView.bounds.size.height +
                          scrollView.contentInset.bottom);
  CGPoint offset = scrollView.contentOffset;
  offset.y = MIN(MAX(offset.y, minOffsetY), maxOffsetY);
  if (fabs(offset.y - scrollView.contentOffset.y) >= kLynxKeyboardAvoidEpsilon) {
    [scrollView setContentOffset:offset animated:NO];
  }
}

- (BOOL)applyScrollViewAvoidDistance:(CGFloat)targetDistance {
  (void)targetDistance;
  LynxUI *scrollUI = [self keyboardAvoidingScrollUIForActiveOwner];
  if (scrollUI == nil) {
    return NO;
  }
  if (self.keyboardAvoidingScrollUI != nil && self.keyboardAvoidingScrollUI != scrollUI) {
    [self clearKeyboardAvoidingScrollView];
  }
  self.keyboardAvoidingScrollUI = scrollUI;
  [self applyRootAvoidDistance:0];

  UIScrollView *scrollView = [self scrollViewForKeyboardAvoidingScrollUI:scrollUI];
  if (scrollView == nil || scrollView.bounds.size.height <= 0) {
    return YES;
  }

  CGFloat baseContentHeight = [self keyboardAvoidingBaseContentHeightForScrollView:scrollView];
  CGFloat minOffsetY = -scrollView.contentInset.top;
  CGFloat rawMaxOffsetYWithoutExtra =
      baseContentHeight - scrollView.bounds.size.height + scrollView.contentInset.bottom;
  CGFloat maxOffsetYWithoutExtra = MAX(minOffsetY, rawMaxOffsetYWithoutExtra);
  CGFloat currentOffsetY = scrollView.contentOffset.y;
  CGRect inputRectInScrollView = [self.inputView convertRect:self.inputView.bounds
                                                      toView:scrollView];
  CGFloat visibleHeight = scrollView.bounds.size.height - scrollView.contentInset.bottom;
  if (self.keyboardHeight > 0 && scrollView.window != nil) {
    CGRect scrollViewRectInScreen = [scrollView convertRect:scrollView.bounds toView:nil];
    CGFloat keyboardTop = UIScreen.mainScreen.bounds.size.height - self.keyboardHeight;
    visibleHeight = MIN(visibleHeight, keyboardTop - CGRectGetMinY(scrollViewRectInScreen));
  }
  CGFloat desiredOffsetY =
      CGRectGetMaxY(inputRectInScrollView) + self.spacing - MAX(0, visibleHeight);

  LynxUI *lowestInputUI = [self lowestKeyboardAvoidingInputUIInScrollUI:scrollUI
                                                             scrollView:scrollView];
  UIView *lowestInputView = lowestInputUI.view;
  CGFloat lowestSpacing = self.spacing;
  LynxKeyboardAvoidingTargetInfo *lowestTargetInfo =
      [self keyboardAvoidingTargetInfoForOwner:lowestInputUI];
  if (lowestTargetInfo != nil) {
    lowestSpacing = lowestTargetInfo.spacing;
  }
  if (lowestInputView == nil) {
    lowestInputView = self.inputView;
  }
  CGRect lowestInputRectInScrollView = [lowestInputView convertRect:lowestInputView.bounds
                                                             toView:scrollView];
  CGFloat lowestDesiredOffsetY =
      CGRectGetMaxY(lowestInputRectInScrollView) + lowestSpacing - MAX(0, visibleHeight);
  CGFloat requiredExtra = 0;
  if (lowestDesiredOffsetY > maxOffsetYWithoutExtra) {
    requiredExtra = MAX(0, lowestDesiredOffsetY - rawMaxOffsetYWithoutExtra);
  }
  [self setKeyboardAvoidingContentHeightExtra:requiredExtra forScrollView:scrollView];

  CGFloat maxOffsetY = MAX(minOffsetY, rawMaxOffsetYWithoutExtra + requiredExtra);
  CGFloat targetOffsetY = MIN(MAX(desiredOffsetY, minOffsetY), maxOffsetY);
  if (fabs(targetOffsetY - currentOffsetY) < kLynxKeyboardAvoidEpsilon) {
    return YES;
  }

  CGPoint targetOffset = scrollView.contentOffset;
  targetOffset.y = targetOffsetY;
  if (self.animationDuration <= 0) {
    [scrollView setContentOffset:targetOffset animated:NO];
    return YES;
  }
  [scrollView setContentOffset:targetOffset animated:YES];
  return YES;
}

- (void)applyRootAvoidDistance:(CGFloat)targetDistance {
  UIView *rootView = self.rootView;
  if (rootView == nil) {
    self.currentAvoidDistance = targetDistance;
    return;
  }
  CGFloat delta = targetDistance - self.currentAvoidDistance;
  if (fabs(delta) < kLynxKeyboardAvoidEpsilon) {
    self.currentAvoidDistance = targetDistance;
    return;
  }
  CGRect targetFrame = rootView.frame;
  targetFrame.origin.y -= delta;
  self.currentAvoidDistance = targetDistance;

  UIViewAnimationOptions options = ((NSUInteger)self.animationCurve << 16) |
                                   UIViewAnimationOptionBeginFromCurrentState |
                                   UIViewAnimationOptionAllowUserInteraction;
  if (self.animationDuration <= 0) {
    rootView.frame = targetFrame;
    return;
  }
  [UIView animateWithDuration:self.animationDuration
                        delay:0
                      options:options
                   animations:^{
                     rootView.frame = targetFrame;
                   }
                   completion:nil];
}

@end

@interface LynxKeyboardEventDispatcher ()
@property(nonatomic, strong) NSMutableDictionary<NSNumber *, LynxWeakProxy *> *observers;
@property(nonatomic, strong) LynxKeyboardAvoidingContext *keyboardAvoidingContext;
@end

@implementation LynxKeyboardEventDispatcher {
  LynxContext *_context;
}

- (instancetype)initWithContext:(LynxContext *)context {
  // Add observer for keyboard popup
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(keyboardWillShow:)
                                               name:UIKeyboardWillShowNotification
                                             object:nil];

  // Add observer for keyboard exist
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(keyboardWillHide:)
                                               name:UIKeyboardWillHideNotification
                                             object:nil];
  _context = context;
  _observers = [NSMutableDictionary dictionary];
  _keyboardAvoidingContext = [[LynxKeyboardAvoidingContext alloc] init];
  return self;
}

- (NSTimeInterval)keyboardAnimationDurationFromNotification:(NSNotification *)notification {
  NSNumber *duration = [notification.userInfo objectForKey:UIKeyboardAnimationDurationUserInfoKey];
  return duration != nil ? duration.doubleValue : kLynxKeyboardDefaultAnimationDuration;
}

- (UIViewAnimationCurve)keyboardAnimationCurveFromNotification:(NSNotification *)notification {
  NSNumber *curve = [notification.userInfo objectForKey:UIKeyboardAnimationCurveUserInfoKey];
  return curve != nil ? curve.integerValue : UIViewAnimationCurveEaseInOut;
}

- (void)keyboardWillShow:(NSNotification *)aNotification {
  /*
   * iOS9-iOS15. Be careful!
     [UIWindow
        [UITextEffectsWindow
            [UIInputSetContainerView
                [UIInputSetHostView]]]]
     and
     [UIRemoteKeyboardWindow
        [UIInputSetContainerView
            [UIInputSetHostView]]]
     both exist.
   * Then, when the UIWindow is portrait and the child uiController is landscape, onWillShowKeyboard
   will get the width and height of UIInputSetHostView in UIWindow.
   * Therefore, should use the rect of UIInputSetHostView of UIRemoteKeyboardWindow instead of that
   in UIKeyboardFrameEndUserInfoKey.
   */
  int systemVersion = [[UIDevice currentDevice] systemVersion].intValue;
  CGRect keyboardRect;
  if (systemVersion < 16) {
    UIView *keyboardView = [self getKeyboardView];
    if (keyboardView == nil) {
      return;
    }
    keyboardRect = keyboardView.frame;
  } else {
    NSDictionary *userInfo = [aNotification userInfo];
    NSValue *aValue = [userInfo objectForKey:UIKeyboardFrameEndUserInfoKey];
    keyboardRect = [aValue CGRectValue];
  }
  int height = keyboardRect.size.height;
  LLog(@"keyboard status is on");
  LLog(@"keyboard height is %d", height);

  NSMutableArray *params = [[NSMutableArray alloc] init];
  NSString *isShow = @"on";
  NSNumber *aHeight = [[NSNumber alloc] initWithInt:height];

  [params addObject:isShow];
  [params addObject:aHeight];
  [_context sendGlobalEvent:@KEYBOARD_STATUS_CHANGED withParams:params];
  NSTimeInterval duration = [self keyboardAnimationDurationFromNotification:aNotification];
  UIViewAnimationCurve curve = [self keyboardAnimationCurveFromNotification:aNotification];
  [_observers enumerateKeysAndObjectsUsingBlock:^(
                  NSNumber *_Nonnull key, LynxWeakProxy *_Nonnull obj, BOOL *_Nonnull stop) {
    id<LynxKeyboardEventObserver> target = obj.target;
    if ([target respondsToSelector:@selector(keyboardWillShow:animationDuration:animationCurve:)]) {
      [(id<LynxKeyboardEventAnimationObserver>)target keyboardWillShow:height
                                                     animationDuration:duration
                                                        animationCurve:curve];
    } else {
      [target keyboardWillShow:height];
    }
  }];
}

- (void)keyboardWillHide:(NSNotification *)aNotification {
  int height = 0;
  LLog(@"keyboard status is off");
  LLog(@"keyboard height is %d", height);

  NSMutableArray *params = [[NSMutableArray alloc] init];
  NSString *isShow = @"off";
  NSNumber *aHeight = [[NSNumber alloc] initWithInt:height];

  [params addObject:isShow];
  [params addObject:aHeight];
  [_context sendGlobalEvent:@KEYBOARD_STATUS_CHANGED withParams:params];
  NSTimeInterval duration = [self keyboardAnimationDurationFromNotification:aNotification];
  UIViewAnimationCurve curve = [self keyboardAnimationCurveFromNotification:aNotification];
  [_observers enumerateKeysAndObjectsUsingBlock:^(
                  NSNumber *_Nonnull key, LynxWeakProxy *_Nonnull obj, BOOL *_Nonnull stop) {
    id<LynxKeyboardEventObserver> target = obj.target;
    if ([target respondsToSelector:@selector(keyboardWillHideWithAnimationDuration:
                                                                    animationCurve:)]) {
      [(id<LynxKeyboardEventAnimationObserver>)target keyboardWillHideWithAnimationDuration:duration
                                                                             animationCurve:curve];
    } else {
      [target keyboardWillHide];
    }
  }];
}

- (UIView *)getKeyboardView {
  // Get the KeyboardWindow
  UIWindow *keyboardWindow = nil;
  int systemVersion = [[UIDevice currentDevice] systemVersion].intValue;
  for (UIWindow *window in [[UIApplication sharedApplication] windows]) {
    NSString *windowName = NSStringFromClass(window.class);
    if (systemVersion < 9) {
      // UITextEffectsWindow
      if (windowName.length != 19) continue;
      if (![windowName hasPrefix:@"UI"]) continue;
      if (![windowName hasSuffix:@"TextEffectsWindow"]) continue;
    } else {
      // UIRemoteKeyboardWindow
      if (windowName.length != 22) continue;
      if (![windowName hasPrefix:@"UI"]) continue;
      if (![windowName hasSuffix:@"RemoteKeyboardWindow"]) continue;
    }
    keyboardWindow = window;
    break;
  }
  if (keyboardWindow == nil) {
    LLog(@"Can not get KeyboardWindow");
    return nil;
  }

  // Get the KeyboardView
  UIView *keyboardView = nil;
  if (systemVersion < 8) {
    // UIPeripheralHostView
    for (UIView *view in [keyboardWindow subviews]) {
      NSString *viewName = NSStringFromClass(view.class);
      if (viewName.length != 20) continue;
      if (![viewName hasPrefix:@"UI"]) continue;
      if (![viewName hasSuffix:@"PeripheralHostView"]) continue;
      keyboardView = view;
      break;
    }
  } else {
    // UIInputSetContainerView
    for (UIView *view in [keyboardWindow subviews]) {
      NSString *viewName = NSStringFromClass(view.class);
      if (viewName.length != 23) continue;
      if (![viewName hasPrefix:@"UI"]) continue;
      if (![viewName hasSuffix:@"InputSetContainerView"]) continue;
      for (UIView *subView in [view subviews]) {
        // UIInputSetHostView
        NSString *subViewName = NSStringFromClass(subView.class);
        if (subViewName.length != 18) continue;
        if (![subViewName hasPrefix:@"UI"]) continue;
        if (![subViewName hasSuffix:@"InputSetHostView"]) continue;
        keyboardView = subView;
        break;
      }
      break;
    }
  }
  if (keyboardView == nil) {
    LLog(@"Can not get KeyboardView");
  }
  return keyboardView;
}

- (void)addKeyboardEventObserver:(id<LynxKeyboardEventObserver>)observer {
  if ([observer conformsToProtocol:@protocol(LynxKeyboardEventObserver)]) {
    [_observers setObject:[LynxWeakProxy proxyWithTarget:observer] forKey:@((uintptr_t)observer)];
  }
}

- (void)inputDidBeginEditingForOwner:(id)owner
                            rootView:(UIView *)rootView
                           inputView:(UIView *)inputView
                       avoidKeyboard:(BOOL)avoidKeyboard
                             spacing:(CGFloat)spacing {
  [self.keyboardAvoidingContext inputDidBeginEditingForOwner:owner
                                                    rootView:rootView
                                                   inputView:inputView
                                               avoidKeyboard:avoidKeyboard
                                                     spacing:spacing];
}

- (void)inputDidEndEditingForOwner:(id)owner finalHideBlock:(dispatch_block_t)block {
  [self.keyboardAvoidingContext inputDidEndEditingForOwner:owner finalHideBlock:block];
}

- (void)inputDidLayoutForOwner:(id)owner
                      rootView:(UIView *)rootView
                     inputView:(UIView *)inputView
                 avoidKeyboard:(BOOL)avoidKeyboard
                       spacing:(CGFloat)spacing {
  [self.keyboardAvoidingContext inputDidLayoutForOwner:owner
                                              rootView:rootView
                                             inputView:inputView
                                         avoidKeyboard:avoidKeyboard
                                               spacing:spacing];
}

- (void)avoidKeyboardPropsDidChangeForOwner:(id)owner
                                   rootView:(UIView *)rootView
                                  inputView:(UIView *)inputView
                              avoidKeyboard:(BOOL)avoidKeyboard
                                    spacing:(CGFloat)spacing {
  [self.keyboardAvoidingContext avoidKeyboardPropsDidChangeForOwner:owner
                                                           rootView:rootView
                                                          inputView:inputView
                                                      avoidKeyboard:avoidKeyboard
                                                            spacing:spacing];
}

- (void)keyboardWillShowForOwner:(id)owner
                        rootView:(UIView *)rootView
                       inputView:(UIView *)inputView
                   avoidKeyboard:(BOOL)avoidKeyboard
                         spacing:(CGFloat)spacing
                          height:(CGFloat)height
                        duration:(NSTimeInterval)duration
                           curve:(UIViewAnimationCurve)curve {
  [self.keyboardAvoidingContext keyboardWillShowForOwner:owner
                                                rootView:rootView
                                               inputView:inputView
                                           avoidKeyboard:avoidKeyboard
                                                 spacing:spacing
                                                  height:height
                                                duration:duration
                                                   curve:curve];
}

- (void)keyboardWillHideForOwner:(id)owner
                        duration:(NSTimeInterval)duration
                           curve:(UIViewAnimationCurve)curve
                  finalHideBlock:(dispatch_block_t)block {
  [self.keyboardAvoidingContext keyboardWillHideForOwner:owner
                                                duration:duration
                                                   curve:curve
                                          finalHideBlock:block];
}

@end
