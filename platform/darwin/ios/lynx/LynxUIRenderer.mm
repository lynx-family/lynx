// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUIRenderer.h"

#import <Lynx/DevToolOverlayDelegate.h>
#import <Lynx/LUIConfigAdapter.h>
#import <Lynx/ListNodeInfoFetcher.h>
#import <Lynx/LynxEventHandler.h>
#import <Lynx/LynxFontFaceManager.h>
#import <Lynx/LynxGenericResourceFetcher.h>
#import <Lynx/LynxKeyboardEventDispatcher.h>
#import <Lynx/LynxService.h>
#import <Lynx/LynxServiceTextProtocol.h>
#import <Lynx/LynxUIKitAPIAdapter.h>
#import <Lynx/LynxWeakProxy.h>
#import <Lynx/UIView+Lynx.h>
#import "LynxBaseConfigurator+Internal.h"
#import "LynxContext+Internal.h"
#import "LynxEnv+Internal.h"
#import "LynxEventHandler+Internal.h"
#import "LynxTemplateRender+Internal.h"
#import "LynxTemplateRender+Protected.h"
#import "LynxTouchHandler+Internal.h"
#import "LynxUIContext+Internal.h"
#import "LynxUIExposure+Internal.h"
#import "LynxUIIntersectionObserver+Internal.h"

#include "core/renderer/ui_wrapper/painting/ios/ui_delegate_darwin.h"

typedef NS_ENUM(NSUInteger, BoxModelOffset) {
  PAD_LEFT = 0,
  PAD_TOP,
  PAD_RIGHT,
  PAD_BOTTOM,
  BORDER_LEFT,
  BORDER_TOP,
  BORDER_RIGHT,
  BORDER_BOTTOM,
  MARGIN_LEFT,
  MARGIN_TOP,
  MARGIN_RIGHT,
  MARGIN_BOTTOM,
  LAYOUT_LEFT,
  LAYOUT_TOP,
  LAYOUT_RIGHT,
  LAYOUT_BOTTOM,
};

static id<LynxServiceTextProtocol> getTextService() {
  static id<LynxServiceTextProtocol> sService = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    sService = LynxService(LynxServiceTextProtocol);
  });
  return sService;
}

@implementation LynxUIRenderer {
  __weak UIView<LUIBodyView> *_containerView;
  __weak LynxContext *_lynxContext;
  LynxProviderRegistry *_providerRegistry;
  std::unique_ptr<lynx::tasm::UIDelegate> ui_delegate_;
  LynxUIOwner *_uiOwner;

  void *_textra;

  LynxEventHandler *_eventHandler;
  LynxEventEmitter *_eventEmitter;
  LynxKeyboardEventDispatcher *_keyboardEventDispatcher;
  LynxUIIntersectionObserverManager *_intersectionObserverManager;

  BOOL _enableGenericResourceLoader;
}

- (instancetype)initWithLynxContext:(LynxContext *)context
                      containerView:(UIView<LUIBodyView> *)containerView
                            builder:(LynxViewBuilder *)builder
                   providerRegistry:(LynxProviderRegistry *)providerRegistry {
  self = [super init];
  if (self) {
    _lynxContext = context;
    _containerView = containerView;
    _providerRegistry = providerRegistry;
    _textra = 0;
    _enableGenericResourceLoader =
        [self checkEnableGenericResourceFetcher:builder.enableGenericResourceFetcher];
    [self setupUIOwnerWithBuilder:builder];
    [self setupResourceProviderWithBuilder:builder];
  }
  return self;
}

- (void)setupUIOwnerWithBuilder:(LynxViewBuilder *)builder {
  LynxScreenMetrics *screenMetrics =
      [[LynxScreenMetrics alloc] initWithScreenSize:builder.screenSize
                                              scale:[UIScreen mainScreen].scale];
  _uiOwner = [[LynxUIOwner alloc] initWithContainerView:_containerView
                                      componentRegistry:builder.config.componentRegistry
                                          screenMetrics:screenMetrics
                                           errorHandler:_containerView
                                               uiConfig:nil
                                           embeddedMode:[builder getEmbeddedMode]];
  _uiOwner.uiContext.lynxContext = _lynxContext;
  _uiOwner.uiContext.contextDict = [builder.config.contextDict copy];
  _uiOwner.uiContext.lynxModuleExtraData = builder.lynxModuleExtraData;
  _uiOwner.uiContext.imageConfig = builder.imageConfig;
}

