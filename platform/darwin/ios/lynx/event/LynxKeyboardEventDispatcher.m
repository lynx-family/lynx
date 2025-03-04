// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxKeyboardEventDispatcher.h"
#import <UIKit/UIKit.h>
#include "LynxLog.h"

#define KEYBOARD_STATUS_CHANGED "keyboardstatuschanged"

@interface LynxKeyboardEventDispatcher ()
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
  return self;
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

@end
