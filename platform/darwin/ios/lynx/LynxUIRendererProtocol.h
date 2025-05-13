// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_DARWIN_IOS_LYNX_LYNXUIRENDERERPROTOCOL_H_
#define PLATFORM_DARWIN_IOS_LYNX_LYNXUIRENDERERPROTOCOL_H_

#import <Foundation/Foundation.h>

#import <Lynx/LynxBooleanOption.h>
#import "LynxTemplateRender+Protected.h"

#include <objc/objc.h>

namespace lynx {
namespace tasm {
class UIDelegate;
}  // namespace tasm
namespace shell {
class LynxShell;
}  // namespace shell
namespace piper {
class LynxModuleManager;
}  // namespace piper
}  // namespace lynx

NS_ASSUME_NONNULL_BEGIN

@class LynxUIOwner;
@class LynxTemplateResourceFetcher;
@class LynxViewBuilder;
@class LynxTemplateRenderContext;

@protocol LynxResourceProvider;

@protocol LynxUIRendererProtocol <NSObject>

@property(nonatomic, readonly) BOOL useInvokeUIMethodFunction;

- (void)attachContainerView:(UIView<LUIBodyView> *)containerView;

- (void)onSetupUIDelegate:(lynx::tasm::UIDelegate *)uiDelegate;

- (void)onSetupUIDelegate:(lynx::shell::LynxShell *)shell
        withModuleManager:(lynx::piper::LynxModuleManager *)moduleManager
              withJSProxy:(std::shared_ptr<lynx::shell::LynxRuntimeProxy>)jsProxy;

- (lynx::tasm::UIDelegate *)uiDelegate;

- (void)setupEventHandler:(id<TemplateRenderCallbackProtocol>)templateRenderer
              engineProxy:(LynxEngineProxy *)engineProxy
            containerView:(UIView<LUIBodyView> *)containerView
                  context:(LynxContext *)context
                 shellPtr:(int64_t)shellPtr;

- (void)setPageConfig:(const std::shared_ptr<lynx::tasm::PageConfig> &)pageConfig
              context:(LynxContext *)context;

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
                       context:(LynxTemplateRenderContext *)context
                    screenSize:(CGSize)screenSize;

- (void)setLynxContext:(LynxContext *)context;

- (void)setEnableGenericResourceFetcher:(BOOL)enable;

- (id<LynxTemplateResourceFetcher>)templateResourceFetcher;

- (id<LynxGenericResourceFetcher>)genericResourceFetcher;

- (void)setupResourceProvider:(id<LynxResourceProvider>)resourceProvider
                  withBuilder:(LynxViewBuilder *)builder;

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
