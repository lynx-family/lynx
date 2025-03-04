// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUIRenderer.h"

#import "LynxContext+Internal.h"
#import "LynxEnv+Internal.h"
#import "LynxEventHandler+Internal.h"
#import "LynxEventHandler.h"
#import "LynxFontFaceManager.h"
#import "LynxGenericResourceFetcher.h"
#import "LynxKeyboardEventDispatcher.h"
#import "LynxTemplateRender+Internal.h"
#import "LynxTemplateRender+Protected.h"
#import "LynxTouchHandler+Internal.h"
#import "LynxUIContext+Internal.h"
#import "LynxUIExposure+Internal.h"
#import "LynxUIIntersectionObserver+Internal.h"
#import "LynxViewBuilder+Internal.h"
#import "LynxWeakProxy.h"

#include "core/renderer/ui_wrapper/painting/ios/ui_delegate_darwin.h"

@implementation LynxUIRenderer {
  std::unique_ptr<lynx::tasm::UIDelegate> ui_delegate_;
  LynxUIOwner *_uiOwner;

  LynxEventHandler *_eventHandler;
  LynxEventEmitter *_eventEmitter;
  LynxKeyboardEventDispatcher *_keyboardEventDispatcher;
  LynxUIIntersectionObserverManager *_intersectionObserverManager;

  BOOL _enableGenericResourceLoader;
}

- (instancetype)init {
  self = [super init];
  return self;
}

- (BOOL)useInvokeUIMethodFunction {
  return NO;
}

- (void)attachLynxView:(nonnull LynxView *)lynxView {
  if (_uiOwner != nil) {
    [_uiOwner attachLynxView:(LynxView *)lynxView];
  }

  if (_eventHandler) {
    [_eventHandler attachLynxView:lynxView];
  }
}

- (void)onSetupUIDelegate:(lynx::tasm::UIDelegate *)uiDelegate {
  ui_delegate_.reset(uiDelegate);
}

- (void)onSetupUIDelegate:(nonnull lynx::shell::LynxShell *)shell
        withModuleManager:(nonnull lynx::piper::LynxModuleManager *)moduleManager
              withJSProxy:(std::shared_ptr<lynx::shell::LynxRuntimeProxy>)jsProxy {
}

- (lynx::tasm::UIDelegate *)uiDelegate {
  return ui_delegate_.get();
}

- (void)setupEventHandler:(LynxTemplateRender *)templateRenderer
              engineProxy:(LynxEngineProxy *)engineProxy
                 lynxView:(LynxView *)lynxView
                  context:(LynxContext *)context
                 shellPtr:(int64_t)shellPtr {
  _uiOwner.uiContext.shellPtr = shellPtr;
  _eventEmitter = [[LynxEventEmitter alloc] initWithLynxEngineProxy:engineProxy];
  __weak typeof(templateRenderer) weakRender = templateRenderer;
  onLynxEvent eventReporter = ^BOOL(LynxEvent *event) {
    __strong typeof(weakRender) strongRender = weakRender;
    return [strongRender onLynxEvent:event];
  };
  [_eventEmitter setEventReporterBlock:eventReporter];
  dispatch_block_t intersectionObserver = ^() {
    __strong typeof(weakRender) strongRender = weakRender;
    [strongRender notifyIntersectionObservers];
  };
  [_eventEmitter setIntersectionObserverBlock:intersectionObserver];
  _uiOwner.uiContext.eventEmitter = _eventEmitter;
  if (_eventHandler == nil) {
    _eventHandler = [[LynxEventHandler alloc] initWithRootView:lynxView];
  }
  _uiOwner.uiContext.eventHandler = _eventHandler;

  [_eventHandler updateUiOwner:_uiOwner eventEmitter:_eventEmitter];
  _intersectionObserverManager =
      [[LynxUIIntersectionObserverManager alloc] initWithLynxContext:context];
  _intersectionObserverManager.uiOwner = _uiOwner;
  [_eventEmitter addObserver:_intersectionObserverManager];

  context.intersectionManager = _intersectionObserverManager;
  context.uiOwner = _uiOwner;

  _keyboardEventDispatcher = [[LynxKeyboardEventDispatcher alloc] initWithContext:context];
}

