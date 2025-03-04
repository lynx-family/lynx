// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxImpressionView.h"
#import "LynxComponentRegistry.h"
#import "LynxLayoutStyle.h"
#import "LynxPropsProcessor.h"
#import "LynxView.h"

NSNotificationName const LynxImpressionWillManualExposureNotification =
    @"LynxImpressionWillManualExposureNotification";
NSNotificationName const LynxImpressionLynxViewIDNotificationKey =
    @"LynxImpressionLynxViewIDNotificationKey";
NSNotificationName const LynxImpressionStatusNotificationKey =
    @"LynxImpressionStatusNotificationKey";
NSNotificationName const LynxImpressionForceImpressionBoolKey =
    @"LynxImpressionForceImpressionBoolKey";

@protocol LynxInnerImpressionViewDelegate <NSObject>

@optional
- (void)impression;
- (void)exit;

@end

@interface LynxInnerImpressionView ()

@property(nonatomic, weak) id<LynxInnerImpressionViewDelegate> delegate;

@end

@implementation LynxInnerImpressionView

- (void)didMoveToWindow {
  [super didMoveToWindow];

  if (self.window) {
    if ([self.superview respondsToSelector:@selector(shouldManualExposure)]) {
      if ([(id<LynxImpressionParentView>)self.superview shouldManualExposure]) {
        return;
      }
    }

    [self impression];
  } else {
    [self exit];
  }
}

- (void)impression {
  if (self.onScreen) {
    return;
  }

  _onScreen = YES;

  if ([self.delegate respondsToSelector:@selector(impression)]) {
    [self.delegate impression];
  }
}

- (void)exit {
  if (!self.onScreen) {
    return;
  }

  _onScreen = NO;

  if ([self.delegate respondsToSelector:@selector(exit)]) {
    [self.delegate exit];
  }
}

@end

@interface LynxImpressionView () <LynxInnerImpressionViewDelegate>

@end

@implementation LynxImpressionView

#if LYNX_LAZY_LOAD
LYNX_LAZY_REGISTER_UI("impression-view")
#else
LYNX_REGISTER_UI("impression-view")
#endif

- (UIView *)createView {
  LynxInnerImpressionView *view = [LynxInnerImpressionView new];
  view.delegate = self;
  [[NSNotificationCenter defaultCenter]
      addObserver:self
         selector:@selector(lynxImpressionWillManualExposureNotification:)
             name:LynxImpressionWillManualExposureNotification
           object:nil];
  return view;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
}

LYNX_PROP_SETTER("impression-percent", impressionPercent, NSInteger) {
  self.view.impressionPercent = MIN(1, MAX((value / 100.f), 0));
}

#pragma mark - LynxImpressionInnerViewDelegate

- (void)impression {
  LynxCustomEvent *event = [[LynxDetailEvent alloc] initWithName:@"impression"
                                                      targetSign:[self sign]
                                                          detail:@{}];
  [self.context.eventEmitter sendCustomEvent:event];
}

- (void)exit {
  LynxCustomEvent *event = [[LynxDetailEvent alloc] initWithName:@"exit"
                                                      targetSign:[self sign]
                                                          detail:@{}];
  [self.context.eventEmitter sendCustomEvent:event];
}

#pragma mark -

- (void)lynxImpressionWillManualExposureNotification:(NSNotification *)noti {
  if ([self.view.superview respondsToSelector:@selector(shouldManualExposure)]) {
    if ([(id<LynxImpressionParentView>)self.view.superview shouldManualExposure]) {
      return;
    }
  }

  if (![self.context.rootView isKindOfClass:LynxView.class]) {
    return;
  }

  if ([noti.userInfo[LynxImpressionStatusNotificationKey] isEqualToString:@"show"]) {
    [self.view impression];
  } else if ([noti.userInfo[LynxImpressionStatusNotificationKey] isEqualToString:@"hide"]) {
    [self.view exit];
  }
}

@end
