// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxDevToolToast.h"
#import <Lynx/LynxUIKitAPIAdapter.h>

@interface LynxDevToolToast ()

#if OS_IOS
@property(readwrite, nonatomic) UIAlertController* alert;
#elif OS_OSX
@property(readwrite, nonatomic) NSAlert* alert;
#endif

@end

@implementation LynxDevToolToast

- (instancetype)initWithMessage:(NSString*)message {
  if (self = [super init]) {
#if OS_IOS
    self.alert = [UIAlertController alertControllerWithTitle:nil
                                                     message:message
                                              preferredStyle:UIAlertControllerStyleAlert];
#elif OS_OSX
    self.alert = [[NSAlert alloc] init];
    [self.alert setInformativeText:message];
#endif
  }
  return self;
}

- (void)show {
#if OS_IOS
  UIViewController* controller = [LynxUIKitAPIAdapter getKeyWindow].rootViewController;
  UIViewController* presentedController = controller.presentedViewController;
  while (presentedController && ![presentedController isBeingDismissed]) {
    controller = presentedController;
    presentedController = controller.presentedViewController;
  }
  [controller presentViewController:self.alert animated:YES completion:nil];

  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC), dispatch_get_main_queue(), ^{
    [self.alert.presentingViewController dismissViewControllerAnimated:YES completion:nil];
  });
#elif OS_OSX
  [self.alert runModal];
#endif
}

+ (void)showToast:(NSString*)message {
  if ([NSThread isMainThread]) {
    [LynxDevToolToast showToastOnMainThread:message];
  } else {
    dispatch_async(dispatch_get_main_queue(), ^{
      [LynxDevToolToast showToastOnMainThread:message];
    });
  }
}

+ (void)showToastOnMainThread:(NSString*)message {
  if (message && [message length] > 0) {
    LynxDevToolToast* toast = [[LynxDevToolToast alloc] initWithMessage:message];
    [toast show];
  }
}

@end