- (void)setupResourceProviderWithBuilder:(LynxViewBuilder *)builder {
  _uiOwner.fontFaceContext.resourceProvider =
      [_providerRegistry getResourceProviderByKey:LYNX_PROVIDER_TYPE_FONT];
  _uiOwner.fontFaceContext.builderRegistedAliasFontMap = [builder getBuilderRegisteredAliasFontMap];

  if (_enableGenericResourceLoader) {
    _uiOwner.uiContext.genericResourceFetcher = [builder genericResourceFetcher];
    _uiOwner.uiContext.mediaResourceFetcher = [builder mediaResourceFetcher];
    _uiOwner.uiContext.templateResourceFetcher = [builder templateResourceFetcher];
    _uiOwner.fontFaceContext.genericResourceServiceFetcher = [builder genericResourceFetcher];
    _uiOwner.uiContext.enableFetchUIImage = [builder enableFetchUIImage];
    // will be deleted later
    if (!_uiOwner.uiContext.enableFetchUIImage &&
        [builder.lynxViewConfig objectForKey:KEY_LYNX_ENABLE_FETCH_UIIMAGE]) {
      _uiOwner.uiContext.enableFetchUIImage =
          [[builder.lynxViewConfig objectForKey:KEY_LYNX_ENABLE_FETCH_UIIMAGE] boolValue];
    }
  }
}

- (BOOL)checkEnableGenericResourceFetcher:(LynxBooleanOption)enable {
  if (enable == LynxBooleanOptionUnset) {
    return [[LynxEnv sharedInstance] enableGenericResourceFetcher];
  }
  return enable == LynxBooleanOptionTrue;
}

- (BOOL)useInvokeUIMethodFunction {
  return NO;
}

- (void)attachContainerView:(nonnull UIView<LUIBodyView> *)containerView {
  if (_uiOwner != nil) {
    [_uiOwner attachContainerView:containerView];
  }

  if (_eventHandler) {
    [_eventHandler attachContainerView:containerView];
  }
}

- (void)setupUIDelegate:(LynxShadowNodeOwner *)owner {
  id<LynxServiceTextProtocol> textService =
      (_lynxContext.isLayoutInElementModeOn && _lynxContext.isTextServiceModeOn) ? getTextService()
                                                                                 : nil;
  if (textService != nil) {
    _textra = [textService createTextLayoutAPIFromContext:_uiOwner];
  }
  ui_delegate_ = std::make_unique<lynx::tasm::UIDelegateDarwin>(
      _uiOwner, _lynxContext.isFragmentLayerRenderOn, _textra,
      [[LynxEnv sharedInstance] enableCreateUIAsync], owner);
}

- (void *)uiDelegate {
  return ui_delegate_.get();
}

