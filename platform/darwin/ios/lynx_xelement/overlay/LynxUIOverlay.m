// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxEventHandler.h>
#import <Lynx/LynxGlobalObserver.h>
#import <Lynx/LynxLog.h>
#import <Lynx/LynxNativeLayoutNode.h>
#import <Lynx/LynxPropsProcessor.h>
#import <Lynx/LynxRootUI.h>
#import <Lynx/LynxShadowNodeOwner.h>
#import <Lynx/LynxSizeValue.h>
#import <Lynx/LynxTemplateRender+Internal.h>
#import <Lynx/LynxTouchHandler.h>
#import <Lynx/LynxUI+Internal.h>
#import <Lynx/LynxUI+Private.h>
#import <Lynx/LynxUIKitAPIAdapter.h>
#import <Lynx/LynxUIMethodProcessor.h>
#import <Lynx/LynxView+Internal.h>
#import <Lynx/LynxViewVisibleHelper.h>
#import <Lynx/UIView+Lynx.h>
#import <XElement/LynxOverlayContainer.h>
#import <XElement/LynxOverlayGlobalManager.h>
#import <XElement/LynxUIOverlay.h>

@interface LynxUIOverlayShadowNode ()

@property(atomic, assign) CGRect customRect;

@end

@implementation LynxUIOverlayShadowNode

LYNX_LAZY_REGISTER_SHADOW_NODE("overlay")

- (instancetype)initWithSign:(NSInteger)sign tagName:(NSString *)tagName {
  if (self = [super initWithSign:sign tagName:tagName]) {
    // enable custom-layout by default
    self.hasCustomLayout = YES;
    self.customRect = CGRectNull;
  }
  return self;
}

/**
 *  Customize Overlay's  measure strategy
 */
- (MeasureResult)customMeasureLayoutNode:(nonnull MeasureParam *)param
                          measureContext:(nullable MeasureContext *)context {
  [self.children enumerateObjectsUsingBlock:^(LynxShadowNode *_Nonnull obj, NSUInteger idx,
                                              BOOL *_Nonnull stop) {
    if ([obj isKindOfClass:[LynxNativeLayoutNode class]]) {
      CGRect bounds = [self getMeasureBounds];
      // Overlay's frame is always equal to the screen, its child should be measured correctly
      MeasureParam *childParam =
          [[MeasureParam alloc] initWithWidth:bounds.size.width - obj.style.computedMarginLeft -
                                              obj.style.computedMarginRight
                                    WidthMode:LynxMeasureModeDefinite
                                       Height:bounds.size.height - obj.style.computedMarginTop -
                                              obj.style.computedMarginBottom
                                   HeightMode:LynxMeasureModeDefinite];
      LynxNativeLayoutNode *child = (LynxNativeLayoutNode *)obj;
      [child measureWithMeasureParam:childParam MeasureContext:context];
    }
    *stop = YES;
  }];
  // Overlay itself will never take up any space
  return (MeasureResult){CGSizeZero};
}

- (void)customAlignLayoutNode:(nonnull AlignParam *)param
                 alignContext:(nonnull AlignContext *)context {
  [self.children enumerateObjectsUsingBlock:^(LynxShadowNode *_Nonnull obj, NSUInteger idx,
                                              BOOL *_Nonnull stop) {
    if ([obj isKindOfClass:[LynxNativeLayoutNode class]]) {
      LynxNativeLayoutNode *child = (LynxNativeLayoutNode *)obj;
      AlignParam *childParam = [[AlignParam alloc] init];
      [childParam SetAlignOffsetWithLeft:0.f Top:0.f];
      [child alignWithAlignParam:childParam AlignContext:context];
    }
    *stop = YES;
  }];
}

- (void)updateCustomRect:(CGRect)customLayout {
  if (CGRectEqualToRect(customLayout, self.customRect)) {
    return;
  }
  self.customRect = customLayout;
  @synchronized(self) {
    [self internalSetNeedsLayoutForce];
  }
}

+ (UIViewController *)findCustomViewControllerWith:(Class)customViewControllerClass
                                          rootView:(UIView *)rootView {
  UIViewController *customViewController = nil;

  UIResponder *responder = rootView;
  while (responder && ![responder isKindOfClass:customViewControllerClass]) {
    responder = responder.nextResponder;
  }
  customViewController = (UIViewController *)responder;

  // Can not find target UIViewController, try to find target UIView
  if (!customViewController) {
    UIView *view = rootView;
    while (view && ![view isKindOfClass:customViewControllerClass]) {
      view = view.superview;
    }
    // It's safe to cast from UIView to UIViewController. TODO: (xiamengfei.moonface) adapt to
    // UIView
    customViewController = (UIViewController *)view;
  }

  return customViewController;
}

