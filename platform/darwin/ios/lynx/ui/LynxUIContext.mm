// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxUIContext.h"
#import "LUIErrorHandling.h"
#import "LynxContext+Internal.h"
#import "LynxEnv.h"
#import "LynxError.h"
#import "LynxEventHandler.h"
#import "LynxGlobalObserver.h"
#import "LynxResourceServiceFetcher.h"
#import "LynxRootUI.h"
#import "LynxScrollFluency.h"
#import "LynxService.h"
#import "LynxServiceDevToolProtocol.h"
#import "LynxSubErrorCode.h"
#import "LynxTemplateRender+Internal.h"
#import "LynxTouchHandler+Internal.h"
#import "LynxUIContext+Internal.h"
#import "LynxUIExposure+Internal.h"
#import "LynxUIIntersectionObserver.h"
#import "LynxUIOwner.h"
#import "LynxUIRendererProtocol.h"
#import "LynxUIReportInfoDelegate.h"
#import "LynxView+Internal.h"

@implementation LynxUIContext {
  BOOL _isDev;
}

- (instancetype)initWithScreenMetrics:(LynxScreenMetrics*)screenMetrics {
  if (self = [super init]) {
    _screenMetrics = screenMetrics;
    _isDev = [LynxService(LynxServiceDevToolProtocol) inspectorOwnerClass] ? YES : NO;
    // Be in charge of notifying exposure detection can be executed when the UI has changed.
    _observer = [[LynxGlobalObserver alloc] init];
    _uiExposure = [[LynxUIExposure alloc] initWithObserver:_observer];
    _fluencyInnerListener = [[LynxScrollFluency alloc] init];
    _defaultAutoResumeAnimation = [LynxEnv.sharedInstance getAutoResumeAnimation];
    _defaultEnableNewTransformOrigin = [LynxEnv.sharedInstance getEnableNewTransformOrigin];
    if ([LynxResourceServiceFetcher ensureLynxService]) {
      _resourceServiceFetcher = [[LynxResourceServiceFetcher alloc] init];
    }
    _enableTextNonContiguousLayout = YES;
    _enableImageDownsampling = NO;
    _enableNewImage = YES;
    _trailUseNewImage = NO;
    _logBoxImageSizeWarningThreshold = -1;
  }
  return self;
}

- (void)updateScreenSize:(CGSize)screenSize {
  [_screenMetrics setLynxScreenSize:screenSize];
}

- (void)onGestureRecognized {
  if (self.eventHandler != nil) {
    [self.eventHandler onGestureRecognized];
  }
}

- (void)onGestureRecognizedByUI:(LynxUI*)ui {
  if (self.eventHandler != nil) {
    [self.eventHandler onGestureRecognizedByEventTarget:ui];
  }
}

- (void)onPropsChangedByUI:(LynxUI*)ui {
  if (self.eventHandler != nil) {
    [self.eventHandler onPropsChangedByEventTarget:ui];
  }
}

- (BOOL)isTouchMoving {
  return [self.eventHandler.touchRecognizer isTouchMoving];
}

// deprecated: use didReceiveResourceError:withSource:type: instead
- (void)didReceiveResourceError:(NSError*)error {
  if (_errorHandler && [_errorHandler respondsToSelector:@selector(didReceiveResourceError:)]) {
    [_errorHandler didReceiveResourceError:error];
  }
}

- (void)didReceiveResourceError:(LynxError*)error
                     withSource:(NSString*)resourceUrl
                           type:(NSString*)type {
  [_errorHandler didReceiveResourceError:error withSource:resourceUrl type:type];
}

- (void)reportLynxError:(LynxError*)error {
  [_errorHandler reportLynxError:error];
}

/// Deprecated: use reportLynxError instead
- (void)reportError:(NSError*)error {
  [_errorHandler reportError:error];
}

- (void)didReceiveException:(NSException*)exception
                withMessage:(NSString*)message
                      forUI:(LynxUI*)ui {
  // stack message
  NSArray<NSString*>* stacks = exception.callStackSymbols;
  NSMutableString* stackInfo = [NSMutableString string];
  // save the first 20 lines of information on the stack
  for (NSUInteger i = 0; i < MIN(stacks.count, (NSUInteger)20); ++i) {
    [stackInfo appendFormat:@"%@\n", stacks[i]];
  }
  NSMutableDictionary* userInfo = [[NSMutableDictionary alloc] init];
  [userInfo setValue:message forKey:LynxErrorUserInfoKeyMessage];
  [userInfo setValue:stackInfo forKey:LynxErrorUserInfoKeyStackInfo];
  // custom info
  if ([ui conformsToProtocol:@protocol(LynxUIReportInfoDelegate)]) {
    if ([ui respondsToSelector:@selector(reportUserInfoOnError)]) {
      [userInfo setValue:[(id<LynxUIReportInfoDelegate>)ui reportUserInfoOnError]
                  forKey:LynxErrorUserInfoKeyCustomInfo];
    }
  }
  LynxError* error = [LynxError lynxErrorWithCode:ECLynxExceptionPlatform userInfo:userInfo];
  [self didReceiveResourceError:error];
}

- (NSNumber*)getLynxRuntimeId {
  UIView* rootView = [self rootView];
  if (rootView != nil && [rootView respondsToSelector:@selector(getLynxRuntimeId)]) {
    return [rootView performSelector:@selector(getLynxRuntimeId)];
  }
  return nil;
}

