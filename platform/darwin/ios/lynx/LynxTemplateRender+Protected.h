// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxRuntimeLifecycleListener.h>
#import <Lynx/LynxTemplateRender.h>
#import <Lynx/LynxTemplateRenderDelegate.h>
#import <Lynx/LynxViewEnum.h>
#import <Lynx/TemplateRenderCallbackProtocol.h>
#import "LynxPerformanceController.h"

#include <memory>

#include "core/renderer/ui_wrapper/painting/ios/ui_delegate_darwin.h"
#include "core/runtime/js/bindings/modules/ios/module_factory_darwin.h"
#include "core/shell/lynx_shell.h"
#include "core/template_bundle/template_codec/binary_decoder/page_config.h"

@class LynxContext;
@class LynxTemplateData;
@class LynxConfig;
@class PaintingContextProxy;
@class LynxUILayoutTick;
@class LynxShadowNodeOwner;
@class LynxEventHandler;
@class LynxEventEmitter;
@class LynxKeyboardEventDispatcher;
@class LynxBackgroundRuntime;
@class LynxBackgroundRuntimeOptions;
@class LynxTheme;
@class LynxProviderRegistry;
@class LynxEngineProxy;
@class LynxSSRHelper;
@class LynxView;
@class LynxViewBuilder;
@class LynxLifecycleDispatcher;
@class LynxViewGroup;
typedef NS_ENUM(NSInteger, LynxBooleanOption);

@protocol LynxDynamicComponentFetcher;
@protocol LynxUIRendererProtocol;

NS_ASSUME_NONNULL_BEGIN

@interface LynxTemplateRender () <TemplateRenderCallbackProtocol> {
 @protected
  BOOL _enableAsyncDisplayFromNative;
  BOOL _enableImageDownsampling;
  BOOL _enableTextNonContiguousLayout;
  BOOL _enableLayoutOnly;
  LynxEmbeddedMode _embeddedMode;

  BOOL _hasStartedLoad;
  BOOL _enableLayoutSafepoint;
  BOOL _enableAutoExpose;
  BOOL _enableAirStrictMode;
  BOOL _needPendingUIOperation;
  BOOL _enablePendingJSTaskOnLayout;
  BOOL _enablePreUpdateData;
  BOOL _enableAsyncHydration;
  BOOL _enableMultiAsyncThread;
  BOOL _enableJSGroupThread;
  BOOL _enableVSyncAlignedMessageLoop;
  BOOL _enableUnifiedPipeline;
  BOOL _enableReuseEngine;
  BOOL _isEngineInitFromReusePool;
  BOOL _enableMTSModule;
  BOOL _isMemoryCollecting;

  LynxViewBuilder* _builder;
  LynxConfig* _config;
  LynxContext* _context;
  LynxUILayoutTick* _uilayoutTick;
  LynxShadowNodeOwner* _shadowNodeOwner;
  LynxThreadStrategyForRender _threadStrategyForRendering;
  LynxBackgroundRuntime* _runtime;
  LynxBackgroundRuntimeOptions* _runtimeOptions;
  LynxGroup* _group;
  LynxTheme* _localTheme;
  LynxTemplateData* _globalProps;
  LynxTemplateData* _templateData;
  PaintingContextProxy* _paintingContextProxy;
  LynxSSRHelper* _lynxSSRHelper;
  LynxPerformanceController* _performanceController;
  LynxEngine* _lynxEngine;
  LynxViewGroup* _lynxViewGroup;
  CGFloat _fontScale;
  CGSize _intrinsicContentSize;
  std::unique_ptr<lynx::shell::LynxShell> shell_;
  std::shared_ptr<lynx::tasm::PageConfig> pageConfig_;
  std::weak_ptr<lynx::piper::ModuleFactoryDarwin> module_factory_;
  std::weak_ptr<lynx::piper::ModuleFactoryDarwin> main_thread_module_factory_;
  id<LynxUIRendererProtocol> _lynxUIRenderer;
  // property
  NSMutableDictionary* _extra;
  NSMutableDictionary<NSString*, id>* _originLynxViewConfig;
  LynxProviderRegistry* _providerRegistry;
  id<LynxDynamicComponentFetcher> _fetcher;
  LynxEngineProxy* _lynxEngineProxy;
  long _initStartTiming;
  long _initEndTiming;
  id _lynxModuleExtraData;
  __weak UIView<LUIBodyView>* _containerView;

  __weak id<LynxTemplateRenderDelegate> _delegate;

  LynxViewSizeMode _layoutWidthMode;
  LynxViewSizeMode _layoutHeightMode;
  CGFloat _preferredMaxLayoutWidth;
  CGFloat _preferredMaxLayoutHeight;
  CGFloat _preferredLayoutWidth;
  CGFloat _preferredLayoutHeight;
  CGRect _frameOfLynxView;
  BOOL _isDestroyed;
  BOOL _hasRendered;
  NSString* _url;
  BOOL _enableJSRuntime;
  LynxDevtool* _devTool;
  BOOL _enablePrePainting;
  BOOL _enableDumpElement;
  BOOL _enableRecycleTemplateBundle;
  NSMutableDictionary<NSString*, id>* _lepusModulesClasses;

  BOOL _enableGenericResourceFetcher;
  BOOL _debuggable;
}

- (lynx::piper::ModuleFactoryDarwin*)getModuleFactory;

- (LynxLifecycleDispatcher*)getLifecycleDispatcher;

@end

NS_ASSUME_NONNULL_END
