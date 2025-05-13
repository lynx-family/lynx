// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxTemplateRenderContext.h>

#import <Lynx/LUIBodyView.h>
#import <Lynx/LynxBackgroundRuntime.h>
#import <Lynx/LynxConfig.h>
#import <Lynx/LynxContext.h>
#import <Lynx/LynxDevtool.h>
#import <Lynx/LynxDynamicComponentFetcher.h>
#import <Lynx/LynxEngineProxy.h>
#import <Lynx/LynxLayoutTick.h>
#import <Lynx/LynxProviderRegistry.h>
#import <Lynx/LynxShadowNodeOwner.h>
#import <Lynx/LynxUILayoutTick.h>
#import <Lynx/LynxUIRendererProtocol.h>
#import <Lynx/LynxViewEnum.h>
#import <Lynx/PaintingContextProxy.h>

#include <memory>

#include "core/services/timing_handler/timing_collector_platform_impl.h"
#include "core/shell/lynx_shell.h"

NS_ASSUME_NONNULL_BEGIN

/**
 * hold necessary members for LynxTemplateRender and LynxFrameRender to setup
 */
@interface LynxTemplateRenderContext () {
 @public
  // input
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
  LynxOnLayoutBlock _layoutBlock;
  LynxThreadStrategyForRender _threadStrategyForRendering;

  id<LynxDynamicComponentFetcher> _fetcher;
  id _lynxModuleExtraData;
  id<LynxUIRendererProtocol> _lynxUIRenderer;

  LynxConfig* _config;
  UIView<LUIBodyView>* _containerView;
  LynxDevtool* _devTool;
  LynxEngineProxy* _lynxEngineProxy;
  LynxProviderRegistry* _providerRegistry;
  LynxBackgroundRuntime* _runtime;
  LynxBackgroundRuntimeOptions* _runtimeOptions;

  // output and input
  NSMutableDictionary* _extra;
  LynxUILayoutTick* _uilayoutTick;

  // output
  std::weak_ptr<lynx::piper::LynxModuleManager> module_manager_;
  std::unique_ptr<lynx::shell::LynxShell> shell_;
  std::shared_ptr<lynx::tasm::timing::TimingCollectorPlatformImpl> timing_collector_platform_impl_;

  NSMutableDictionary<NSString*, id>* _lepusModulesClasses;
  LynxContext* _context;
  PaintingContextProxy* _paintingContextProxy;
  LynxShadowNodeOwner* _shadowNodeOwner;
}

@end

NS_ASSUME_NONNULL_END