- (void)setupEventHandler:(LynxEngineProxy *)engineProxy
                 shellPtr:(int64_t)shellPtr
                    block:(onLynxEvent)block {
  _uiOwner.uiContext.shellPtr = shellPtr;

  auto shell = reinterpret_cast<lynx::shell::LynxShell *>(shellPtr);
  int64_t list_engine_proxy_ptr = reinterpret_cast<int64_t>(shell->GetListEngineProxy().get());
  _uiOwner.uiContext.fetcher = [[ListNodeInfoFetcher alloc] initWithShell:shellPtr
                                                       andListEngineProxy:list_engine_proxy_ptr];

  _eventEmitter = [[LynxEventEmitter alloc] initWithLynxEngineProxy:engineProxy];
  __weak typeof(self) weakSelf = self;
  [_eventEmitter setEventReporterBlock:block];
  dispatch_block_t intersectionObserver = ^() {
    __strong __typeof(weakSelf) strongSelf = weakSelf;
    if (!strongSelf) {
      return;
    }
    if (strongSelf->_lynxContext.intersectionManager) {
      [strongSelf->_lynxContext.intersectionManager notifyObservers];
    }
  };
  [_eventEmitter setIntersectionObserverBlock:intersectionObserver];
  _uiOwner.uiContext.eventEmitter = _eventEmitter;
  if (_eventHandler == nil) {
    _eventHandler = [[LynxEventHandler alloc] initWithRootView:_containerView];
  }
  _uiOwner.uiContext.eventHandler = _eventHandler;

  [_eventHandler updateUiOwner:_uiOwner eventEmitter:_eventEmitter];
  _intersectionObserverManager =
      [[LynxUIIntersectionObserverManager alloc] initWithLynxContext:_lynxContext];
  _intersectionObserverManager.uiOwner = _uiOwner;
  [_eventEmitter addObserver:_intersectionObserverManager];

  _lynxContext.intersectionManager = _intersectionObserverManager;
  _lynxContext.uiOwner = _uiOwner;

  _keyboardEventDispatcher = [[LynxKeyboardEventDispatcher alloc] initWithContext:_lynxContext];
  _lynxContext.keyboardEventDispatcher = _keyboardEventDispatcher;
}

- (void)onPageConfigUpdate:(const std::shared_ptr<lynx::tasm::PageConfig> &)pageConfig {
  // Since page config is a C++ class and Event Handler is a pure OC class, the set methods must be
  // called here.
  [_eventHandler setEnableSimultaneousTap:pageConfig->GetEnableSimultaneousTap()];
  [_eventHandler setEnableViewReceiveTouch:pageConfig->GetEnableViewReceiveTouch()];
  [_eventHandler setDisableLongpressAfterScroll:pageConfig->GetDisableLongpressAfterScroll()];
  [_eventHandler setTapSlop:[NSString stringWithUTF8String:pageConfig->GetTapSlop().c_str()]];
  [_eventHandler setLongPressDuration:pageConfig->GetLongPressDuration()];
  [_eventHandler setEnablePlatformGesture:pageConfig->GetEnablePlatformGesture()];
  [_eventHandler.touchRecognizer setEnableTouchRefactor:pageConfig->GetEnableTouchRefactor()];
  [_eventHandler.touchRecognizer
      setEnableEndGestureAtLastFingerUp:pageConfig->GetEnableEndGestureAtLastFingerUp()];
  _eventHandler.touchRecognizer.enableNewGesture = pageConfig->GetEnableNewGesture();
  [_uiOwner initNewGestureInUIThread:pageConfig->GetEnableNewGesture()];
  // If enable fiber arch, enable touch pseudo as default.
  [_eventHandler.touchRecognizer setEnableTouchPseudo:pageConfig->GetEnableFiberArch()];
  // Enable support multi-finger events.
  [_eventHandler.touchRecognizer setEnableMultiTouch:pageConfig->GetEnableMultiTouch()];

  // Set config to IntersectionObserverManager
  [_intersectionObserverManager
      setEnableNewIntersectionObserver:pageConfig->GetEnableNewIntersectionObserver()];

  // Set config to LynxUIExposure
  [_uiOwner.uiContext.uiExposure
      setObserverFrameRateForExposure:pageConfig->GetObserverFrameRate()];
  [_uiOwner.uiContext.uiExposure
      setEnableCheckExposureOptimize:pageConfig->GetEnableCheckExposureOptimize()];

  // Set config to LynxUIContext;
  LynxUIContext *uiContext = _uiOwner.uiContext;
  LUIConfigAdapter *configAdapter = [[LUIConfigAdapter alloc] initWithConfig:pageConfig.get()];
  [uiContext setUIConfig:configAdapter];
}

- (void)setFluencyTracerEnabled:(LynxBooleanOption)enabled {
  [_uiOwner.uiContext.fluencyInnerListener setEnabledBySampling:enabled];
}

- (BOOL)needPaintingContextProxy {
  return YES;
}

- (void)onSetFrame:(CGRect)frame {
  return;
}

- (nullable LynxUIIntersectionObserverManager *)getLynxUIIntersectionObserverManager {
  return _intersectionObserverManager;
}