- (CGRect)getMeasureBounds {
  if (CGRectIsNull(self.customRect)) {
    return [LynxUIOverlayShadowNode windowBounds];
  } else {
    return self.customRect;
  }
}

+ (CGRect)getBoundsWithMode:(LynxOverlayMode)mode
             viewController:(UIViewController *)viewController {
  if (mode == LynxOverlayModeWindow) {
    return [LynxUIOverlayShadowNode windowBounds];
  } else {
    return [LynxOverlayGlobalManager getTopViewControllerWithMode:mode
                                             customViewController:viewController]
        .bounds;
  }
}

+ (CGRect)windowBounds {
  UIWindow *window = [LynxUIKitAPIAdapter getKeyWindow];
  if (window) {
    return window.bounds;
  }
  return [UIScreen mainScreen].bounds;
}

@end

typedef NS_ENUM(NSInteger, LynxOverlayTouchEvent) {
  LynxOverlayTouchEventUnknown = -1,
  LynxOverlayTouchEventBegan = 0,
  LynxOverlayTouchEventChanged = 1,
  LynxOverlayTouchEventEnded = 2,
  LynxOverlayTouchEventCanceled = 3,
};

@interface LynxUIOverlay () <LynxOverlayViewDelegate, LynxViewVisibleHelper>
// props
@property(nonatomic, assign) BOOL visible;
@property(nonatomic, assign) BOOL eventPassThrough;
@property(nonatomic, assign) BOOL eventPassThroughHasBeenSet;
@property(nonatomic, assign) BOOL allowPanGesture;
@property(nonatomic, assign) LynxOverlayMode mode;
@property(nonatomic, assign) NSInteger level;
@property(nonatomic, strong) NSString *nestScrollViewID;

// nested scrollview in overlay, to resolve conflicts between scroll events and PanGesture
@property(nonatomic, strong) UIScrollView *nestScrollView;

// marks that overlay will be visible
@property(nonatomic, assign) BOOL willBecomeVisible;

@property(nonatomic, strong) Class customViewControllerClass;

@property(nonatomic, weak) UIViewController *customViewController;

@property(nonatomic, assign) BOOL notAdjustLeftMargin;

@property(nonatomic, assign) BOOL notAdjustTopMargin;

@property(nonatomic, assign) BOOL platformEventRootActive;

@property(nonatomic, assign) BOOL hasPlatformEventRootOffset;
@property(nonatomic, assign) CGPoint platformEventRootOffset;

- (void)updatePlatformEventRootActiveForFragmentLayer:(BOOL)active;

@property(nonatomic, assign) BOOL followModeEdge;

@end

@implementation LynxUIOverlay {
  enum LynxEventPropStatus _ignoreFocus;
  enum LynxPointerEventsValue _pointerEvents;
}

LYNX_LAZY_REGISTER_UI("overlay")

#pragma mark - LynxUI

- (instancetype)init {
  if (self = [super init]) {
    // init default value
    self.eventPassThrough = YES;
    self.level = 1;
    self.notAdjustLeftMargin = YES;
    self.notAdjustTopMargin = YES;
    _ignoreFocus = kLynxEventPropUndefined;
  }
  return self;
}

- (UIView *)createView {
  LynxOverlayContainer *container = [[LynxOverlayContainer alloc] init];
  container.uiDelegate = self;
  return container;
}

- (void)eventDidSet {
  [super eventDidSet];

  [(LynxOverlayContainer *)self.view
      enableTouchOverlayEvent:[self.eventSet objectForKey:@"overlaytouch"]];
}

/**
 *  Refresh UI and some status after props or layout updated
 */
