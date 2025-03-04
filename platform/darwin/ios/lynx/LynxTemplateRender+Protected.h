// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTemplateRender.h"
#import "LynxTemplateRenderDelegate.h"
#import "LynxViewEnum.h"
#import "TemplateRenderCallbackProtocol.h"

#include <memory>

#include "core/renderer/page_config.h"
#include "core/renderer/ui_wrapper/painting/ios/ui_delegate_darwin.h"
#include "core/runtime/bindings/jsi/modules/ios/module_factory_darwin.h"
#include "core/services/timing_handler/timing_collector_platform_impl.h"
#include "core/shell/lynx_shell.h"

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

  LynxConfig* _config;
  LynxContext* _context;
  LynxUILayoutTick* _uilayoutTick;
  LynxShadowNodeOwner* _shadowNodeOwner;
  LynxThreadStrategyForRender _threadStrategyForRendering;
  LynxBackgroundRuntime* _runtime;
  LynxBackgroundRuntimeOptions* _runtimeOptions;
  LynxTheme* _localTheme;
  LynxTemplateData* _globalProps;
  PaintingContextProxy* _paintingContextProxy;
  LynxSSRHelper* _lynxSSRHelper;

  CGFloat _fontScale;
  CGSize _intrinsicContentSize;
  std::unique_ptr<lynx::shell::LynxShell> shell_;
  std::shared_ptr<lynx::tasm::PageConfig> pageConfig_;
  std::shared_ptr<lynx::tasm::timing::TimingCollectorPlatformImpl> timing_collector_platform_impl_;
  std::weak_ptr<lynx::piper::LynxModuleManager> module_manager_;
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
  __weak LynxView* _lynxView;

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
}

- (lynx::piper::ModuleFactoryDarwin*)getModuleFactory;

@end

NS_ASSUME_NONNULL_END