- (BOOL)needHandleHitTest {
  return NO;
}

- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event {
  return nil;
}

- (id<LynxEventTarget>)hitTestInEventHandler:(CGPoint)point withEvent:(UIEvent *)event {
  return [_eventHandler hitTest:point withEvent:event];
}

- (void)handleFocus:(id<LynxEventTarget>)target
             onView:(UIView *)view
      withContainer:(UIView *)container
           andPoint:(CGPoint)point
           andEvent:(UIEvent *)event {
  [_eventHandler handleFocus:target
                      onView:view
               withContainer:container
                    andPoint:point
                    andEvent:event];
}

- (UIView *)eventHandlerRootView {
  return nil;
}

- (void)setupWithContainerView:(UIView<LUIBodyView> *)containerView
                       builder:(LynxViewBuilder *)builder
                    screenSize:(CGSize)screenSize {
}

- (LynxUIOwner *)uiOwner {
  return _uiOwner;
}

- (LynxRootUI *)rootUI {
  return _uiOwner.rootUI;
}

- (id<LynxTemplateResourceFetcher>)templateResourceFetcher {
  if (_enableGenericResourceLoader) {
    return _uiOwner.uiContext.templateResourceFetcher;
  }
  return nil;
}

- (id<LynxGenericResourceFetcher>)genericResourceFetcher {
  if (_enableGenericResourceLoader) {
    return _uiOwner.uiContext.genericResourceFetcher;
  }
  return nil;
}

- (id<LynxMediaResourceFetcher>)mediaResourceFetcher {
  if (_enableGenericResourceLoader) {
    return _uiOwner.uiContext.mediaResourceFetcher;
  }
  return nil;
}

- (void)reset {
  [_uiOwner reset];

  if (getTextService() != nil && _textra) {
    [getTextService() destroyTextLayoutAPI:_textra];
  }
}

- (LynxGestureArenaManager *)getGestureArenaManager {
  return _uiOwner.gestureArenaManager;
}

- (void)onEnterForeground {
  [_uiOwner onEnterForeground];
}

- (void)onEnterBackground {
  [_uiOwner onEnterBackground];
}

- (void)willMoveToWindow:(UIWindow *)newWindow {
  [_uiOwner willContainerViewMoveToWindow:newWindow];
}

- (void)didMoveToWindow:(BOOL)windowIsNil {
  [_uiOwner didMoveToWindow:windowIsNil];
}

#pragma mark - View

- (void)setCustomizedLayoutInUIContext:(id<LynxListLayoutProtocol> _Nullable)customizedListLayout {
  _uiOwner.uiContext.customizedListLayout = customizedListLayout;
}

- (void)setScrollListener:(id<LynxScrollListener>)scrollListener {
  _uiOwner.uiContext.scrollListener = scrollListener;
}

- (void)setImageFetcherInUIOwner:(id<LynxImageFetcher>)imageFetcher {
  _uiOwner.uiContext.imageFetcher = imageFetcher;
}

- (void)setResourceFetcherInUIOwner:(id<LynxResourceFetcher>)resourceFetcher {
  _uiOwner.uiContext.resourceFetcher = resourceFetcher;
  _uiOwner.fontFaceContext.resourceFetcher = resourceFetcher;
}

- (LynxScreenMetrics *)getScreenMetrics {
  if (_uiOwner != nil && _uiOwner.uiContext != nil) {
    return _uiOwner.uiContext.screenMetrics;
  }
  return nil;
}

- (void)updateScreenWidth:(CGFloat)width height:(CGFloat)height {
  if (_uiOwner != nil && _uiOwner.uiContext != nil) {
    [_uiOwner.uiContext updateScreenSize:CGSizeMake(width, height)];
  }
}

- (void)pauseRootLayoutAnimation {
  [_uiOwner pauseRootLayoutAnimation];
}

- (void)resumeRootLayoutAnimation {
  [_uiOwner resumeRootLayoutAnimation];
}

- (void)restartAnimation {
  [_uiOwner restartAnimation];
}

- (void)resetAnimation {
  [_uiOwner resetAnimation];
}