- (void)onNodeReady {
  [super onNodeReady];

  if (!self.context.rootUI) {
    return;
  }
  CGRect modeRect = self.followModeEdge
                        ? [LynxUIOverlayShadowNode getBoundsWithMode:self.mode
                                                      viewController:self.customViewController]
                        : [LynxUIOverlayShadowNode windowBounds];

  // Overlay's frame must be equal to UIScreen
  self.view.frame = modeRect;

  // add Overlay's view to global container according to its level and mode
  UIView *container =
      [[LynxOverlayGlobalManager sharedInstance] showOverlayView:self.view
                                                         atLevel:self.level
                                                        withMode:self.mode
                                            customViewController:self.customViewController];

  // reset frame if container has its own offset
  CGPoint offset = [[self windowContainer] convertPoint:CGPointZero toView:container];

  CGRect rect = {(self.notAdjustLeftMargin ? 0 : offset.x),
                 (self.notAdjustTopMargin ? 0 : offset.y), modeRect.size};
  self.view.frame = rect;

  if (self.followModeEdge) {
    LynxUIOverlayShadowNode *node =
        (LynxUIOverlayShadowNode *)[self.context.nodeOwner nodeWithSign:self.sign];
    [node updateCustomRect:rect];
  }

  // make sure Overlay is always at the front
  if (self.willBecomeVisible) {
    self.willBecomeVisible = NO;
    [self.view.superview bringSubviewToFront:self.view];
  }

  if (self.view.hidden != !self.visible) {
    LynxCustomEvent *event =
        [[LynxDetailEvent alloc] initWithName:self.visible ? @"showoverlay" : @"dismissoverlay"
                                   targetSign:[self sign]
                                       detail:nil];
    [self.context.eventEmitter dispatchCustomEvent:event];
  }

  self.view.hidden = !self.visible;
  [self updatePlatformEventRootActiveForFragmentLayer:self.visible];

  // find nested scroll view
  if (self.nestScrollViewID) {
    UIView *rootView = [self.context.uiOwner uiWithIdSelector:self.nestScrollViewID].view;
    if ([self checkNestedScrollView:(UIScrollView *)rootView]) {
      self.nestScrollView = (UIScrollView *)rootView;
    } else {
      self.nestScrollView = [self findNestedScrollView:rootView.subviews];
    }
  } else {
    self.nestScrollView = nil;
  }
}

- (BOOL)blockNativeEvent:(UIGestureRecognizer *)gestureRecognizer {
  return !self.allowPanGesture;
}

/**
 *  The LynxUI of Overlay itself will not respond to any gesture, the logic is removed to
 * LynxOverlayContainer
 */
- (BOOL)shouldHitTest:(CGPoint)point withEvent:(nullable UIEvent *)event {
  return NO;
}

- (BOOL)IsViewVisible {
  return _visible;
}

- (BOOL)ignoreFocus {
  LYNX_ASSERT_ON_MAIN_THREAD;

  // If _ignoreFocus == Enable, return true. Otherwise, return false.
  if (_ignoreFocus == kLynxEventPropEnable) {
    return YES;
  }
  return NO;
}

- (enum LynxPointerEventsValue)pointerEvents {
  LYNX_ASSERT_ON_MAIN_THREAD;

  if (_pointerEvents == kLynxPointerEventsValueNone) {
    return _pointerEvents;
  }

  return kLynxPointerEventsValueAuto;
}

- (BOOL)eventThrough:(CGPoint)point {
  LYNX_ASSERT_ON_MAIN_THREAD

  // If _eventThrough == Enable, return true. Otherwise, return false.
  BOOL isEventThrough = NO;
  if (self.eventThrough == kLynxEventPropEnable) {
    isEventThrough = YES;
  }

  if (!self.eventThroughActiveRegions) {
    return isEventThrough;
  }

  CGSize size = self.view.bounds.size;
  __block BOOL isHitEventThroughActiveRegions = NO;
  [self.eventThroughActiveRegions
      enumerateObjectsUsingBlock:^(NSArray<LynxSizeValue *> *_Nonnull obj, NSUInteger idx,
                                   BOOL *_Nonnull stop) {
        if ([obj count] == 4) {
          CGFloat left = [obj[0] convertToDevicePtWithFullSize:size.width];
          CGFloat top = [obj[1] convertToDevicePtWithFullSize:size.height];
          CGFloat right = left + [obj[2] convertToDevicePtWithFullSize:size.width];
          CGFloat bottom = top + [obj[3] convertToDevicePtWithFullSize:size.height];
          isHitEventThroughActiveRegions =
              point.x >= left && point.x < right && point.y >= top && point.y < bottom;
          if (isHitEventThroughActiveRegions) {
            LLogInfo(@"hit the event through active regions!");
            *stop = YES;
          }
        }
      }];
  return isHitEventThroughActiveRegions ? isEventThrough : !isEventThrough;
}

#pragma mark - LYNX_PROPS

LYNX_PROP_SETTER("visible", setVisible, BOOL) {
  if (value && !self.visible) {
    self.willBecomeVisible = YES;
  }
  self.visible = value;
}

LYNX_PROP_SETTER("ios-follow-mode-edge", iOSFollowModeEdge, BOOL) { self.followModeEdge = value; }

