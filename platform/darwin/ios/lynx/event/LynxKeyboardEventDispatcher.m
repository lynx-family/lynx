// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxKeyboardEventDispatcher.h>
#include <Lynx/LynxLog.h>
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

@interface LynxKeyboardAvoidingContext : NSObject

@property(nonatomic, weak) id activeOwner;
@property(nonatomic, weak) id lastEventOwner;
@property(nonatomic, weak) UIView *rootView;
@property(nonatomic, weak) UIView *inputView;
@property(nonatomic, copy) dispatch_block_t finalHideBlock;
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

@end

@implementation LynxKeyboardAvoidingContext

- (instancetype)init {
  if (self = [super init]) {
    _animationDuration = kLynxKeyboardDefaultAnimationDuration;
    _animationCurve = UIViewAnimationCurveEaseInOut;
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
  }
}

- (void)inputDidLayoutForOwner:(id)owner
                      rootView:(UIView *)rootView
                     inputView:(UIView *)inputView
                 avoidKeyboard:(BOOL)avoidKeyboard
                       spacing:(CGFloat)spacing {
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
    [self applyAvoidDistance:0];
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