- (nullable id<LynxResourceFetcher>)getGenericResourceFetcher {
  if (_resourceFetcher != nil &&
      [_resourceFetcher respondsToSelector:@selector
                        (requestAsyncWithResourceRequest:type:lynxResourceLoadCompletedBlock:)] &&
      [_resourceFetcher respondsToSelector:@selector(requestSyncWithResourceRequest:type:)]) {
    return _resourceFetcher;
  }
  return _resourceServiceFetcher;
}

- (BOOL)isDev {
  return _isDev && [[LynxEnv sharedInstance] automationEnabled];
}

- (void)addUIToExposedMap:(LynxUI*)ui {
  [self addUIToExposedMap:ui withUniqueIdentifier:nil extraData:nil useOptions:nil];
}

- (void)addUIToExposedMap:(LynxUI*)ui
     withUniqueIdentifier:(NSString*)uniqueID
                extraData:(NSDictionary*)data
               useOptions:(NSDictionary*)options {
  // Since the exposure node using the custom event method can also set the exposure-related
  // properties, we choose to check whether the node is bound to the relevant event here to avoid
  // modifying it in multiple places.
  if (!uniqueID &&
      ([ui.eventSet objectForKey:@"uiappear"] || [ui.eventSet objectForKey:@"uidisappear"])) {
    [_uiExposure addLynxUI:ui
        withUniqueIdentifier:[NSString stringWithFormat:@"%ld", (long)ui.sign]
                   extraData:data
                  useOptions:@{@"sendCustom" : @YES}];
  }

  // Native components can call this interface to reuse the exposure capability.
  [_uiExposure addLynxUI:ui withUniqueIdentifier:uniqueID extraData:data useOptions:options];
}

- (void)removeUIFromExposedMap:(LynxUI*)ui {
  [self removeUIFromExposedMap:ui withUniqueIdentifier:nil];
}

- (void)removeUIFromExposedMap:(LynxUI*)ui withUniqueIdentifier:(NSString*)uniqueID {
  // Since the exposure node using the custom event method can also set the exposure-related
  // properties, we choose to check whether the node is bound to the relevant event here to avoid
  // modifying it in multiple places.
  if (!uniqueID &&
      ([ui.eventSet objectForKey:@"uiappear"] || [ui.eventSet objectForKey:@"uidisappear"])) {
    [_uiExposure removeLynxUI:ui
         withUniqueIdentifier:[NSString stringWithFormat:@"%ld", (long)ui.sign]];
  }

  // Native components can call this interface to reuse the exposure capability.
  [_uiExposure removeLynxUI:ui withUniqueIdentifier:uniqueID];
}

- (LynxUIIntersectionObserverManager*)intersectionManager {
  // TODO(hexionghui): move intersectionObserver into LynxUI Lib. To enable intersection observer
  // can work out of Lynx Template Context.
  return [_rootUI.lynxView.templateRender.lynxUIRenderer getLynxUIIntersectionObserverManager];
}

- (void)removeUIFromIntersectionManager:(LynxUI*)ui {
  if (![self intersectionManager].enableNewIntersectionObserver) {
    return;
  }
  [[self intersectionManager] removeAttachedIntersectionObserver:ui];
}

#pragma mark - Page configs

- (void)setDefaultOverflowVisible:(BOOL)enable {
  _defaultOverflowVisible = enable;
}

- (void)setEnableTextNonContiguousLayout:(BOOL)enable {
  _enableTextNonContiguousLayout = enable;
}

- (void)setEnableTextLayerRender:(BOOL)enable {
  _enableTextLayerRender = enable;
}

- (void)setDefaultImplicitAnimation:(BOOL)enable {
  _defaultImplicitAnimation = enable;
}

- (void)setEnableTextRefactor:(BOOL)enable {
  _enableTextRefactor = enable;
}

- (void)setEnableTextOverflow:(BOOL)enable {
  _enableTextOverflow = enable;
}

- (void)setEnableNewClipMode:(BOOL)enable {
  _enableNewClipMode = enable;
}

- (void)setEnableEventRefactor:(BOOL)enable {
  _enableEventRefactor = enable;
}

- (void)setEnableA11yIDMutationObserver:(BOOL)enable {
  _enableA11yIDMutationObserver = enable;
}

- (void)setEnableEventThrough:(BOOL)enable {
  _enableEventThrough = enable;
}

- (void)setEnableBackgroundShapeLayer:(BOOL)enable {
  _enableBackgroundShapeLayer = enable;
}

- (void)setEnableFiberArch:(BOOL)enable {
  _enableFiberArch = enable;
}

- (void)setEnableExposureUIMargin:(BOOL)enable {
  _enableExposureUIMargin = enable;
}

- (void)setEnableNewGesture:(BOOL)enable {
  _enableNewGesture = enable;
}

- (void)setEnableTextLanguageAlignment:(BOOL)enable {
  _enableTextLanguageAlignment = enable;
}

- (void)setEnableXTextLayoutReused:(BOOL)enableXTextLayoutReused {
  _enableXTextLayoutReused = enableXTextLayoutReused;
}

- (void)setCSSAlignWithLegacyW3c:(BOOL)algin {
  _cssAlignWithLegacyW3c = algin;
}

- (void)setTargetSdkVersion:(NSString*)version {
  _targetSdkVersion = version;
}

- (void)setEnableImageDownsampling:(BOOL)enable {
  _enableImageDownsampling = enable;
}

- (void)setEnableNewImage:(BOOL)enable {
  _enableNewImage = enable;
}

- (void)setTrailUseNewImage:(BOOL)enable {
  _trailUseNewImage = enable;
}

- (void)setLogBoxImageSizeWarningThreshold:(NSInteger)threshold {
  _logBoxImageSizeWarningThreshold = threshold;
}

- (int32_t)instanceId {
  return self.rootView.instanceId;
}

@end
