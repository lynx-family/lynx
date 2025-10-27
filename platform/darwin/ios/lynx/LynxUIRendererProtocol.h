// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_DARWIN_IOS_LYNX_LYNXUIRENDERERPROTOCOL_H_
#define PLATFORM_DARWIN_IOS_LYNX_LYNXUIRENDERERPROTOCOL_H_

#import <Foundation/Foundation.h>

#import <Lynx/LynxBooleanOption.h>
#import <Lynx/LynxContext.h>
#import <Lynx/LynxEventEmitter.h>
#import <Lynx/LynxUIMethodProcessor.h>

NS_ASSUME_NONNULL_BEGIN

@class LynxUIOwner;
@class LynxTemplateResourceFetcher;
@class LynxViewBuilder;
@class LynxUIIntersectionObserverManager;
@class LynxRootUI;
@class LynxScreenMetrics;
@class LynxGestureArenaManager;
@class LynxShadowNodeOwner;

@protocol LynxResourceProvider;

@protocol LynxUIRendererProtocol <NSObject>

@property(nonatomic, readonly) BOOL useInvokeUIMethodFunction;

- (instancetype)initWithLynxContext:(LynxContext *)context
                      containerView:(UIView<LUIBodyView> *)containerView
                            builder:(LynxViewBuilder *)builder
                   providerRegistry:(LynxProviderRegistry *)providerRegistry;

- (void)attachContainerView:(UIView<LUIBodyView> *)containerView;

- (void)setupUIDelegate:(LynxShadowNodeOwner *)owner;

- (void *)uiDelegate;

- (void)setupEventHandler:(LynxEngineProxy *)engineProxy
                 shellPtr:(int64_t)shellPtr
                    block:(onLynxEvent)block;

- (void)setFluencyTracerEnabled:(LynxBooleanOption)enabled;

- (BOOL)needPaintingContextProxy;

- (void)onSetFrame:(CGRect)frame;

- (nullable LynxUIIntersectionObserverManager *)getLynxUIIntersectionObserverManager;

- (BOOL)needHandleHitTest;

- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event;

- (id<LynxEventTarget>)hitTestInEventHandler:(CGPoint)point withEvent:(UIEvent *)event;

- (void)handleFocus:(id<LynxEventTarget>)target
             onView:(UIView *)view
      withContainer:(UIView *)container
           andPoint:(CGPoint)point
           andEvent:(UIEvent *)event;

- (UIView *)eventHandlerRootView;

- (LynxUIOwner *)uiOwner;

- (LynxRootUI *)rootUI;

- (void)setupWithContainerView:(UIView<LUIBodyView> *)containerView
                       builder:(LynxViewBuilder *)builder
                    screenSize:(CGSize)screenSize;

- (id<LynxTemplateResourceFetcher>)templateResourceFetcher;

- (id<LynxGenericResourceFetcher>)genericResourceFetcher;

- (id<LynxMediaResourceFetcher>)mediaResourceFetcher;

- (void)reset;

- (LynxScreenMetrics *)getScreenMetrics;

- (LynxGestureArenaManager *)getGestureArenaManager;

- (void)onEnterForeground;

- (void)onEnterBackground;

- (void)willMoveToWindow:(UIWindow *)newWindow;

- (void)didMoveToWindow:(BOOL)windowIsNil;

#pragma mark - View

- (void)setCustomizedLayoutInUIContext:(id<LynxListLayoutProtocol> _Nullable)customizedListLayout;

- (void)setScrollListener:(id<LynxScrollListener>)scrollListener;

- (void)setImageFetcherInUIOwner:(id<LynxImageFetcher>)imageFetcher;

- (void)setResourceFetcherInUIOwner:(id<LynxResourceFetcher>)resourceFetcher;

- (void)updateScreenWidth:(CGFloat)width height:(CGFloat)height;

- (void)pauseRootLayoutAnimation;

- (void)resumeRootLayoutAnimation;

- (void)restartAnimation;

- (void)resetAnimation;

- (void)invokeUIMethodForSelectorQuery:(NSString *)method
                                params:(NSDictionary *)params
                              callback:(LynxUIMethodCallbackBlock)callback
                                toNode:(int)sign;

- (BOOL)isFullScreenShotSupported;

- (NSArray<NSNumber *> *)getTransformValue:(NSInteger)sign
                 withPadBorderMarginLayout:(NSArray<NSNumber *> *)padBorderMarginLayout;

- (int)findNodeIdForLocationWithX:(float)x withY:(float)y mode:(NSString *)mode;

#pragma mark - Find Node

- (LynxUI *)findUIBySign:(NSInteger)sign;

- (nullable UIView *)findViewWithName:(nonnull NSString *)name;

- (nullable LynxUI *)uiWithName:(nonnull NSString *)name;

- (nullable LynxUI *)uiWithIdSelector:(nonnull NSString *)idSelector;

- (nullable UIView *)viewWithIdSelector:(nonnull NSString *)idSelector;

- (nullable UIView *)viewWithName:(nonnull NSString *)name;

@end

NS_ASSUME_NONNULL_END

#endif  // PLATFORM_DARWIN_IOS_LYNX_LYNXUIRENDERERPROTOCOL_H_