- (void)invokeUIMethodForSelectorQuery:(NSString *)method
                                params:(NSDictionary *)params
                              callback:(LynxUIMethodCallbackBlock)callback
                                toNode:(int)sign {
  [_uiOwner invokeUIMethodForSelectorQuery:method params:params callback:callback toNode:sign];
}

std::vector<float> NSArrayToVector(NSArray<NSNumber *> *array) {
  std::vector<float> result;
  result.reserve(array.count);
  for (NSNumber *num in array) {
    result.push_back(num.floatValue);
  }
  return result;
}

NSArray<NSNumber *> *VectorToNSArray(const std::vector<float> &vec) {
  NSMutableArray<NSNumber *> *result = [NSMutableArray arrayWithCapacity:vec.size()];
  for (float value : vec) {
    [result addObject:@(value)];
  }
  return [result copy];
}

- (BOOL)isFullScreenShotSupported {
  return YES;
}

- (NSArray<NSNumber *> *)getTransformValue:(NSInteger)sign
                 withPadBorderMarginLayout:(NSArray<NSNumber *> *)arrayLayout {
  std::vector<float> padBorderMarginLayout = NSArrayToVector(arrayLayout);
  std::vector<float> res;
  LynxUI *ui = [_uiOwner findUIBySign:sign];
  if (ui != nil) {
    for (int i = 0; i < 4; i++) {
      TransOffset arr;
      if (i == 0) {
        arr = [ui getTransformValueWithLeft:padBorderMarginLayout[PAD_LEFT] +
                                            padBorderMarginLayout[BORDER_LEFT] +
                                            padBorderMarginLayout[LAYOUT_LEFT]
                                      right:-padBorderMarginLayout[PAD_RIGHT] -
                                            padBorderMarginLayout[BORDER_RIGHT] -
                                            padBorderMarginLayout[LAYOUT_RIGHT]
                                        top:padBorderMarginLayout[PAD_TOP] +
                                            padBorderMarginLayout[BORDER_TOP] +
                                            padBorderMarginLayout[LAYOUT_TOP]
                                     bottom:-padBorderMarginLayout[PAD_BOTTOM] -
                                            padBorderMarginLayout[BORDER_BOTTOM] -
                                            padBorderMarginLayout[LAYOUT_BOTTOM]];
      } else if (i == 1) {
        arr = [ui getTransformValueWithLeft:padBorderMarginLayout[BORDER_LEFT] +
                                            padBorderMarginLayout[LAYOUT_LEFT]
                                      right:-padBorderMarginLayout[BORDER_RIGHT] -
                                            padBorderMarginLayout[LAYOUT_RIGHT]
                                        top:padBorderMarginLayout[BORDER_TOP] +
                                            padBorderMarginLayout[LAYOUT_TOP]
                                     bottom:-padBorderMarginLayout[BORDER_BOTTOM] -
                                            padBorderMarginLayout[LAYOUT_BOTTOM]];
      } else if (i == 2) {
        arr = [ui getTransformValueWithLeft:padBorderMarginLayout[LAYOUT_LEFT]
                                      right:-padBorderMarginLayout[LAYOUT_RIGHT]
                                        top:padBorderMarginLayout[LAYOUT_TOP]
                                     bottom:-padBorderMarginLayout[LAYOUT_BOTTOM]];
      } else {
        arr = [ui getTransformValueWithLeft:-padBorderMarginLayout[MARGIN_LEFT] +
                                            padBorderMarginLayout[LAYOUT_LEFT]
                                      right:padBorderMarginLayout[MARGIN_RIGHT] -
                                            padBorderMarginLayout[LAYOUT_RIGHT]
                                        top:-padBorderMarginLayout[MARGIN_TOP] +
                                            padBorderMarginLayout[LAYOUT_TOP]
                                     bottom:padBorderMarginLayout[MARGIN_BOTTOM] -
                                            padBorderMarginLayout[LAYOUT_BOTTOM]];
      }
      res.push_back(arr.left_top.x);
      res.push_back(arr.left_top.y);
      res.push_back(arr.right_top.x);
      res.push_back(arr.right_top.y);
      res.push_back(arr.right_bottom.x);
      res.push_back(arr.right_bottom.y);
      res.push_back(arr.left_bottom.x);
      res.push_back(arr.left_bottom.y);
    }
  }

  NSArray<NSNumber *> *result = VectorToNSArray(res);
  return result;
}

