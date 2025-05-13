// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBaseTemplateRender.h>

#import <Lynx/LUIBodyView.h>
#import <Lynx/LynxBackgroundRuntime.h>
#import <Lynx/LynxConfig.h>
#import <Lynx/LynxContext.h>
#import <Lynx/LynxDevtool.h>
#import <Lynx/LynxDynamicComponentFetcher.h>
#import <Lynx/LynxEngineProxy.h>
#import <Lynx/LynxLayoutTick.h>
#import <Lynx/LynxLifecycleDispatcher.h>
#import <Lynx/LynxPerformanceController.h>
#import <Lynx/LynxProviderRegistry.h>
#import <Lynx/LynxShadowNodeOwner.h>
#import <Lynx/LynxUILayoutTick.h>
#import <Lynx/LynxViewBuilder.h>
#import <Lynx/LynxViewEnum.h>
#import <Lynx/PaintingContextProxy.h>
#import <Lynx/TemplateRenderCallbackProtocol.h>

#include <memory>

#include "core/public/painting_ctx_platform_impl.h"
#include "core/shell/lynx_shell.h"

@protocol LynxUIRendererProtocol;

/**
 *internal class helping to setup UIRender, LynxShell and Event
 */
@interface LynxBaseTemplateRender () <TemplateRenderCallbackProtocol> {
 @protected
  BOOL _enableAsyncHydration;
  BOOL _enableJSGroupThread;
  BOOL _enableJSRuntime;
  BOOL _enableLayoutOnly;
  BOOL _enableMultiAsyncThread;
  BOOL _enablePendingJSTaskOnLayout;
  BOOL _enablePreUpdateData;
  BOOL _enableVSyncAlignedMessageLoop;
  BOOL _enableUnifiedPipeline;
  BOOL _needPendingUIOperation;

  EmbeddedMode _embeddedMode;
  CGFloat _fontScale;
  LynxThreadStrategyForRender _threadStrategyForRendering;

  id<LynxDynamicComponentFetcher> _fetcher;
  id _lynxModuleExtraData;
  id<LynxUIRendererProtocol> _lynxUIRenderer;

  LynxConfig* _config;
  LynxContext* _context;
  LynxDevtool* _devTool;
  NSMutableDictionary* _extra;
  NSMutableDictionary<NSString*, id>* _lepusModulesClasses;
  LynxLifecycleDispatcher* _lifecycleDispatcher;
  LynxEngineProxy* _lynxEngineProxy;
  PaintingContextProxy* _paintingContextProxy;
  LynxPerformanceController* _performanceController;
  LynxProviderRegistry* _providerRegistry;
  LynxBackgroundRuntime* _runtime;
  LynxBackgroundRuntimeOptions* _runtimeOptions;
  LynxShadowNodeOwner* _shadowNodeOwner;
  LynxUILayoutTick* _uilayoutTick;

  std::weak_ptr<lynx::piper::LynxModuleManager> module_manager_;
  std::unique_ptr<lynx::shell::LynxShell> shell_;
}

- (UIView<LUIBodyView>*)containerView;

- (void)setUpWithBuilder:(LynxViewBuilder*)builder screenSize:(CGSize)screenSize;

- (void)reset:(int32_t)lastInstanceId;

- (void)setUpPerformanceController:
    (const std::unique_ptr<lynx::tasm::PaintingCtxPlatformImpl>&)painting_context;

@end