- (void)setPageConfig:(const std::shared_ptr<lynx::tasm::PageConfig> &)pageConfig
              context:(LynxContext *)context {
  // Since page config is a C++ class and Event Handler is a pure OC class, the set methods must be
  // called here.
  [_eventHandler setEnableSimultaneousTap:pageConfig->GetEnableSimultaneousTap()];
  [_eventHandler setEnableViewReceiveTouch:pageConfig->GetEnableViewReceiveTouch()];
  [_eventHandler setDisableLongpressAfterScroll:pageConfig->GetDisableLongpressAfterScroll()];
  [_eventHandler setTapSlop:[NSString stringWithUTF8String:pageConfig->GetTapSlop().c_str()]];
  [_eventHandler setLongPressDuration:pageConfig->GetLongPressDuration()];
  [_eventHandler.touchRecognizer setEnableTouchRefactor:pageConfig->GetEnableTouchRefactor()];
  [_eventHandler.touchRecognizer
      setEnableEndGestureAtLastFingerUp:pageConfig->GetEnableEndGestureAtLastFingerUp()];
  _eventHandler.touchRecognizer.enableNewGesture = pageConfig->GetEnableNewGesture();
  [context.uiOwner initNewGestureInUIThread:pageConfig->GetEnableNewGesture()];

  // If enable fiber arch, enable touch pseudo as default.
  [_eventHandler.touchRecognizer setEnableTouchPseudo:pageConfig->GetEnableFiberArch()];
  // Enable support multi-finger events.
  [_eventHandler.touchRecognizer setEnableMultiTouch:pageConfig->GetEnableMultiTouch()];

  // Set config to IntersectionObserverManager
  [_intersectionObserverManager
      setEnableNewIntersectionObserver:pageConfig->GetEnableNewIntersectionObserver()];

  // Set config to LynxUIExposure
  [context.uiOwner.uiContext.uiExposure setObserverFrameRate:pageConfig->GetObserverFrameRate()];
  [context.uiOwner.uiContext.uiExposure
      setEnableCheckExposureOptimize:pageConfig->GetEnableCheckExposureOptimize()];

  // Set config to LynxUIContext;
  LynxUIContext *uiContext = context.uiOwner.uiContext;
  // TODO(renzhongyue): Add setPageConfig:(PageConfig) to a private extension of LynxUIContext.
  // And add a init method to enable LynxUIOwner init with a PageConfig directly.
  // Configs are not modifiable after initialization.
  [uiContext setDefaultOverflowVisible:pageConfig->GetDefaultOverflowVisible()];
  [uiContext setEnableTextRefactor:pageConfig->GetEnableTextRefactor()];
  [uiContext setEnableTextOverflow:pageConfig->GetEnableTextOverflow()];
  [uiContext setEnableNewClipMode:pageConfig->GetEnableNewClipMode()];
  [uiContext setDefaultImplicitAnimation:pageConfig->GetGlobalImplicit()];
  [uiContext setEnableEventRefactor:pageConfig->GetEnableEventRefactor()];
  [uiContext setEnableA11yIDMutationObserver:pageConfig->GetEnableA11yIDMutationObserver()];

  [uiContext setEnableEventThrough:pageConfig->GetEnableEventThrough()];
  [uiContext setEnableBackgroundShapeLayer:pageConfig->GetEnableBackgroundShapeLayer()];
  [uiContext setEnableExposureUIMargin:pageConfig->GetEnableExposureUIMargin()];
  [uiContext setEnableTextLanguageAlignment:pageConfig->GetEnableTextLanguageAlignment()];
  [uiContext setEnableXTextLayoutReused:pageConfig->GetEnableXTextLayoutReused()];
  [uiContext setEnableFiberArch:pageConfig->GetEnableFiberArch()];
  [uiContext setEnableNewGesture:pageConfig->GetEnableNewGesture()];
  [uiContext setCSSAlignWithLegacyW3c:pageConfig->GetCSSAlignWithLegacyW3C()];
  [uiContext
      setTargetSdkVersion:[NSString
                              stringWithUTF8String:pageConfig->GetTargetSDKVersion().c_str()]];

  uiContext.imageMonitorEnabled =
      [[LynxEnv sharedInstance] boolFromExternalEnv:LynxEnvEnableImageMonitor defaultValue:YES];
  uiContext.devtoolEnabled = [[LynxEnv sharedInstance] devtoolEnabled];
  uiContext.fixNewImageDownSampling =
      [[LynxEnv sharedInstance] boolFromExternalEnv:LynxEnvFixNewImageDownSampling
                                       defaultValue:YES];
  // If EnableLynxFluency is configured, Lynx will determine whether to enable fluency
  // metics based on this probability when creating a LynxView.
  [[uiContext fluencyInnerListener]
      setFluencyPageconfigProbability:pageConfig->GetEnableScrollFluencyMonitor()];
  if (pageConfig->GetEnableTextLayerRender() == lynx::tasm::TernaryBool::UNDEFINE_VALUE) {
    auto new_value = [[LynxEnv sharedInstance] boolFromExternalEnv:LynxEnvEnableTextLayerRender
                                                      defaultValue:NO];
    pageConfig->SetEnableTextLayerRender(new_value ? lynx::tasm::TernaryBool::TRUE_VALUE
                                                   : lynx::tasm::TernaryBool::FALSE_VALUE);
  }

  [uiContext setEnableTextLayerRender:pageConfig->GetEnableTextLayerRender() ==
                                      lynx::tasm::TernaryBool::TRUE_VALUE];

  [uiContext setEnableTextNonContiguousLayout:pageConfig->GetEnableTextNonContiguousLayout()];
  [uiContext setEnableImageDownsampling:pageConfig->GetEnableImageDownsampling()];
  [uiContext setEnableNewImage:pageConfig->GetEnableNewImage()];
  [uiContext
      setTrailUseNewImage:pageConfig->GetTrailNewImage() == lynx::tasm::TernaryBool::TRUE_VALUE];
  [uiContext setLogBoxImageSizeWarningThreshold:pageConfig->GetLogBoxImageSizeWarningThreshold()];
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

- (UIView *)eventHandlerRootView {
  return nil;
}

- (void)setupWithContainerView:(LynxView *)containerView
              templateRenderer:(LynxTemplateRender *)templateRenderer
                       builder:(LynxViewBuilder *)builder
                    screenSize:(CGSize)screenSize {
  LynxScreenMetrics *screenMetrics =
      [[LynxScreenMetrics alloc] initWithScreenSize:screenSize scale:[UIScreen mainScreen].scale];
  _uiOwner = [[LynxUIOwner alloc] initWithContainerView:(LynxView *)containerView
                                         templateRender:templateRenderer
                                      componentRegistry:builder.config.componentRegistry
                                          screenMetrics:screenMetrics];

  _uiOwner.uiContext.contextDict = [builder.config.contextDict copy];
  _uiOwner.uiContext.lynxModuleExtraData = builder.lynxModuleExtraData;
}

- (void)setLynxContext:(LynxContext *)context {
  _uiOwner.uiContext.lynxContext = context;
}

- (LynxUIOwner *)uiOwner {
  return _uiOwner;
}

- (LynxRootUI *)rootUI {
  return _uiOwner.rootUI;
}

- (void)setEnableGenericResourceFetcher:(BOOL)enable {
  _enableGenericResourceLoader = enable;
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

- (void)setupResourceProvider:(id<LynxResourceProvider>)resourceProvider
                  withBuilder:(LynxViewBuilder *)builder {
  _uiOwner.fontFaceContext.resourceProvider = resourceProvider;

  _uiOwner.fontFaceContext.builderRegistedAliasFontMap = [builder getBuilderRegisteredAliasFontMap];

  if (_enableGenericResourceLoader) {
    _uiOwner.uiContext.genericResourceFetcher = [builder genericResourceFetcher];
    _uiOwner.uiContext.mediaResourceFetcher = [builder mediaResourceFetcher];
    _uiOwner.uiContext.templateResourceFetcher = [builder templateResourceFetcher];

    _uiOwner.fontFaceContext.genericResourceServiceFetcher = [builder genericResourceFetcher];
  }
}

- (void)reset {
  [_uiOwner reset];
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