- (CGPoint)convertPointFromScreen:(CGPoint)point ToView:(UIView *)view {
  return [[LynxUIKitAPIAdapter getKeyWindow] convertPoint:point toView:view];
}

/*
 *find the minimum ui node which the point falls in
 *
 *Parameter:
 * x,y is coordinate to the screen of the point
 * uiSign is the id of the starting search node, lynxView or overlay view
 * thus,before calling view's hitTest function, We need to first convert the coordinates relative to
 *the screen into coordinates relative to the view
 *
 * Return Value:
 * the id of the found node , return 0 if not found
 */
- (int)findNodeIdForLocationWithX:(float)x withY:(float)y fromUI:(int)uiSign mode:(NSString *)mode {
  if (_uiOwner) {
    UIView *view;
    if (uiSign == 0) {
      // find node from LynxView
      view = _uiOwner.rootUI.rootView;
    } else {
      // find node from overlay view
      LynxUI *ui = [_uiOwner findUIBySign:uiSign];
      if (ui != NULL) {
        view = ui.view;
      } else {
        return 0;
      }
    }
    UIEvent *event = nil;
    CGPoint point_to_view;
    if ([mode isEqualToString:@"lynxview"]) {
      point_to_view = CGPointMake(x, y);
    } else {  // fullscreen
      point_to_view = [self convertPointFromScreen:CGPointMake(x, y) ToView:view];
    }
    UIView *target = [view hitTest:point_to_view withEvent:event];
    if (target && target.lynxSign) {
      return [target.lynxSign intValue];
    }
  }
  return 0;
}

- (NSArray<NSNumber *> *)getVisibleOverlayView {
  NSArray<NSNumber *> *array = [DevToolOverlayDelegate.sharedInstance getAllVisibleOverlaySign];
  return array;
}

- (int)findNodeIdForLocationWithX:(float)x withY:(float)y mode:(NSString *)mode {
  int node_id = 0;
  if ([mode isEqualToString:@"fullscreen"]) {
    NSArray<NSNumber *> *overlays = [self getVisibleOverlayView];
    NSEnumerator *enumerator = [overlays reverseObjectEnumerator];
    NSNumber *num;
    while ((num = [enumerator nextObject]) != nil) {
      node_id = [self findNodeIdForLocationWithX:x withY:y fromUI:[num intValue] mode:mode];
      // overlay node's size is window size and it has one and only
      // one child if id == overlays[i], it means point is not in child so
      // not in overlay Under this circumstances,we need reset id to 0
      if (node_id != 0 && node_id != [num intValue]) {
        return node_id;
      } else {
        node_id = 0;
      }
    }
    node_id =
        node_id != 0 ? node_id : [self findNodeIdForLocationWithX:x withY:y fromUI:0 mode:mode];
  } else {  // lynxview
    node_id = [self findNodeIdForLocationWithX:x withY:y fromUI:0 mode:mode];
  }
  return node_id;
}

#pragma mark - Find Node

- (LynxUI *)findUIBySign:(NSInteger)sign {
  return [_uiOwner findUIBySign:sign];
}

- (nullable UIView *)findViewWithName:(nonnull NSString *)name {
  LynxWeakProxy *weakLynxUI = [_uiOwner weakLynxUIWithName:name];
  return ((LynxUI *)weakLynxUI.target).view;
}

- (nullable LynxUI *)uiWithName:(nonnull NSString *)name {
  return [_uiOwner uiWithName:name];
}

- (nullable LynxUI *)uiWithIdSelector:(nonnull NSString *)idSelector {
  return [_uiOwner uiWithIdSelector:idSelector];
}

- (nullable UIView *)viewWithIdSelector:(nonnull NSString *)idSelector {
  return [_uiOwner uiWithIdSelector:idSelector].view;
}

- (nullable UIView *)viewWithName:(nonnull NSString *)name {
  return [_uiOwner uiWithName:name].view;
}

@end
