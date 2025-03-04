// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

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

@class LynxView;
@class LynxUIOwner;
@class LynxTemplateRender;
@class LynxTemplateResourceFetcher;
@class LynxViewBuilder;

@protocol LynxResourceProvider;

@protocol LynxUIRendererProtocol <NSObject>

@property(nonatomic, readonly) BOOL useInvokeUIMethodFunction;

- (void)attachLynxView:(LynxView *)lynxView;

- (void)onSetupUIDelegate:(lynx::tasm::UIDelegate *)uiDelegate;

- (void)onSetupUIDelegate:(lynx::shell::LynxShell *)shell
        withModuleManager:(lynx::piper::LynxModuleManager *)moduleManager
              withJSProxy:(std::shared_ptr<lynx::shell::LynxRuntimeProxy>)jsProxy;

- (lynx::tasm::UIDelegate *)uiDelegate;

- (void)setupEventHandler:(LynxTemplateRender *)templateRenderer
              engineProxy:(LynxEngineProxy *)engineProxy
                 lynxView:(LynxView *)lynxView
                  context:(LynxContext *)context
                 shellPtr:(int64_t)shellPtr;

- (void)setPageConfig:(const std::shared_ptr<lynx::tasm::PageConfig> &)pageConfig
              context:(LynxContext *)context;

- (BOOL)needPaintingContextProxy;

- (void)onSetFrame:(CGRect)frame;

- (nullable LynxUIIntersectionObserverManager *)getLynxUIIntersectionObserverManager;

- (BOOL)needHandleHitTest;

- (UIView *)hitTest:(CGPoint)point withEvent:(UIEvent *)event;

- (id<LynxEventTarget>)hitTestInEventHandler:(CGPoint)point withEvent:(UIEvent *)event;

- (UIView *)eventHandlerRootView;

- (LynxUIOwner *)uiOwner;

- (LynxRootUI *)rootUI;

- (void)setupWithContainerView:(LynxView *)containerView
              templateRenderer:(LynxTemplateRender *)templateRenderer
                       builder:(LynxViewBuilder *)builder
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