// legacy API
LYNX_PROP_SETTER("allow-pan-gesture", setAllowPanGesture, BOOL) { self.allowPanGesture = value; }

LYNX_PROP_SETTER("ios-enable-swipe-back", enableSwipeBack, BOOL) { self.allowPanGesture = value; }

LYNX_PROP_SETTER("mode", setMode, NSString *) {
  if ([value isEqualToString:@"page"]) {
    self.mode = LynxOverlayModePage;
  } else if ([value isEqualToString:@"top"]) {
    self.mode = LynxOverlayModeTopController;
  } else if (NSClassFromString(value)) {
    self.mode = LynxOverlayModeCustom;
    self.customViewControllerClass = NSClassFromString(value);
  } else {
    self.mode = LynxOverlayModeWindow;
  }
}

LYNX_PROP_SETTER("level", setLevel, NSInteger) { self.level = value; }

// legacy API
LYNX_PROP_SETTER("events-pass-through", setEventPassthrough, BOOL) {
  self.eventPassThrough = value;
  self.eventPassThroughHasBeenSet = YES;
}

LYNX_PROP_SETTER("nest-scroll", setNestScroll, NSString *) { self.nestScrollViewID = value; }

LYNX_PROP_SETTER("ignore-focus", setIgnoreFocus, BOOL) {
  // If requestReset, the _ignoreFocus will be Undefined.
  if (requestReset) {
    _ignoreFocus = kLynxEventPropUndefined;
    return;
  }
  _ignoreFocus = value ? kLynxEventPropEnable : kLynxEventPropDisable;
}

LYNX_PROP_SETTER("pointer-events", setPointerEvents, NSInteger) {
  // If requestReset, the _pointerEvents will be Undefined.
  if (requestReset) {
    _pointerEvents = kLynxPointerEventsValueUnset;
    return;
  }
  if (value >= kLynxPointerEventsValueAuto && value < kLynxPointerEventsValueUnset) {
    _pointerEvents = (enum LynxPointerEventsValue)value;
  }
}

#pragma mark - LynxUIOverlayViewDelegate

- (BOOL)forbidPanGesture {
  return !self.allowPanGesture;
}

- (BOOL)eventPassed:(CGPoint)point {
  return self.eventPassThroughHasBeenSet ? self.eventPassThrough : [self eventThrough:point];
}

- (NSInteger)getSign {
  return self.sign;
}

- (void)requestClose:(NSDictionary *)info {
  LynxCustomEvent *event = [[LynxDetailEvent alloc] initWithName:@"onRequestClose"
                                                      targetSign:[self sign]
                                                          detail:nil];
  [self.context.eventEmitter dispatchCustomEvent:event];
  [self.context.observer notifyLayout:NULL];
}

- (LynxUI *)overlayRootUI {
  return self;
}

/**
 *  Expose pan gesture to lepus
 */
- (void)overlayTouched:(NSString *)name
                 point:(CGPoint)point
                 state:(UIGestureRecognizerState)state
              velocity:(CGPoint)velocity {
  NSInteger eventState = LynxOverlayTouchEventUnknown;
  switch (state) {
    case UIGestureRecognizerStateBegan:
      eventState = LynxOverlayTouchEventBegan;
      break;
    case UIGestureRecognizerStateChanged:
      eventState = LynxOverlayTouchEventChanged;
      break;
    case UIGestureRecognizerStateEnded:
      eventState = LynxOverlayTouchEventEnded;
    case UIGestureRecognizerStateCancelled:
      eventState = LynxOverlayTouchEventEnded;
      break;
    default:
      break;
  }
  LynxCustomEvent *event = [[LynxDetailEvent alloc] initWithName:name
                                                      targetSign:[self sign]
                                                          detail:@{
                                                            @"x" : @(point.x),
                                                            @"y" : @(point.y),
                                                            @"vx" : @(velocity.x),
                                                            @"vy" : @(velocity.y),
                                                            @"state" : @(eventState)
                                                          }];
  [self.context.eventEmitter dispatchCustomEvent:event];
  [self.context.observer notifyLayout:NULL];
}

#pragma mark - Internal

- (UIScrollView *)findNestedScrollView:(NSArray<__kindof UIView *> *)subviews {
  NSArray<UIView *> *reverseSubview = [[subviews reverseObjectEnumerator] allObjects];

  // use BFS, to make sure that the top-most scrollview is the target one
  for (UIView *child in reverseSubview) {
    if ([self checkNestedScrollView:(UIScrollView *)child]) {
      return (UIScrollView *)child;
    }
  }

  for (UIView *child in reverseSubview) {
    UIScrollView *ret = [self findNestedScrollView:child.subviews];
    if (ret) {
      return ret;
    }
  }

  return nil;
}

