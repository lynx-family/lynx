// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxEventHandler.h>
#import <Lynx/LynxUI.h>
#import <UIKit/UIKit.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSUInteger, LynxOverlayMode) {
  LynxOverlayModeWindow = 0,     // Attach to Window
  LynxOverlayModePage,           // Attach to NavigationController
  LynxOverlayModeTopController,  // Attach to TopController
  LynxOverlayModeCustom,         // Attach to custom ViewController
};

@protocol LynxOverlayViewDelegate <NSObject>
- (BOOL)forbidPanGesture;
- (BOOL)eventPassed:(CGPoint)point;
- (void)requestClose:(NSDictionary *)info;
- (void)overlayTouched:(NSString *)name
                 point:(CGPoint)point
                 state:(UIGestureRecognizerState)state
              velocity:(CGPoint)velocity;
- (LynxUI *)overlayRootUI;
- (UIScrollView *)nestScrollView;
- (NSInteger)getSign;

@end

/**
 * LynxOverlayContainer is the root view of Overlay.
 * It recognizes gestures, and sent touch events to Lepus.
 */
@interface LynxOverlayContainer : UIView <UIGestureRecognizerDelegate>
@property(nonatomic, strong) LynxEventHandler *eventHandler;
@property(nonatomic, weak) id<LynxOverlayViewDelegate> uiDelegate;

- (void)enableTouchOverlayEvent:(BOOL)enable;
@end

NS_ASSUME_NONNULL_END