- (BOOL)checkNestedScrollView:(UIScrollView *)scrollview {
  if (![scrollview isKindOfClass:UIScrollView.class]) {
    return NO;
  }
  // a vertical scrollview is needed
  if (scrollview.alwaysBounceHorizontal ||
      scrollview.contentSize.width > scrollview.bounds.size.width) {
    return NO;
  }
  if (!scrollview.alwaysBounceVertical &&
      scrollview.contentSize.height <= scrollview.bounds.size.height) {
    return NO;
  }
  return YES;
}

- (UIView *)windowContainer {
  return [LynxUIKitAPIAdapter getForegroundKeyWindow];
}

- (UIViewController *)customViewController {
  if (![_customViewController isKindOfClass:self.customViewControllerClass]) {
    _customViewController =
        [LynxUIOverlayShadowNode findCustomViewControllerWith:self.customViewControllerClass
                                                     rootView:self.parent.view];
  }
  return _customViewController;
}

- (void)dealloc {
  [self updatePlatformEventRootActiveForFragmentLayer:NO];

  // avoid objc store weak
  if (_customViewController) {
    [[LynxOverlayGlobalManager sharedInstance] destroyOverlayView:self.view
                                                          atLevel:self.level
                                                         withMode:self.mode
                                             customViewController:_customViewController];
  } else {
    [[LynxOverlayGlobalManager sharedInstance] destoryUnattachedOverlay:self.view];
  }

  // Overlay's view is attached to the global container, so, remove it manually when dealloc
  [self.view removeFromSuperview];
}

LYNX_PROPS_GROUP_DECLARE(LYNX_PROP_DECLARE("ios-not-adjust-top-margin", setIosNotAdjustTopMargin,
                                           BOOL),
                         LYNX_PROP_DECLARE("ios-not-adjust-left-margin", setIosNotAdjustLeftMargin,
                                           BOOL))

/**
 * @name: ios-not-adjust-left-margin
 * @description: On iOS, if we are trying to open a new UIViewController with animation, the
 *overlay's container may have a left offset in the UIWindow's coordinate. This property is designed
 *to disable the left margin adjustment.
 * @category: different
 * @standardAction: offline
 * @supportVersion: 2.8
 * @resolveVersion: 3.0
 **/
LYNX_PROP_DEFINE("ios-not-adjust-left-margin", setIosNotAdjustLeftMargin, BOOL) {
  self.notAdjustLeftMargin = value;
}

/**
 * @name: ios-not-adjust-top-margin
 * @description: On iOS, if we are trying to open a new UIViewController with animation, the
 *overlay's container may have a top offset in the UIWindow's coordinate. This property is designed
 *to disable the top margin adjustment.
 * @category: different
 * @standardAction: offline
 * @supportVersion: 2.10
 * @resolveVersion: 3.0
 **/
LYNX_PROP_DEFINE("ios-not-adjust-top-margin", setIosNotAdjustTopMargin, BOOL) {
  self.notAdjustTopMargin = value;
}

- (BOOL)isOverlay {
  return YES;
}

- (void)updatePlatformEventRootActiveForFragmentLayer:(BOOL)active {
  LynxUIContext *uiContext = self.context.uiOwner.uiContext;
  if (!uiContext.lynxContext.isFragmentLayerRenderOn) {
    return;
  }
  UIView *rootView = uiContext.rootView;
  if (![rootView isKindOfClass:LynxView.class]) {
    return;
  }
  if (active) {
    CGPoint offset = [self.view convertPoint:CGPointZero toView:rootView];
    if (!self.hasPlatformEventRootOffset ||
        !CGPointEqualToPoint(self.platformEventRootOffset, offset)) {
      self.platformEventRootOffset = offset;
      self.hasPlatformEventRootOffset = YES;
      [((LynxView *)rootView).templateRender SetPlatformEventRootOffset:self.sign
                                                                offsetX:offset.x
                                                                offsetY:offset.y];
    }
  } else {
    self.hasPlatformEventRootOffset = NO;
  }
  if (self.platformEventRootActive == active) {
    return;
  }
  self.platformEventRootActive = active;
  [((LynxView *)rootView).templateRender SetPlatformEventRootActive:self.sign active:active];
}

@end
