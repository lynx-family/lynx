// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/JSModule.h>
#import <Lynx/LynxDebugger.h>
#import <Lynx/LynxDevtool+Internal.h>
#import <Lynx/LynxError.h>
#import <Lynx/LynxEventReporter.h>
#import <Lynx/LynxFontFaceManager.h>
#import <Lynx/LynxLoadMeta.h>
#import <Lynx/LynxLog.h>
#import <Lynx/LynxPerformanceEntryConverter.h>
#import <Lynx/LynxRootUI.h>
#import <Lynx/LynxScreenMetrics.h>
#import <Lynx/LynxService.h>
#import <Lynx/LynxServiceExtensionProtocol.h>
#import <Lynx/LynxSetModule.h>
#import <Lynx/LynxSubErrorCode.h>
#import <Lynx/LynxTemplateBundle.h>
#import <Lynx/LynxTemplateData.h>
#import <Lynx/LynxTemplateRender.h>
#import <Lynx/LynxTemplateRenderHelper.h>
#import <Lynx/LynxTheme.h>
#import <Lynx/LynxTraceEvent.h>
#import <Lynx/LynxTraceEventDef.h>
#import <Lynx/LynxView.h>
#import "LynxAccessibilityModule.h"
#import "LynxBackgroundRuntime+Internal.h"
#import "LynxCallStackUtil.h"
#import "LynxConfig+Internal.h"
#import "LynxContext+Internal.h"
#import "LynxEngine.h"
#import "LynxEnginePool.h"
#import "LynxEngineProxy+Native.h"
#import "LynxEnv+Internal.h"
#import "LynxEventReporterUtils.h"
#import "LynxExposureModule.h"
#import "LynxFetchModule.h"
#import "LynxGroup+Internal.h"
#import "LynxIntersectionObserverModule.h"
#import "LynxResourceModule.h"
#import "LynxSSRHelper.h"
#import "LynxTemplateBundle+Converter.h"
#import "LynxTemplateData+Converter.h"
#import "LynxTemplateRender+Protected.h"
#import "LynxTextInfoModule.h"
#import "LynxTimingConstants.h"
#import "LynxUIIntersectionObserver+Internal.h"
#import "LynxUILayoutTick.h"
#import "LynxUIMethodModule.h"
#import "LynxUIRenderer.h"
#import "LynxUIRendererProtocol.h"
#import "LynxViewBuilder+Internal.h"
#import "PaintingContextProxy.h"

#include <functional>

#include "base/include/debug/backtrace.h"
#include "core/base/darwin/lynx_env_darwin.h"
#include "core/public/lynx_extension_delegate.h"
#include "core/public/pipeline_option.h"
#include "core/renderer/data/ios/platform_data_darwin.h"
#include "core/renderer/dom/ios/lepus_value_converter.h"
#include "core/renderer/lynx_global_pool.h"
#include "core/renderer/ui_wrapper/common/ios/prop_bundle_darwin.h"
#include "core/renderer/ui_wrapper/layout/ios/layout_context_darwin.h"
#include "core/renderer/ui_wrapper/painting/ios/painting_context_darwin.h"
#include "core/renderer/utils/darwin/event_converter_darwin.h"
#include "core/resource/lazy_bundle/lazy_bundle_loader.h"
#include "core/resource/lynx_resource_loader_darwin.h"
#include "core/runtime/vm/lepus/json_parser.h"
#include "core/services/performance/darwin/performance_controller_darwin.h"
#include "core/services/timing_handler/timing_constants.h"
#include "core/shell/ios/data_utils.h"
#include "core/shell/ios/lynx_layout_proxy_darwin.h"
#include "core/shell/ios/native_facade_darwin.h"
#include "core/shell/ios/tasm_platform_invoker_darwin.h"
#include "core/shell/lynx_shell_builder.h"
#include "core/shell/module_delegate_impl.h"
#include "core/value_wrapper/darwin/value_impl_darwin.h"

@implementation LynxTemplateRender

#pragma mark - Initialization process

LYNX_NOT_IMPLEMENTED(-(instancetype)initWithCoder : (NSCoder*)aDecoder)

- (BOOL)checkEnableGenericResourceFetcher:(LynxBooleanOption)enable {
  if (enable == LynxBooleanOptionUnset) {
    return [[LynxEnv sharedInstance] enableGenericResourceFetcher];
  }
  return enable == LynxBooleanOptionTrue;
}

- (instancetype)initWithBuilderBlock:(LynxViewBuilderBlock)block
                            lynxView:(LynxView* _Nullable)lynxView {
  if (self = [self initWithBuilderBlock:block containerView:lynxView]) {
  }
  return self;
}

- (instancetype)initWithBuilderBlock:(LynxViewBuilderBlock)block
                       containerView:(UIView<LUIBodyView>*)containerView {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_RENDER_INIT_WITH_BUILDER_BLOCK);
  if (self = [super init]) {
    _initStartTiming = [[NSDate date] timeIntervalSince1970] * 1000 * 1000;

    /// Builder
    LynxViewBuilder* builder = [self setUpBuilder];
    if (block) {
      TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_RENDER_CUSTOM_BUILDER);
      block(builder);
      [LynxTrail parseLynxViewBuilder:builder];
    }

    /// Runtime Options
    _runtime = builder.lynxBackgroundRuntime;
    // Avoid unexpected changes
    _runtimeOptions =
        [[LynxBackgroundRuntimeOptions alloc] initWithOptions:builder.lynxBackgroundRuntimeOptions];
    if (_runtime) {
      if (![_runtime attachToLynxView]) {
        _LogE(@"Create a LynxView with an invalid LynxBackgroundRuntime, returning nil");
        return nil;
      }

      // If user uses LynxBackgroundRuntime to create LynxView, we should merge
      // the LynxBackgroundRuntimeOptions.
      _runtime.options.pendingCoreJsLoad = NO;
      [_runtimeOptions merge:_runtime.options];
    }

    [builder.lynxUIRenderer attachContainerView:containerView];

    /// Member Variable
    CGSize screenSize;
    if (!CGSizeEqualToSize(builder.screenSize, CGSizeZero)) {
      screenSize = builder.screenSize;
    } else {
      screenSize = [UIScreen mainScreen].bounds.size;
    }
    [self setUpVariableWithBuilder:builder containerView:containerView screenSize:screenSize];

    /// DevTool
    [self setUpDevTool:builder.debuggable];

    /// UIRender + LynxShell + Event
    [self setUpWithBuilder:builder screenSize:screenSize];

    // Update info
    [self updateNativeTheme];
    [self updateNativeGlobalProps];

    /// Frame
    [self setUpFrame:builder.frame];

    /// Timing
    _initEndTiming = [[NSDate date] timeIntervalSince1970] * 1000 * 1000;
    [self setUpTiming];
  }

  // Destruction of Runtime inside wrapper will be handled by LynxShell. Since after
  // attachement, user cannot use LynxBackgroundRuntime, we can safely release its reference.
  // To not influence the process of recreating LynxShell, we should set _runtime nil here.
  _runtime = nil;

  return self;
}

- (LynxViewBuilder*)setUpBuilder {
  LynxViewBuilder* builder = [LynxViewBuilder new];
  [builder setThreadStrategyForRender:LynxThreadStrategyForRenderAllOnUI];
  builder.enableJSRuntime = YES;
  builder.frame = CGRectZero;
  builder.screenSize = CGSizeZero;
  return builder;
}

- (void)reuseLynxEngine {
  _lynxEngine = [[LynxEnginePool sharedInstance] pollEngineWithRender:_templateBundle];
  if (!_lynxEngine) {
    _lynxEngine = [[LynxEngine alloc] initWithTemplateRender:self];
    _isEngineInitFromReusePool = NO;
  } else {
    [_lynxEngine setEngineState:LynxEngineStateOnReusing];
    _lynxUIRenderer = [_lynxEngine lynxUIRenderer];
    _shadowNodeOwner = [_lynxEngine shadowNodeOwner];
    // TODO(renzhongyue) : attachBodyView
    _isEngineInitFromReusePool = YES;
  }
  [_lynxEngine attachTemplateRender:self];
}

- (void)setUpVariableWithBuilder:(LynxViewBuilder*)builder
                   containerView:(UIView<LUIBodyView>*)containerView
                      screenSize:(CGSize)screenSize {
  _enableGenericResourceFetcher =
      [self checkEnableGenericResourceFetcher:builder.enableGenericResourceFetcher];
  [builder.lynxUIRenderer setEnableGenericResourceFetcher:_enableGenericResourceFetcher];
  _originLynxViewConfig = builder.lynxViewConfig;
  _enableAirStrictMode = builder.enableAirStrictMode;
  // enable js default yes
  _enableJSRuntime = _enableAirStrictMode ? NO : builder.enableJSRuntime;
  _needPendingUIOperation = builder.enableUIOperationQueue;
  _lynxEngineProxy = [[LynxEngineProxy alloc] init];
  _enablePendingJSTaskOnLayout = builder.enablePendingJSTaskOnLayout;
  _enableMultiAsyncThread = builder.enableMultiAsyncThread;
  _enableVSyncAlignedMessageLoop = builder.enableVSyncAlignedMessageLoop;
  _enableAsyncHydration = builder.enableAsyncHydration;
  _enableJSGroupThread = builder.group.enableJSGroupThread;
  _enableUnifiedPipeline = builder.enableUnifiedPipeline;
  // First prepare env
  if (!builder.enableAsyncCreateRender) {
    [self setUpEnvWidthScreenSize:screenSize];
  }

  _enableAsyncDisplayFromNative = YES;
  _enableTextNonContiguousLayout = [builder enableTextNonContiguousLayout];
  _enableLayoutOnly = [LynxEnv.sharedInstance getEnableLayoutOnly];
  _embeddedMode = [builder getEmbeddedMode];
  _templateBundle = [builder lynxTemplateBundleForEngineReused];
  builder.config = builder.config ?: [LynxEnv sharedInstance].config;
  builder.config = builder.config ?: [[LynxConfig alloc] initWithProvider:nil];
  _config = builder.config;
  _fetcher = builder.fetcher;
  _hasStartedLoad = NO;
  _fontScale = builder.fontScale;

  _threadStrategyForRendering = builder.getThreadStrategyForRender;
  _enableLayoutSafepoint = builder.enableLayoutSafepoint;
  _enableAutoExpose = builder.enableAutoExpose;
  _enablePreUpdateData = builder.enablePreUpdateData;
  _lynxModuleExtraData = builder.lynxModuleExtraData;
  _lynxUIRenderer = builder.lynxUIRenderer;

  [self setUpContainerView:containerView builder:builder];

  _enableReuseEngine = ((_embeddedMode & LynxEmbeddedModeEnginePool) != 0 &&
                        builder.lynxTemplateBundleForEngineReused != nil);
  if (_enableReuseEngine) {
    [self reuseLynxEngine];
  }
}

- (void)setUpContainerView:(UIView<LUIBodyView>*)containerView builder:(LynxViewBuilder*)builder {
  if (containerView != nil) {
    _containerView = containerView;
    containerView.clipsToBounds = YES;
    if ([containerView isKindOfClass:[LynxView class]]) {
      LynxView* lynxView = (LynxView*)containerView;
      // TODO(zhoupeng.z): support for UIBodyView
      _delegate = (id<LynxTemplateRenderDelegate>)lynxView;
      [lynxView setEnableTextNonContiguousLayout:[builder enableTextNonContiguousLayout]];
      [lynxView setEnableLayoutOnly:[LynxEnv.sharedInstance getEnableLayoutOnly]];
      [lynxView setEnableSyncFlush:[builder enableSyncFlush]];
    }
  }
}

- (void)setUpEnvWidthScreenSize:(CGSize)screenSize {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_RENDER_SETUP_SCREEN_SIZE);
  [[LynxEnv sharedInstance] initLayoutConfig:screenSize];
}

- (LynxView*)getLynxView {
  return [_containerView isKindOfClass:[LynxView class]] ? (LynxView*)_containerView : nil;
}

- (void)setUpDevTool:(BOOL)debuggable {
  LynxView* lynxView = [self getLynxView];
  if (!lynxView) {
    return;
  }
  // TODO(zhoupeng.z): devtool should accept UIBodyView
  if (LynxEnv.sharedInstance.lynxDebugEnabled) {
    if (_runtime) {
      _devTool = _runtime.devtool;
      [_devTool attachLynxView:lynxView];
    } else {
      _devTool = [[LynxDevtool alloc] initWithLynxView:lynxView debuggable:debuggable];
    }
    __weak LynxTemplateRender* weakSelf = self;
    [_devTool setDispatchMessageEventBlock:^(NSDictionary* _Nonnull event) {
      dispatch_async(dispatch_get_main_queue(), ^{
        LynxTemplateRender* strongSelf = weakSelf;
        if (strongSelf == nil) {
          return;
        }
        strongSelf->shell_->DispatchMessageEvent(
            lynx::tasm::darwin::EventConverterDarwin::ConvertNSDictionaryToMessageEvent(event));
      });
    }];
  }
}

- (lynx::piper::ModuleFactoryDarwin*)getModuleFactory {
  auto manager = module_manager_.lock();
  if (manager) {
    return static_cast<lynx::piper::ModuleFactoryDarwin*>(manager->GetPlatformModuleFactory());
  }
  return nullptr;
}

- (void)setUpFrame:(CGRect)frame {
  // update viewport when preset width and height
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_RENDER_SETUP_FRAME);

  // If the engine is from the pool, there's no need to update the viewport again.
  if (!_isEngineInitFromReusePool) {
    [self updateFrame:frame];
  }
  _frameOfLynxView = frame;
  if (_containerView && !CGRectEqualToRect(_containerView.frame, _frameOfLynxView) &&
      !CGRectEqualToRect(CGRectZero, _frameOfLynxView)) {
    _containerView.frame = _frameOfLynxView;
  }
}

- (void)updateFrame:(CGRect)frame {
  if ((!CGRectEqualToRect(frame, CGRectZero) && !CGSizeEqualToSize(frame.size, CGSizeZero))) {
    _layoutWidthMode = LynxViewSizeModeExact;
    _layoutHeightMode = LynxViewSizeModeExact;
    _preferredLayoutWidth = frame.size.width;
    _preferredLayoutHeight = frame.size.height;
    [self updateViewport];
  }
}

- (void)setUpTiming {
  [self setTiming:_initStartTiming key:kTimingCreateLynxStart pipelineID:nil];
  [self setTiming:_initEndTiming key:kTimingCreateLynxEnd pipelineID:nil];
}

#pragma mark - Clean & Reuse

- (void)reset {
  if (_delegate) {
    __weak LynxTemplateRender* weakSelf = self;
    [LynxTemplateRender runOnMainThread:^() {
      __strong LynxTemplateRender* strongSelf = weakSelf;
      if (strongSelf) {
        [strongSelf->_delegate templateRenderOnResetViewAndLayer:strongSelf];
      }
    }];
    _hasRendered = NO;
  }

  _lynxSSRHelper = nil;

  _globalProps = [_globalProps deepClone];

  if (_lynxEngine == nil) {
    [_lynxUIRenderer reset];
    [_shadowNodeOwner destroySelf];
  } else if ([_lynxEngine isRunOnCurrentTemplateRender:self]) {
    [_lynxUIRenderer reset];
    [_shadowNodeOwner destroySelf];
    [self destroyLynxEngine];
  }

  shell_->ClearPipelineTimingInfo();
  // remove generic info
  [LynxEventReporter removeGenericInfo:_context.instanceId];
  int32_t lastInstanceId = _context.instanceId;
  _context.instanceId = kUnknownInstanceId;
  shell_->Destroy();

  if ([_delegate respondsToSelector:@selector(templateRenderOnTransitionUnregister:)]) {
    [_delegate templateRenderOnTransitionUnregister:self];
  }

  [self reset:lastInstanceId];
  // Update info
  [self updateNativeTheme];
  [self updateNativeGlobalProps];

  [self updateViewport];
  [self setUpTiming];
}

- (void)detachLynxEngine {
  _lynxEngine = nil;
}

- (void)destroyLynxEngine {
  if (_lynxEngine) {
    [_lynxEngine destroy];
    _lynxEngine = nil;
  }
}

// TODO(huangweiwu): maybe we need remove this method..
- (void)clearForDestroy {
  [_lynxUIRenderer reset];
  [LynxEventReporter clearCacheForInstanceId:_context.instanceId];
  _context.instanceId = kUnknownInstanceId;
  shell_->Destroy();
}

- (void)dealloc {
  if (_lynxEngine == nil) {
    [_lynxUIRenderer reset];
    [_shadowNodeOwner destroySelf];
  } else {
    [_lynxUIRenderer reset];
    [_shadowNodeOwner destroySelf];
    [self destroyLynxEngine];
  }

  pageConfig_.reset();
  // ios block cannot capture std::unique_ptr, tricky...
  auto* shell = shell_.release();
  // LynxView maybe release in main flow of TemplateAssembler,
  // so need just release LynxShell delay, avoid crash
  dispatch_async(dispatch_get_main_queue(), ^{
    delete shell;
  });
}

#pragma mark - Runtime

- (void)startLynxRuntime {
  _enablePendingJSTaskOnLayout = NO;
  if (shell_ != nullptr) {
    shell_->StartJsRuntime();
  }
}

#pragma mark - Load Template

- (void)loadTemplate:(nonnull LynxLoadMeta*)meta {
  if (meta.loadMode & LynxLoadModePrePainting) {
    _enablePrePainting = YES;
  }
  if (meta.loadOption & LynxLoadOptionDumpElement) {
    _enableDumpElement = YES;
  }
  if (meta.loadOption & LynxLoadOptionRecycleTemplateBundle) {
    _enableRecycleTemplateBundle = YES;
  }
  if (meta.loadOption & LynxLoadOptionProcessLayoutWithoutUIFlush) {
    [self setNeedPendingUIOperation:YES];
  }
  BOOL isTemplateBundleValid = meta.templateBundle != nil && meta.templateBundle.errorMsg == nil;
  BOOL isElementBundleValid = isTemplateBundleValid && [meta.templateBundle isElementBundleValid];

  // Inject platform page configs
  if (meta.lynxViewConfig) {
    [self setPlatformConfig:[meta.lynxViewConfig objectForKey:KEY_LYNX_PLATFORM_CONFIG]];
  } else if (_originLynxViewConfig) {
    [self setPlatformConfig:[_originLynxViewConfig objectForKey:KEY_LYNX_PLATFORM_CONFIG]];
  }

  if (meta.globalProps) {
    [self updateGlobalPropsWithTemplateData:meta.globalProps];
  }

  if (meta.loadMode == LynxLoadModeRenderSSR) {
    [self loadSSRDataWithMeta:meta];
    return;
  }

  if (_lynxSSRHelper && meta.loadMode == LynxLoadModeHydrateSSR) {
    [_lynxSSRHelper onHydrateStart];
  }

  if (meta.templateBundle) {
    [self loadTemplateBundle:meta.templateBundle withURL:meta.url initData:meta.initialData];
  } else if (meta.binaryData) {
    [self loadTemplate:meta.binaryData withURL:meta.url initData:meta.initialData];
  } else if (meta.url) {
    [self loadTemplateFromURL:meta.url initData:meta.initialData];
  }

  LOGI("loadTemplate preload:" << _enablePrePainting << " ,templateBundle:" << isTemplateBundleValid
                               << " ,isElementBundleValid:" << isElementBundleValid
                               << " ,enableDumpElement" << _enableDumpElement
                               << " ,enableRecycleTemplateBundle:" << _enableRecycleTemplateBundle
                               << " ,url:" << meta.url);
}

- (void)loadTemplate:(NSData*)tem withURL:(NSString*)url initData:(LynxTemplateData*)data {
  if (nil == url) {
    _LogE(@"LynxTemplateRender loadTemplate with data but url is empty! in render %p", self);
    return;
  }

  [_devTool onLoadFromLocalFile:tem withURL:url initData:data];

  [self updateUrl:url];
  [self dispatchViewDidStartLoading];
  // TODO(zhoumingsong.smile) move attachToDebugBridge to dispatchViewDidStartLoading
  // Due to lynxDevTool UI session limitations, we cannot do this yet
  [self->_devTool attachDebugBridge:url];
  [self internalLoadTemplate:tem withUrl:url initData:data];
}

- (void)loadTemplateFromURL:(NSString*)url initData:(LynxTemplateData*)data {
  if (nil == url) {
    _LogE(@"LynxTemplateRender loadTemplateFromURL url is empty! in render %p", self);
    return;
  }
  // It is essential to execute onLoadFromURL before dispatchViewDidStartLoading
  // so that the necessary information is available when the dispatchViewDidStartLoading callback
  // occurs.
  [self onLoadFromURL:url initData:data];
  [self dispatchViewDidStartLoading];
  __weak LynxTemplateRender* weakSelf = self;

  _LogI(@"LynxTemplateRender loadTemplate url after process is %@", url);
  [weakSelf markTiming:[kTimingPrepareTemplateStart UTF8String] pipelineID:[@"" UTF8String]];
  if (_lynxUIRenderer.uiOwner.uiContext.templateResourceFetcher) {
    _LogI(@"loadTemplateFromURL with templateResourceFetcher.");
    LynxResourceRequest* request =
        [[LynxResourceRequest alloc] initWithUrl:url type:LynxResourceTypeTemplate];
    [_lynxUIRenderer.uiOwner.uiContext.templateResourceFetcher
        fetchTemplate:(LynxResourceRequest* _Nonnull)request
           onComplete:^(LynxTemplateResource* _Nullable templateRes, NSError* _Nullable error) {
             if (!error) {
               if (templateRes.bundle) {
                 [weakSelf.devTool onLoadFromBundle:templateRes.bundle withURL:url initData:data];
                 // TODO(zhoumingsong.smile) move attachToDebugBridge to dispatchViewDidStartLoading
                 // Due to lynxDevTool UI session limitations, we cannot do this yet
                 [self->_devTool attachDebugBridge:url];
                 [weakSelf loadTemplateBundle:templateRes.bundle withURL:url initData:data];
               } else if (templateRes.data) {
                 [weakSelf.devTool onTemplateLoadSuccess:templateRes.data];
                 // TODO(zhoumingsong.smile) move attachToDebugBridge to dispatchViewDidStartLoading
                 // Due to lynxDevTool UI session limitations, we cannot do this yet
                 [self->_devTool attachDebugBridge:url];
                 [weakSelf internalLoadTemplate:templateRes.data withUrl:url initData:data];
               }
             } else {
               [weakSelf onFetchTemplateError:error];
             }
           }];
  } else {
    _LogI(@"loadTemplateFromURL with legacy templateProvider.");
    LynxTemplateLoadBlock complete = ^(id tem, NSError* error) {
      [weakSelf markTiming:[kTimingPrepareTemplateEnd UTF8String] pipelineID:[@"" UTF8String]];
      if (!error) {
        if ([tem isKindOfClass:[NSData class]]) {
          [weakSelf.devTool onTemplateLoadSuccess:tem];
          [weakSelf internalLoadTemplate:tem withUrl:url initData:data];
        } else if ([tem isKindOfClass:[LynxTemplateBundle class]]) {
          [weakSelf.devTool onLoadFromBundle:tem withURL:url initData:data];
          [weakSelf loadTemplateBundle:tem withURL:url initData:data];
        }
      } else {
        [weakSelf onFetchTemplateError:error];
      }
    };
    [_config.templateProvider loadTemplateWithUrl:url onComplete:complete];
  }
}

- (void)loadTemplateBundle:(LynxTemplateBundle*)bundle
                   withURL:(NSString*)url
                  initData:(LynxTemplateData*)data {
  if (_enableReuseEngine && [_lynxEngine hasLoaded] &&
      [_lynxEngine isRunOnCurrentTemplateRender:self]) {
    // TODO(renzhongyue): attachUIBodyView
    [self updateDataWithTemplateData:data];
    [_lynxEngine registerToReuse];
    return;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_RENDER_LOAD_TEMPLATE_BUNDLE, "url", [url UTF8String]);
  auto pipeline_options = std::make_shared<lynx::tasm::PipelineOptions>();
  pipeline_options->pipeline_origin = lynx::tasm::timing::kLoadBundle;
  pipeline_options->need_timestamps = YES;
  [self onPipelineStart:pipeline_options->pipeline_id
              pipelineOrigin:pipeline_options->pipeline_origin
      pipelineStartTimestamp:pipeline_options->pipeline_start_timestamp];
  [self markTiming:lynx::tasm::timing::kLoadBundleStart
        pipelineID:pipeline_options->pipeline_id.c_str()];

  [self updateUrl:url];
  [self dispatchViewDidStartLoading];
  // TODO(zhoumingsong.smile) move attachToDebugBridge to dispatchViewDidStartLoading
  // Due to lynxDevTool UI session limitations, we cannot do this yet
  [self->_devTool attachDebugBridge:url];
  if ([bundle errorMsg]) {
    NSString* errorMsg =
        [NSString stringWithFormat:@"LynxTemplateRender loadTemplateBundle with an invalid "
                                   @"LynxTemplateBundle. error is: %p",
                                   [bundle errorMsg]];
    LynxError* error =
        [LynxError lynxErrorWithCode:ECLynxAppBundleLoadBadBundle
                             message:errorMsg
                       fixSuggestion:@"TemplateBundle init failed, Please check the error message."
                               level:LynxErrorLevelError];
    [self onErrorOccurred:error];
    return;
  }

  auto template_bundle = LynxGetRawTemplateBundle(bundle);
  if (template_bundle == nullptr) {
    NSString* errorMsg = @"LynxTemplateRender loadTemplateBundle with an empty LynxTemplateBundle.";
    LynxError* error =
        [LynxError lynxErrorWithCode:ECLynxAppBundleLoadRenderFailed
                             message:errorMsg
                       fixSuggestion:@"TemplateBundle init failed, Please check the error message."
                               level:LynxErrorLevelError];
    [self onErrorOccurred:error];
    return;
  }
  [_devTool onLoadFromBundle:bundle withURL:url initData:data];
  __weak LynxTemplateRender* weakSelf = self;
  [self
      executeNativeOpSafely:^() {
        [self prepareForLoadTemplateWithUrl:url initData:data];
        lynx::lepus::Value value;
        std::shared_ptr<lynx::tasm::TemplateData> ptr(nullptr);
        if (data != nil) {
          TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_RENDER_CREATE_TEMPLATE_DATA);
          value = *LynxGetLepusValueFromTemplateData(data);
          ptr = std::make_shared<lynx::tasm::TemplateData>(
              value, data.isReadOnly, data.processorName ? data.processorName.UTF8String : "");
          ptr->SetPlatformData(std::make_unique<lynx::tasm::PlatformDataDarwin>(data));
        }

        lynx::tasm::LynxTemplateBundle copied_bundle = *template_bundle;
        if (template_bundle->GetContainsElementTree()) {
          // The Element Bundle cannot be reused for rendering and is disposable. Therefore, a deep
          // copy is needed here to ensure that the internal rendering does not invalidate the
          // element bundle passed in by the business.
          copied_bundle.SetElementBundle(template_bundle->GetElementBundle().DeepClone());
        }
        [self markTiming:lynx::tasm::timing::kFfiStart
              pipelineID:pipeline_options->pipeline_id.c_str()];
        pipeline_options->enable_pre_painting = _enablePrePainting;
        pipeline_options->enable_dump_element_tree = _enableDumpElement;
        self->shell_->LoadTemplateBundle(lynx::base::SafeStringConvert([url UTF8String]),
                                         std::move(copied_bundle), pipeline_options, ptr);
        _hasStartedLoad = YES;
        [_lynxEngine registerToReuse];
      }
      withErrorCallback:^(NSString* msg, NSString* stack) {
        __strong LynxTemplateRender* strongSelf = weakSelf;
        [strongSelf reportError:ECLynxAppBundleLoadException withMsg:msg rawStack:stack];
      }];
}

- (void)internalLoadTemplate:(NSData*)tem withUrl:(NSString*)url initData:(LynxTemplateData*)data {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_RENDER_INTERNAL_LOAD_TEMPLATE, "url", [url UTF8String]);
  // TODO(zhoumingsong.smile) move attachToDebugBridge to dispatchViewDidStartLoading
  // Due to lynxDevTool UI session limitations, we cannot do this yet
  [self->_devTool attachDebugBridge:url];
  auto pipeline_options = std::make_shared<lynx::tasm::PipelineOptions>();
  pipeline_options->pipeline_origin = lynx::tasm::timing::kLoadBundle;
  pipeline_options->need_timestamps = YES;
  [self onPipelineStart:pipeline_options->pipeline_id
              pipelineOrigin:pipeline_options->pipeline_origin
      pipelineStartTimestamp:pipeline_options->pipeline_start_timestamp];
  [self markTiming:lynx::tasm::timing::kLoadBundleStart
        pipelineID:pipeline_options->pipeline_id.c_str()];

  pipeline_options->enable_pre_painting = _enablePrePainting;
  pipeline_options->enable_recycle_template_bundle = _enableRecycleTemplateBundle;
  pipeline_options->enable_dump_element_tree = _enableDumpElement;

  __weak LynxTemplateRender* weakSelf = self;
  [self
      executeNativeOpSafely:^() {
        [self prepareForLoadTemplateWithUrl:url initData:data];
        lynx::lepus::Value value;
        std::shared_ptr<lynx::tasm::TemplateData> ptr(nullptr);
        if (data != nil) {
          TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_RENDER_CREATE_TEMPLATE_DATA);
          value = *LynxGetLepusValueFromTemplateData(data);
          ptr = std::make_shared<lynx::tasm::TemplateData>(
              value, data.isReadOnly, data.processorName ? data.processorName.UTF8String : "");
          ptr->SetPlatformData(std::make_unique<lynx::tasm::PlatformDataDarwin>(data));
        }
        auto securityService = LynxService(LynxServiceSecurityProtocol);
        if (securityService == nil) {
          // if securityService is nil, Skip Security Check.
          [self markTiming:lynx::tasm::timing::kFfiStart
                pipelineID:pipeline_options->pipeline_id.c_str()];
          self->shell_->LoadTemplate([url UTF8String], ConvertNSBinary(tem), pipeline_options, ptr);
          _hasStartedLoad = YES;
        } else {
          [self markTiming:lynx::tasm::timing::kVerifyTasmStart
                pipelineID:pipeline_options->pipeline_id.c_str()];
          LynxVerificationResult* verification = [securityService verifyTASM:tem
                                                                        view:[self getLynxView]
                                                                         url:url
                                                                        type:LynxTASMTypeTemplate];
          [self markTiming:lynx::tasm::timing::kVerifyTasmEnd
                pipelineID:pipeline_options->pipeline_id.c_str()];
          if (verification.verified) {
            [self markTiming:lynx::tasm::timing::kFfiStart
                  pipelineID:pipeline_options->pipeline_id.c_str()];
            self->shell_->LoadTemplate([url UTF8String], ConvertNSBinary(tem), pipeline_options,
                                       ptr);
            _hasStartedLoad = YES;
          } else {
            [self reportError:ECLynxAppBundleVerifyInvalidSignature
                      withMsg:verification.errorMsg
                     rawStack:nil];
          }
        }
      }
      withErrorCallback:^(NSString* msg, NSString* stack) {
        __strong LynxTemplateRender* strongSelf = weakSelf;
        [strongSelf reportError:ECLynxAppBundleLoadException withMsg:msg rawStack:stack];
      }];
}

- (void)prepareForLoadTemplateWithUrl:(NSString*)url initData:(LynxTemplateData*)data {
  if (![NSThread isMainThread]) {
    LOGE("LoadTemplate on other thread:" << [NSThread currentThread] << ", url:" << url);
  }
  {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, SERVICE_REPORT_ERROR_GLOBAL_CONTEXT_TAG);
    NSString* finalSchema = [self formatLynxSchema:url];
    [LynxService(LynxServiceMonitorProtocol) reportErrorGlobalContextTag:LynxContextTagLastLynxURL
                                                                    data:finalSchema];
  }

  if (_lynxSSRHelper) {
    [self onLoadTemplateFromSSRPage];
  }

  if (_hasStartedLoad || self->shell_->IsDestroyed()) {
    [self reset];
  } else {
    [self updateViewport];
  }
  [self resetLayoutStatus];
  // Update the url info to generic info after the shell is rebuilt, because the rebuilt shell
  // generates a new instance ID.
  [self updateGenericInfoURL:url];
  TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY_VITALS, TEMPLATE_RENDER_START_LOAD);
}

- (void)onLoadFromURL:(NSString*)url initData:(LynxTemplateData*)data {
  NSDictionary* urls = [self processUrl:url];
  url = [urls objectForKey:@"compile_path"] ?: @"";
  NSString* postUrl = [urls objectForKey:@"post_url"] ?: @"";
  [_devTool onLoadFromURL:url initData:data postURL:postUrl];
  [self updateUrl:url];
}

- (void)updateUrl:(NSString*)url {
  _url = url;
  [self updateGenericInfoURL:url];
}

- (void)updateGenericInfoURL:(NSString*)url {
  if (!url) {
    return;
  }
  // Update template url to Generic Info.
  [LynxEventReporter updateGenericInfo:url key:kPropURL instanceId:_context.instanceId];
  NSString* relativePath = [LynxEventReporterUtils relativePathForURL:url];
  if (relativePath) {
    [LynxEventReporter updateGenericInfo:relativePath
                                     key:kPropRelativePath
                              instanceId:_context.instanceId];
  }
}

- (void)loadTemplateWithoutLynxView:(NSData*)tem
                            withURL:(NSString*)url
                           initData:(LynxTemplateData*)data {
  [self detachLynxView];
  _hasRendered = NO;
  [self loadTemplate:tem withURL:url initData:data];
}

#pragma mark - Update data

- (void)updateMetaData:(LynxUpdateMeta*)meta {
  if (meta.data) {
    [_devTool onUpdateDataWithTemplateData:meta.data];
  }
  if (meta.data || meta.globalProps) {
    __weak LynxTemplateRender* weakSelf = self;
    [self
        executeNativeOpSafely:^() {
          lynx::lepus::Value updated_global_props{};
          if (meta.globalProps) {
            // globalProps in merged in platform.
            // if passing globalProps is nil means globalProps should not be updated, pass Nil
            // downwards. if passing globalProps is not nil means globalProps should be updated,
            // pass merged globalProps downwards.
            if (_globalProps == nil) {
              _globalProps = meta.globalProps;
            } else {
              [_globalProps updateWithTemplateData:meta.globalProps];
            }
            updated_global_props = *LynxGetLepusValueFromTemplateData(_globalProps);
            [_devTool onGlobalPropsUpdated:_globalProps];
          }

          std::shared_ptr<lynx::tasm::TemplateData> updated_data = nullptr;
          if (meta.data) {
            lynx::lepus::Value value = *LynxGetLepusValueFromTemplateData(meta.data);
            updated_data = std::make_shared<lynx::tasm::TemplateData>(
                value, meta.data.isReadOnly,
                meta.data.processorName ? meta.data.processorName.UTF8String : "");
            updated_data->SetPlatformData(
                std::make_unique<lynx::tasm::PlatformDataDarwin>(meta.data));
          }
          [self resetLayoutStatus];
          [self markDirty];
          self->shell_->UpdateMetaData(updated_data, updated_global_props);
        }
        withErrorCallback:^(NSString* msg, NSString* stack) {
          __strong LynxTemplateRender* strongSelf = weakSelf;
          [strongSelf reportError:ECLynxDataFlowUpdateException withMsg:msg rawStack:stack];
        }];
  }
}

- (void)updateDataWithString:(NSString*)data processorName:(NSString*)name {
  if (data) {
    [self requestLayoutWhenSafepointEnable];
    LynxTemplateData* templateData = [[LynxTemplateData alloc] initWithJson:data];
    [templateData markState:name];
    [templateData markReadOnly];
    [self updateDataWithTemplateData:templateData];
  }
}

- (void)updateDataWithDictionary:(NSDictionary<NSString*, id>*)data processorName:(NSString*)name {
  if (data.count > 0) {
    [self requestLayoutWhenSafepointEnable];
    LynxTemplateData* templateData = [[LynxTemplateData alloc] initWithDictionary:data];
    [templateData markState:name];
    [templateData markReadOnly];
    [self updateDataWithTemplateData:templateData];
  }
}

- (void)updateDataWithTemplateData:(LynxTemplateData*)data {
  if (data) {
    [_devTool onUpdateDataWithTemplateData:data];
    [self executeUpdateDataSafely:^() {
      lynx::lepus::Value value = *LynxGetLepusValueFromTemplateData(data);
      std::shared_ptr<lynx::tasm::TemplateData> ptr = std::make_shared<lynx::tasm::TemplateData>(
          value, data.isReadOnly, data.processorName ? data.processorName.UTF8String : "");
      ptr->SetPlatformData(std::make_unique<lynx::tasm::PlatformDataDarwin>(data));
      [self resetLayoutStatus];
      [self markDirty];
      self->shell_->UpdateDataByParsedData(ptr);
    }];
  }
}

#pragma mark - Reset Data

- (void)resetDataWithTemplateData:(LynxTemplateData*)data {
  if (data) {
    [_devTool onResetDataWithTemplateData:data];
    [self executeUpdateDataSafely:^() {
      lynx::lepus::Value value = *LynxGetLepusValueFromTemplateData(data);
      std::shared_ptr<lynx::tasm::TemplateData> ptr = std::make_shared<lynx::tasm::TemplateData>(
          value, data.isReadOnly, data.processorName ? data.processorName.UTF8String : "");
      ptr->SetPlatformData(std::make_unique<lynx::tasm::PlatformDataDarwin>(data));
      [self resetLayoutStatus];
      [self markDirty];
      self->shell_->ResetDataByParsedData(ptr);
    }];
  }
}

#pragma mark - Reload

- (void)reloadTemplateWithTemplateData:(nullable LynxTemplateData*)data
                           globalProps:(nullable LynxTemplateData*)globalProps {
  if (data) {
    [self resetTimingBeforeReload];
    auto pipeline_options = std::make_shared<lynx::tasm::PipelineOptions>();
    pipeline_options->pipeline_origin = lynx::tasm::timing::kReloadBundleFromNative;
    pipeline_options->need_timestamps = YES;
    pipeline_options->is_reload_template = YES;
    [self onPipelineStart:pipeline_options->pipeline_id
                pipelineOrigin:pipeline_options->pipeline_origin
        pipelineStartTimestamp:pipeline_options->pipeline_start_timestamp];
    [self markTiming:lynx::tasm::timing::kReloadBundleStart
          pipelineID:pipeline_options->pipeline_id.c_str()];

    if ([_delegate respondsToSelector:@selector(templateRenderOnPageStarted:withPipelineInfo:)]) {
      LynxPipelineInfo* pipelineInfo = [[LynxPipelineInfo alloc] initWithUrl:[self url]];
      [pipelineInfo addPipelineOrigin:LynxReload];
      [_delegate templateRenderOnPageStarted:self withPipelineInfo:pipelineInfo];
    }

    [self executeUpdateDataSafely:^() {
      auto template_data = ConvertLynxTemplateDataToTemplateData(data);
      template_data->SetPlatformData(std::make_unique<lynx::tasm::PlatformDataDarwin>(data));
      [self resetLayoutStatus];
      [self markDirty];

      /**
       * Null globalProps -> Nil Value;
       * Empty globalProps -> Table Value;
       */
      [self markTiming:lynx::tasm::timing::kFfiStart
            pipelineID:pipeline_options->pipeline_id.c_str()];
      if (globalProps == nil) {
        self->shell_->ReloadTemplate(template_data, pipeline_options);
      } else {
        self->_globalProps = globalProps;
        auto props_value = LynxGetLepusValueFromTemplateData(globalProps);
        self->shell_->ReloadTemplate(
            template_data, pipeline_options,
            props_value ? *props_value : lynx::lepus::Value(lynx::lepus::Dictionary::Create()));
      }
    }];
  }
}

#pragma mark - SSR

- (void)loadSSRDataWithMeta:(LynxLoadMeta*)meta {
  if (meta.binaryData) {
    [self loadSSRData:meta.binaryData withURL:meta.url initData:meta.initialData];
  } else if (meta.url) {
    [self loadSSRDataFromURL:meta.url initData:meta.initialData];
  } else {
    _LogE(@"SSR rendering failed: Binary data is invalid or URL is empty.");
  }
}

- (void)loadSSRData:(NSData*)tem
            withURL:(NSString*)url
           initData:(nullable LynxTemplateData*)initData {
  [_devTool onLoadFromLocalFile:tem withURL:url initData:initData];

  [self updateUrl:url];
  __weak LynxTemplateRender* weakSelf = self;
  [self
      executeNativeOpSafely:^() {
        if (![NSThread isMainThread]) {
          NSString* stack = [[[NSThread callStackSymbols] valueForKey:@"description"]
              componentsJoinedByString:@"\n"];
          _LogE(@"LoadSSRData on other thread:%@, stack:%@", [NSThread currentThread].name, stack);
        }
        __strong LynxTemplateRender* strongSelf = weakSelf;
        if (!strongSelf) {
          return;
        }
        [strongSelf markDirty];
        if (_hasStartedLoad || strongSelf->shell_->IsDestroyed()) {
          [strongSelf reset];
        } else {
          [strongSelf updateViewport];
        }

        // wating for hydarte
        _hasStartedLoad = YES;
        _lynxSSRHelper = [[LynxSSRHelper alloc] init];
        [_lynxSSRHelper onLoadSSRDataStart];
        auto data = ConvertNSBinary(tem);
        std::shared_ptr<lynx::tasm::TemplateData> ptr(nullptr);
        lynx::lepus::Value value;
        if (initData != nil) {
          value = *LynxGetLepusValueFromTemplateData(initData);
          ptr = std::make_shared<lynx::tasm::TemplateData>(
              value, initData.isReadOnly,
              initData.processorName ? initData.processorName.UTF8String : "");
          ptr->SetPlatformData(std::make_unique<lynx::tasm::PlatformDataDarwin>(initData));
        }

        std::string urlStr = url ? std::string([url UTF8String]) : std::string();
        strongSelf->shell_->SetSSRTimingData(std::move(urlStr), tem.length);
        [LynxEventReporter updateGenericInfo:@(YES)
                                         key:kPropEnableSSR
                                  instanceId:[self instanceId]];
        [strongSelf->_devTool attachDebugBridge:url];
        strongSelf->shell_->LoadSSRData(std::move(data), ptr);
      }
      withErrorCallback:^(NSString* msg, NSString* stack) {
        __strong LynxTemplateRender* strongSelf = weakSelf;
        [strongSelf reportError:ECLynxSSRDecode withMsg:msg rawStack:stack];
      }];
}

- (void)loadSSRDataFromURL:(NSString*)url initData:(nullable LynxTemplateData*)data {
  if (nil == url) {
    _LogE(@"LynxTemplateRender loadSSRDataFromURL url is empty! in render %p", self);
    return;
  }

  __weak LynxTemplateRender* weakSelf = self;
  _LogI(@"loadSSRDataFromURL load-ssr-data url after process is %@", url);
  if (_lynxUIRenderer.uiOwner.uiContext.templateResourceFetcher) {
    _LogI(@"loadSSRDataFromURL with templateResourceFetcher.");
    LynxResourceRequest* request =
        [[LynxResourceRequest alloc] initWithUrl:url type:LynxResourceTypeTemplate];
    [_lynxUIRenderer.uiOwner.uiContext.templateResourceFetcher
        fetchSSRData:request
          onComplete:^(NSData* _Nullable tem, NSError* _Nullable error) {
            if (!error) {
              [weakSelf loadSSRData:tem withURL:url initData:data];
            } else {
              [weakSelf onFetchTemplateError:error];
            }
          }];
  } else {
    _LogI(@"loadSSRDataFromURL with legacy templateProvider.");
    LynxTemplateLoadBlock complete = ^(id tem, NSError* error) {
      if (!error) {
        if ([tem isKindOfClass:[NSData class]]) {
          [weakSelf loadSSRData:tem withURL:url initData:data];
        }
      } else {
        [weakSelf onFetchTemplateError:error];
      }
    };
    [_config.templateProvider loadTemplateWithUrl:url onComplete:complete];
  }
}

- (void)onLoadTemplateFromSSRPage {
  if (_lynxSSRHelper && [_lynxSSRHelper isHydrateStarted]) {
    _hasStartedLoad = NO;
    [_lynxSSRHelper onHydrateExecuting];
  }
}

- (void)ssrHydrate:(nonnull NSData*)tem
           withURL:(nonnull NSString*)url
          initData:(nullable LynxTemplateData*)data {
  if (_lynxSSRHelper) {
    [_lynxSSRHelper onHydrateStart];
  }

  [self loadTemplate:tem withURL:url initData:data];
}

- (void)ssrHydrateFromURL:(NSString*)url initData:(nullable LynxTemplateData*)data {
  if (_lynxSSRHelper) {
    [_lynxSSRHelper onHydrateStart];
  }

  [self loadTemplateFromURL:url initData:data];
}

#pragma mark - Global Props

- (void)updateGlobalPropsWithDictionary:(NSDictionary<NSString*, id>*)data {
  LYNX_TRACE_SECTION_WITH_INFO(LYNX_TRACE_CATEGORY_WRAPPER, TEMPLATE_RENDER_UPDATE_GLOBAL_PROPS,
                               data);
  if (data.count > 0) {
    [self updateGlobalPropsWithTemplateData:[[LynxTemplateData alloc] initWithDictionary:data]];
  }
  LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER);
}

- (void)updateGlobalPropsWithTemplateData:(LynxTemplateData*)data {
  LYNX_TRACE_SECTION(LYNX_TRACE_CATEGORY_WRAPPER, TEMPLATE_RENDER_UPDATE_GLOBAL_PROPS);
  if (data) {
    if (!_globalProps) {
      _globalProps = [[LynxTemplateData alloc] initWithDictionary:[NSDictionary new]];
    }
    [_globalProps updateWithTemplateData:data];
    [self updateNativeGlobalProps];
  }
  LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER);
}

- (void)updateNativeGlobalProps {
  if (shell_ == nil || _globalProps == nil) {
    return;
  }
  [_devTool onGlobalPropsUpdated:_globalProps];
  __weak LynxTemplateRender* weakSelf = self;
  [self
      executeNativeOpSafely:^() {
        self->shell_->UpdateGlobalProps(*LynxGetLepusValueFromTemplateData(_globalProps));
      }
      withErrorCallback:^(NSString* msg, NSString* stack) {
        __strong LynxTemplateRender* strongSelf = weakSelf;
        [strongSelf reportError:ECLynxDataFlowUpdateException withMsg:msg rawStack:stack];
      }];
}

#pragma mark - Storage

- (void)setSessionStorageItem:(NSString*)key WithTemplateData:(LynxTemplateData*)data {
  if (data) {
    [self
        executeNativeOpSafely:^() {
          lynx::lepus::Value value = *LynxGetLepusValueFromTemplateData(data);
          self->shell_->SetSessionStorageItem(
              [key UTF8String],
              std::make_shared<lynx::tasm::TemplateData>(value, data.isReadOnly, ""));
        }
            withErrorCallback:nil];
  }
}

- (void)getSessionStorageItem:(NSString*)key
                 withCallback:(void (^)(id<NSObject> _Nullable))callback {
  [self
      executeNativeOpSafely:^() {
        self->shell_->GetSessionStorageItem(
            [key UTF8String],
            std::make_unique<lynx::shell::PlatformCallBack>([callback](const lepus_value& value) {
              callback(lynx::tasm::convertLepusValueToNSObject(value));
            }));
      }
          withErrorCallback:nil];
}

- (double)subscribeSessionStorage:(NSString*)key
                     withCallback:(void (^)(id<NSObject> _Nullable))callback {
  return self->shell_->SubscribeSessionStorage(
      [key UTF8String],
      std::make_unique<lynx::shell::PlatformCallBack>([callback](const lepus_value& value) {
        callback(lynx::tasm::convertLepusValueToNSObject(value));
      }));
}

- (void)unSubscribeSessionStorage:(NSString*)key withId:(double)callbackId {
  [self
      executeNativeOpSafely:^() {
        self->shell_->UnSubscribeSessionStorage([key UTF8String], callbackId);
      }
          withErrorCallback:nil];
}

#pragma mark - Event

- (id<LynxEventTarget>)hitTestInEventHandler:(CGPoint)point withEvent:(UIEvent*)event {
  return [_lynxUIRenderer hitTestInEventHandler:point withEvent:event];
};

- (void)willMoveToWindow:(UIWindow*)newWindow {
  [_lynxUIRenderer willMoveToWindow:newWindow];
  if (newWindow != nil) {
    [self onEnterForeground:false];
  } else {
    [self onEnterBackground:false];
  }
}

- (void)sendGlobalEvent:(nonnull NSString*)name withParams:(nullable NSArray*)params {
  // When SSR hydrate status is pending beginning or failed, a global event will be sent to SSR
  // runtime to be consumed. But this global event will also be cached so that when runtimeReady it
  // behaves as normal global event.
  NSArray* finalParams = params;
  if ([_lynxSSRHelper shouldSendEventToSSR]) {
    // Send global event to SSR
    lynx::lepus::Value value = LynxConvertToLepusValue(params);
    self->shell_->SendSsrGlobalEvent([name UTF8String], value);

    // process params
    finalParams = [LynxSSRHelper processEventParams:params];
  }

  if (_context != nil) {
    [_context sendGlobalEvent:name withParams:finalParams];
  } else {
    _LogE(@"TemplateRender %p sendGlobalEvent %@ error, can't get LynxContext", self, name);
  }
}

- (void)sendGlobalEventToLepus:(nonnull NSString*)name withParams:(nullable NSArray*)params {
  lynx::lepus::Value value = LynxConvertToLepusValue(params);
  self->shell_->SendGlobalEventToLepus([name UTF8String], value);
}

- (void)triggerEventBus:(nonnull NSString*)name withParams:(nullable NSArray*)params {
  lynx::lepus::Value value = LynxConvertToLepusValue(params);
  self->shell_->TriggerEventBus([name UTF8String], value);
}

- (void)onEnterForeground {
  [self onEnterForeground:true];
}

- (void)onEnterBackground {
  [self onEnterBackground:true];
}

- (void)onLongPress {
  [_devTool handleLongPress];
}

// when called onEnterForeground/onEnterBackground
// directly by LynxView, force onShow/onHide,
// else by willMoveToWindow, need check autoExpose or not
- (void)onEnterForeground:(bool)forceChangeStatus {
  LOGI("onEnterForeground. force: " << forceChangeStatus);
  if (shell_ != nullptr && (forceChangeStatus || [self getAutoExpose])) {
    shell_->OnEnterForeground();
  }
  [_lynxUIRenderer onEnterForeground];
  [_devTool onEnterForeground];
}

- (void)onEnterBackground:(bool)forceChangeStatus {
  LOGI("onEnterBackground. force: " << forceChangeStatus);
  if (shell_ != nullptr && (forceChangeStatus || [self getAutoExpose])) {
    shell_->OnEnterBackground();
  }
  [_lynxUIRenderer onEnterBackground];
  [_devTool onEnterBackground];
}

- (BOOL)getAutoExpose {
  return _enableAutoExpose && (!pageConfig_ || pageConfig_->GetAutoExpose());
}

- (void)setAttachLynxPageUICallback:(attachLynxPageUI)callback {
  [_lynxUIRenderer.uiOwner setAttachLynxPageUICallback:callback];
}

#pragma mark - Life Cycle

- (void)dispatchViewDidStartLoading {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_RENDER_DID_START_LOADING);
  if (_delegate) {
    [_delegate templateRenderOnTemplateStartLoading:self];
  }
  [LynxEventReporter onEvent:@"lynxsdk_open_page" instanceId:[self instanceId] props:nil];
}

- (void)dispatchError:(LynxError*)error {
  if (_delegate) {
    [_delegate templateRender:self onErrorOccurred:error];
  }
}

- (void)dispatchDidFirstScreen {
  [_delegate templateRenderOnFirstScreen:self];
}

- (void)dispatchDidPageUpdate {
  [_delegate templateRenderOnPageUpdate:self];
  [_devTool onPageUpdate];
}

#pragma mark - View

- (void)setCustomizedLayoutInUIContext:(id<LynxListLayoutProtocol> _Nullable)customizedListLayout {
  [_lynxUIRenderer setCustomizedLayoutInUIContext:customizedListLayout];
}

- (void)setScrollListener:(id<LynxScrollListener>)scrollListener {
  [_lynxUIRenderer setScrollListener:scrollListener];
}

- (void)setImageFetcherInUIOwner:(id<LynxImageFetcher>)imageFetcher {
  [_lynxUIRenderer setImageFetcherInUIOwner:imageFetcher];
}

- (void)setResourceFetcher:(id<LynxResourceFetcher>)resourceFetcher {
  _resourceFetcher = resourceFetcher;
  [_lynxUIRenderer setResourceFetcherInUIOwner:resourceFetcher];
}

- (void)setResourceFetcherInUIOwner:(id<LynxResourceFetcher>)resourceFetcher {
  [_lynxUIRenderer setResourceFetcherInUIOwner:resourceFetcher];
}

- (void)triggerLayout {
  [self updateViewport];
  if (_uilayoutTick) {
    __weak LynxTemplateRender* weakSelf = self;
    [self
        executeNativeOpSafely:^{
          [self->_uilayoutTick triggerLayout];
        }
        withErrorCallback:^(NSString* msg, NSString* stack) {
          __strong LynxTemplateRender* strongSelf = weakSelf;
          [strongSelf reportError:ECLynxLayoutInternal withMsg:msg rawStack:stack];
        }];
  }
}

- (void)triggerLayoutInTick {
  if (_uilayoutTick) {
    [_uilayoutTick triggerLayout];
  }
}

- (void)updateViewport {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_RENDER_UPDATE_VIEWPORT);
  [self updateViewport:true];
}

- (void)updateViewport:(BOOL)needLayout {
  if (shell_->IsDestroyed()) {
    return;
  }
  SLMeasureMode heightMode = (SLMeasureMode)_layoutHeightMode;
  SLMeasureMode widthMode = (SLMeasureMode)_layoutWidthMode;
  CGFloat width =
      _layoutWidthMode == LynxViewSizeModeMax ? _preferredMaxLayoutWidth : _preferredLayoutWidth;
  CGFloat height =
      _layoutHeightMode == LynxViewSizeModeMax ? _preferredMaxLayoutHeight : _preferredLayoutHeight;
  shell_->UpdateViewport(width, widthMode, height, heightMode, needLayout);
}

- (void)updateScreenMetricsWithWidth:(CGFloat)width height:(CGFloat)height {
  if (shell_->IsDestroyed()) {
    return;
  }

  CGFloat scale = [UIScreen mainScreen].scale;
  shell_->UpdateScreenMetrics(width, height, scale);

  [_lynxUIRenderer updateScreenWidth:width height:height];
}

- (void)updateFontScale:(CGFloat)scale {
  if (shell_->IsDestroyed()) {
    return;
  }
  shell_->UpdateFontScale(scale);
}

- (void)pauseRootLayoutAnimation {
  [_lynxUIRenderer pauseRootLayoutAnimation];
}

- (void)resumeRootLayoutAnimation {
  [_lynxUIRenderer resumeRootLayoutAnimation];
}

- (void)restartAnimation {
  [_lynxUIRenderer restartAnimation];
}

- (void)resetAnimation {
  [_lynxUIRenderer resetAnimation];
}

- (void)setTheme:(LynxTheme*)theme {
  if (theme == nil) {
    return;
  }

  [self setLocalTheme:theme];
  [self markDirty];
  [self requestLayoutWhenSafepointEnable];
  [self updateNativeTheme];
}

- (nullable LynxTheme*)theme {
  return _localTheme;
}

- (void)setLocalTheme:(LynxTheme*)theme {
  if (_localTheme == nil) {
    _localTheme = theme;
  } else {
    [_localTheme setThemeConfig:[theme themeConfig]];
  }
}

- (void)updateNativeTheme {
  if (shell_ == nullptr || _localTheme == nil) {
    return;
  }
  auto themeDict = lynx::lepus::Dictionary::Create();
  auto keys = [_localTheme allKeys];
  for (NSString* key in keys) {
    NSString* val = [_localTheme valueForKey:key];
    themeDict->SetValue([key UTF8String], [val UTF8String]);
  }

  lynx::fml::RefPtr<lynx::lepus::Dictionary> dict = lynx::lepus::Dictionary::Create();
  dict->SetValue(BASE_STATIC_STRING(lynx::tasm::CARD_CONFIG_THEME), std::move(themeDict));

  shell_->UpdateConfig(lynx::lepus::Value(std::move(dict)));
}

- (void)resetLayoutStatus {
  [_paintingContextProxy resetLayoutStatus];
}

- (void)markDirty {
  shell_->MarkDirty();
}

- (void)requestLayoutWhenSafepointEnable {
  if (_enableLayoutSafepoint &&
      (_threadStrategyForRendering == LynxThreadStrategyForRenderPartOnLayout ||
       _needPendingUIOperation) &&
      _delegate != nil) {
    // trigger layout
    if ([_delegate respondsToSelector:@selector(templateRenderRequestNeedsLayout:)]) {
      [_delegate templateRenderRequestNeedsLayout:self];
    }
  }
}

- (void)detachLynxView {
  [_delegate templateRenderOnDetach:self];
  if (_containerView) {
    _containerView = nil;
    _delegate = nil;
  }
}

#pragma mark - Find Node

- (LynxUI*)findUIBySign:(NSInteger)sign {
  return [_lynxUIRenderer findUIBySign:sign];
}

- (nullable UIView*)findViewWithName:(nonnull NSString*)name {
  return [_lynxUIRenderer findViewWithName:name];
}

- (nullable LynxUI*)uiWithName:(nonnull NSString*)name {
  return [_lynxUIRenderer uiWithName:name];
}

- (nullable LynxUI*)uiWithIdSelector:(nonnull NSString*)idSelector {
  return [_lynxUIRenderer uiWithIdSelector:idSelector];
}

- (nullable UIView*)viewWithIdSelector:(nonnull NSString*)idSelector {
  return [_lynxUIRenderer viewWithIdSelector:idSelector];
}

- (nullable UIView*)viewWithName:(nonnull NSString*)name {
  return [_lynxUIRenderer viewWithName:name];
}

#pragma mark - Module

- (void)registerModule:(Class<LynxModule> _Nonnull)module param:(id _Nullable)param {
  auto module_factory = [self getModuleFactory];
  if (module_factory) {
    _LogI(@"LynxTemplateRender registerModule: %@ with param (address): %p", module, param);
    module_factory->registerModule(module, param);
  }
}

- (BOOL)isModuleExist:(NSString* _Nonnull)moduleName {
  auto module_factory = [self getModuleFactory];
  if (module_factory) {
    return [module_factory->getModuleClasses() objectForKey:moduleName] != nil;
  }
  return NO;
}

- (nullable JSModule*)getJSModule:(nonnull NSString*)name {
  if (_context != nil) {
    return [_context getJSModule:name];
  }
  return NULL;
}

#pragma mark - Setter & Getter

- (LynxLifecycleDispatcher*)getLifecycleDispatcher {
  return [[self getLynxView] getLifecycleDispatcher];
};

- (void)setEnableAsyncDisplay:(BOOL)enableAsyncDisplay {
  _enableAsyncDisplayFromNative = enableAsyncDisplay;
}

- (BOOL)enableAsyncDisplay {
  return _enableAsyncDisplayFromNative &&
         (pageConfig_ == nullptr || pageConfig_->GetEnableAsyncDisplay());
}

- (BOOL)enableTextNonContiguousLayout {
  return pageConfig_ != nullptr && pageConfig_->GetEnableTextNonContiguousLayout();
}

- (LynxContext*)getLynxContext {
  return _context;
}

- (LynxThreadStrategyForRender)getThreadStrategyForRender {
  return _threadStrategyForRendering;
}

- (void)setNeedPendingUIOperation:(BOOL)needPendingUIOperation {
  _needPendingUIOperation = needPendingUIOperation;
  if (!self->shell_->IsDestroyed()) {
    self->shell_->SetEnableUIFlush(!needPendingUIOperation);
  }
}

- (LynxUIOwner*)uiOwner {
  return [_lynxUIRenderer uiOwner];
}

- (LynxPerformanceController*)performanceController {
  return _performanceController;
}

- (LynxEngineProxy*)getEngineProxy {
  return _lynxEngineProxy;
}

- (BOOL)enableLayoutOnly {
  return _enableLayoutOnly;
}

// TODO(chengjunnan): remove this unused property.
- (void)setImageDownsampling:(BOOL)enableImageDownsampling {
  _enableImageDownsampling = enableImageDownsampling;
}

- (BOOL)enableImageDownsampling {
  return pageConfig_ != nullptr && pageConfig_->GetEnableImageDownsampling();
}

- (BOOL)enableNewImage {
  return pageConfig_ != nullptr && pageConfig_->GetEnableNewImage();
}

- (BOOL)trailNewImage {
  return pageConfig_ != nullptr &&
         pageConfig_->GetTrailNewImage() == lynx::tasm::TernaryBool::TRUE_VALUE;
}

- (void)setTemplateRenderDelegate:(LynxTemplateRenderDelegateExternal*)delegate {
  if (_containerView != nil) {
    _LogE(@"LynxTemplateRender can not setTemplateRenderDelegate, _lynxView has been attached.");
    return;
  }
  _delegate = delegate;
}

#pragma mark - Get Info

- (NSDictionary*)getCurrentData {
  std::unique_ptr<lepus_value> data = shell_->GetCurrentData();
  if (data == nullptr) {
    LLogWarn(@"getCurrentData with nullptr;");
    return @{};
  }
  NSString* json = [NSString stringWithUTF8String:(lepusValueToJSONString(*(data.get())).data())];
  NSData* nsData = [json dataUsingEncoding:NSUTF8StringEncoding];
  if (!nsData) {
    LLogWarn(@"getCurrentData with nil data;");
    return @{};
  }
  NSError* error;
  NSDictionary* dict = [NSJSONSerialization JSONObjectWithData:nsData
                                                       options:NSJSONReadingMutableContainers
                                                         error:&error];
  if (error) {
    LLogWarn(@"getCurrentData error: %@", error);
    return @{};
  }
  return dict;
}

- (NSDictionary*)getPageDataByKey:(NSArray*)keys {
  std::vector<std::string> keysVec;
  for (NSString* key : keys) {
    keysVec.emplace_back([key UTF8String]);
  }
  lepus_value data = shell_->GetPageDataByKey(std::move(keysVec));
  if (data.IsNil()) {
    LLogWarn(@"getCurrentData return nullptr;");
    return @{};
  }

  NSString* json = [NSString stringWithUTF8String:(lepusValueToJSONString(data).data())];
  NSData* nsData = [json dataUsingEncoding:NSUTF8StringEncoding];
  if (!nsData) {
    LLogWarn(@"getCurrentData with nil data;");
    return @{};
  }
  NSError* error;
  NSDictionary* dict = [NSJSONSerialization JSONObjectWithData:nsData
                                                       options:NSJSONReadingMutableContainers
                                                         error:&error];
  if (error) {
    LLogWarn(@"getCurrentData error: %@", error);
    return @{};
  }
  return dict;
}

- (NSString*)cardVersion {
  if (!pageConfig_) {
    return @"error";
  } else {
    return [NSString stringWithUTF8String:pageConfig_->GetVersion().c_str()];
  }
}

- (NSDictionary*)getAllJsSource {
  NSMutableDictionary* dict = [[NSMutableDictionary alloc] init];
  for (auto const& item : shell_->GetAllJsSource()) {
    auto key = [NSString stringWithCString:item.first.c_str() encoding:NSUTF8StringEncoding];
    auto value = [NSString stringWithCString:item.second.c_str() encoding:NSUTF8StringEncoding];
    if (key && value) {
      [dict setObject:value forKey:key];
    }
  }
  return dict;
}

- (nullable NSNumber*)getLynxRuntimeId {
  if (_context != nil) {
    return [_context getLynxRuntimeId];
  }
  return nil;
}

- (int32_t)instanceId {
  if (_context) {
    return _context.instanceId;
  }
  return kUnknownInstanceId;
}

- (BOOL)isLayoutFinish {
  return [_paintingContextProxy isLayoutFinish];
}

- (float)rootWidth {
  if (_shadowNodeOwner != nil) {
    return [_shadowNodeOwner rootWidth];
  } else {
    return 0;
  }
}

- (float)rootHeight {
  if (_shadowNodeOwner != nil) {
    return [_shadowNodeOwner rootHeight];
  } else {
    return 0;
  }
}

- (nullable NSDictionary*)getExtraInfo {
  return _extra;
}
#pragma mark - Intersection

- (void)notifyIntersectionObservers {
  if (_context && _context.intersectionManager) {
    [_context.intersectionManager notifyObservers];
  }
}

#pragma mark - Multi Thread

- (void)syncFlush {
  if (![NSThread isMainThread]) {
    NSString* stack =
        [[[NSThread callStackSymbols] valueForKey:@"description"] componentsJoinedByString:@"\n"];
    NSString* errMsg = [NSString
        stringWithFormat:
            @"LynxTemplateRender %p syncFlush must be called on ui thread, thread:%@, stack:%@",
            self, [NSThread currentThread], stack];
    [self onErrorOccurred:ECLynxThreadWrongThreadSyncFlushError message:errMsg];
    _LogE(@"LynxTemplateRender %p syncFlush must be called on ui thread, thread:%@", self,
          [NSThread currentThread]);
    return;
  }
  shell_->Flush();
}

- (void)attachEngineToUIThread {
  if (![NSThread isMainThread]) {
    _LogE(@"attachEngineToUIThread should be called on ui thread, url: %@", _url);
    return;
  }
  switch (_threadStrategyForRendering) {
    case LynxThreadStrategyForRenderMostOnTASM:
      _threadStrategyForRendering = LynxThreadStrategyForRenderAllOnUI;
      break;
    case LynxThreadStrategyForRenderMultiThreads:
      _threadStrategyForRendering = LynxThreadStrategyForRenderPartOnLayout;
      break;
    default:
      return;
  }
  shell_->AttachEngineToUIThread();
  [self onThreadStrategyUpdated];
}

- (void)detachEngineFromUIThread {
  if (![NSThread isMainThread]) {
    _LogE(@"detachEngineFromUIThread should be called on ui thread, url: %@", _url);
    return;
  }
  switch (_threadStrategyForRendering) {
    case LynxThreadStrategyForRenderAllOnUI:
      _threadStrategyForRendering = LynxThreadStrategyForRenderMostOnTASM;
      break;
    case LynxThreadStrategyForRenderPartOnLayout:
      _threadStrategyForRendering = LynxThreadStrategyForRenderMultiThreads;
      break;
    default:
      return;
  }
  shell_->DetachEngineFromUIThread();
  [self onThreadStrategyUpdated];
}

- (void)onThreadStrategyUpdated {
  [LynxEventReporter updateGenericInfo:@(_threadStrategyForRendering)
                                   key:kPropThreadMode
                            instanceId:_context.instanceId];
}

#pragma mark - Internal Extension

- (NSInteger)logBoxImageSizeWarningThreshold {
  if (pageConfig_ != nullptr) {
    return pageConfig_->GetLogBoxImageSizeWarningThreshold();
  }
  return -1;
}

- (void)setPlatformConfig:(NSString*)configJsonString {
  if (configJsonString) {
    shell_->SetPlatformConfig(configJsonString.UTF8String);
  }
}

- (void)didMoveToWindow:(BOOL)windowIsNil {
  [_devTool onMovedToWindow];
  [_lynxUIRenderer didMoveToWindow:windowIsNil];
}

- (BOOL)enableNewListContainer {
  return pageConfig_ != nullptr && pageConfig_->GetEnableNewListContainer();
}

- (void)runOnTasmThread:(dispatch_block_t)task {
  [_lynxEngineProxy dispatchTaskToLynxEngine:task];
}

- (LynxGestureArenaManager*)getGestureArenaManager {
  return [_lynxUIRenderer getGestureArenaManager];
}

#pragma mark - Helper

- (void)executeNativeOpSafely:(void(NS_NOESCAPE ^)(void))block
            withErrorCallback:(void(NS_NOESCAPE ^)(NSString* msg, NSString* stack))onError {
  NSString* errorMsg = nil;
  NSString* errorStack = nil;
  try {
    @try {
      block();
    } @catch (NSException* exception) {
      errorMsg = [NSString stringWithFormat:@"%@:%@", [exception name], [exception reason]];
      errorStack = [LynxCallStackUtil getCallStack:exception];
    }
  } catch (const std::runtime_error& e) {
    errorMsg = [NSString stringWithUTF8String:e.what()];
  } catch (const std::exception& e) {
    errorMsg = [NSString stringWithUTF8String:e.what()];
  } catch (const char* msg) {
    errorMsg = [NSString stringWithUTF8String:msg];
  } catch (const lynx::base::LynxError& e) {
    errorMsg = [NSString stringWithUTF8String:e.error_message_.c_str()];
  } catch (...) {
    errorMsg = @"Unknow fatal exception";
  }

  if (errorMsg) {
    __weak LynxTemplateRender* weakSelf = self;
    [LynxTemplateRender runOnMainThread:^() {
      __strong LynxTemplateRender* strongSelf = weakSelf;
      if (strongSelf) {
        [strongSelf->_delegate templateRenderOnResetViewAndLayer:strongSelf];
      }
    }];

    [LynxEventReporter clearCacheForInstanceId:_context.instanceId];
    _context.instanceId = kUnknownInstanceId;
    shell_->Destroy();
    if (onError) {
      onError(errorMsg, errorStack);
    }
  }
}

- (void)executeUpdateDataSafely:(void (^)(void))block {
  if (!self->shell_->IsDestroyed()) {
    __weak LynxTemplateRender* weakSelf = self;
    [self
        executeNativeOpSafely:^() {
          [self requestLayoutWhenSafepointEnable];
          if (![NSThread isMainThread]) {
            LOGE("update data on other thread:" << [NSThread currentThread]);
          }
          block();
        }
        withErrorCallback:^(NSString* msg, NSString* stack) {
          __strong LynxTemplateRender* strongSelf = weakSelf;
          [strongSelf reportError:ECLynxDataFlowUpdateException withMsg:msg rawStack:stack];
        }];
  }
}

- (NSMutableDictionary*)processUrl:(NSString*)url {
  NSMutableDictionary* res = [[NSMutableDictionary alloc] init];
  NSString* templateUrl = url;
  NSString* postUrl = @"";
  NSString* compileKey = @"compile_path";
  NSString* postKey = @"post_url";
  [res setObject:templateUrl forKey:compileKey];
  [res setObject:postUrl forKey:postKey];

  NSString* sep = @"&=?";
  NSCharacterSet* set = [NSCharacterSet characterSetWithCharactersInString:sep];
  NSArray* temp = [url componentsSeparatedByCharactersInSet:set];
  for (size_t i = 0; i < [temp count] - 1; ++i) {
    if ([temp[i] isEqualToString:compileKey]) {
      [res setObject:temp[i + 1] forKey:compileKey];
    } else if ([temp[i] isEqualToString:postKey]) {
      [res setObject:temp[i + 1] forKey:postKey];
    }
  }
  return res;
}

- (NSString*)formatLynxSchema:(NSString*)url {
  if (!url) {
    return url;
  }

  {
    // handle abnormal url like @"this is a test url", return itself without processing
    NSURL* uri = [NSURL URLWithString:url];
    if (!uri) {
      return url;
    }
  }

  // Clear query parameters on target NSURLComponents
  NSURLComponents* originalComponents = [[NSURLComponents alloc] initWithString:url];
  NSURLComponents* targetComponents = [[NSURLComponents alloc] initWithString:url];
  [targetComponents setQueryItems:nil];

  // Append reserved queries to targetComponents
  NSArray<NSURLQueryItem*>* queryItems = [originalComponents queryItems];
  NSSet<NSString*>* reservedParamKeys =
      [[NSSet alloc] initWithObjects:@"surl", @"url", @"channel", @"bundle", nil];
  NSMutableArray<NSURLQueryItem*>* appendedParams = [[NSMutableArray alloc] init];
  for (NSURLQueryItem* item in queryItems) {
    if ([reservedParamKeys containsObject:[item name]] && [item value] && [item value].length > 0) {
      [appendedParams addObject:item];
    }
  }
  [targetComponents setQueryItems:appendedParams];
  return [targetComponents string];
}

- (NSDictionary*)prepareEntryForInvokeUIMethodCallbackParam:(NSDictionary*)params {
  static NSString* const kDefaultEntry = @"__Card__";
  static NSString* const kEntryName = @"entry_name";
  NSMutableDictionary* resWithEntry = [[NSMutableDictionary alloc] init];
  [resWithEntry addEntriesFromDictionary:params];
  [resWithEntry setObject:kDefaultEntry forKey:kEntryName];
  return resWithEntry;
}

#pragma mark - Timing & Report

- (void)setExtraTiming:(LynxExtraTiming*)timing {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_RENDER_SET_EXTRA_TIMING);
  if (timing.openTime > 0) {
    [self setTiming:timing.openTime * 1000 key:kTimingOpenTime pipelineID:nil];
  }
  if (timing.containerInitStart > 0) {
    [self setTiming:timing.containerInitStart * 1000 key:kTimingContainerInitStart pipelineID:nil];
  }
  if (timing.containerInitEnd > 0) {
    [self setTiming:timing.containerInitEnd * 1000 key:kTimingContainerInitEnd pipelineID:nil];
  }
  if (timing.prepareTemplateStart > 0) {
    [self setTiming:timing.prepareTemplateStart * 1000
                key:kTimingPrepareTemplateStart
         pipelineID:nil];
  }
  if (timing.prepareTemplateEnd > 0) {
    [self setTiming:timing.prepareTemplateEnd * 1000 key:kTimingPrepareTemplateEnd pipelineID:nil];
  }
}

- (void)setFluencyTracerEnabled:(LynxBooleanOption)enabled {
  [_lynxUIRenderer setFluencyTracerEnabled:enabled];
}

- (void)setLongTaskMonitorEnabled:(LynxBooleanOption)enabled {
  auto options = shell_->GetPageOptions();
  options.SetLongTaskMonitorDisabled(enabled == LynxBooleanOptionFalse);
  shell_->SetPageOptions(options);
}

- (void)putExtraParamsForReportingEvents:(NSDictionary<NSString*, id>*)params {
  [LynxEventReporter putExtraParams:params forInstanceId:self.instanceId];
}

- (nullable NSDictionary*)getAllTimingInfo {
  return lynx::tasm::convertLepusValueToNSObject(shell_->GetAllTimingInfo());
}

- (void)setTiming:(uint64_t)timestamp key:(NSString*)key pipelineID:(nullable NSString*)pipelineID {
  [_performanceController setTiming:timestamp key:key pipelineID:pipelineID];
}

- (void)resetTimingBeforeReload {
  [_performanceController resetTimingBeforeReload];
}

- (void)onPipelineStart:(std::string)pipelineID
            pipelineOrigin:(std::string)pipelineOrigin
    pipelineStartTimestamp:(uint64_t)timestamp {
  [_performanceController onPipelineStart:[NSString stringWithUTF8String:pipelineID.c_str()]
                           pipelineOrigin:[NSString stringWithUTF8String:pipelineOrigin.c_str()]
                                timestamp:timestamp];
}

- (void)markTiming:(const char*)key pipelineID:(const char*)pipelineID {
  [_performanceController markTiming:[NSString stringWithUTF8String:key]
                          pipelineID:[NSString stringWithUTF8String:pipelineID]];
}

/**
 * report error caught by function executeNativeOpSafely, dont use it to report other error
 */
- (void)reportError:(NSInteger)code withMsg:(NSString*)msg rawStack:(NSString*)stack {
  LynxError* error = [LynxError lynxErrorWithCode:code
                                          message:msg
                                    fixSuggestion:@""
                                            level:LynxErrorLevelError];
  if (stack) {
    [error addCustomInfo:stack forKey:@"raw_stack"];
  }
  error.callStack = [LynxCallStackUtil getCallStack];
  [self onErrorOccurred:error];
}

- (void)onFetchTemplateError:(NSError*)error {
  NSString* error_msg = @"Error occurred while fetching app bundle resource";
  LynxError* lynxError = [LynxError lynxErrorWithCode:ECLynxAppBundleLoadBadResponse
                                              message:error_msg];
  if (error) {
    [lynxError setRootCause:[error localizedDescription]];
  }
  [lynxError setCallStack:[LynxCallStackUtil getCallStack]];
  [self onErrorOccurred:lynxError];
}

- (void)onErrorOccurred:(LynxError*)error {
  if (!error || ![error isValid]) {
    _LogE(@"receive invalid error");
    return;
  }
  [_lynxSSRHelper onErrorOccurred:error.code sourceError:error];

  [error setTemplateUrl:_url];
  [error setCardVersion:[self cardVersion]];

  [_devTool showErrorMessage:error];
  NSString* jsonStr = [error.userInfo objectForKey:LynxErrorUserInfoKeyMessage];
  if (!jsonStr || [jsonStr length] == 0) {
    _LogI(@"Failed to parse error to json");
    return;
  }

  _LogE(@"LynxTemplateRender onErrorOccurred message: %@ in %p", jsonStr, self);
  if (!error.isLogBoxOnly) {
    [LynxEventReporter onEvent:kLynxSDKErrorEvent
                    instanceId:[_context instanceId]
                  propsBuilder:^NSDictionary<NSString*, NSObject*>* _Nonnull {
                    return @{
                      @"code" : @(error.errorCode),
                      @"level" : error.level ?: @"",
                      @"summary_message" : error.summaryMessage ?: @"",
                    };
                  }];
    [self dispatchError:error];
  }
}

- (void)onErrorOccurred:(NSInteger)code sourceError:(NSError*)source {
  if (source) {
    std::string errorMessage = [[source localizedDescription] UTF8String];
    [self onErrorOccurred:code
                  message:[NSString
                              stringWithCString:lynx::base::debug::GetBacktraceInfo(errorMessage)
                                                    .c_str()
                                       encoding:NSUTF8StringEncoding]];

    [_lynxSSRHelper onErrorOccurred:code sourceError:source];
  }
}

- (void)updateMemoryUsage {
  if (![LynxPerformanceController isMemoryMonitorEnabled]) {
    return;
  }
  __weak __typeof(self) weakSelf = self;
  int delay = [[LynxEnv sharedInstance] memoryAcquisitionDelaySec];
  // Since resources are usually loaded asynchronously, such as images downloaded asynchronously
  // from the network, it is necessary to delay the collection of memory so as to collect as much
  // resource memory as possible.
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(delay * NSEC_PER_SEC)),
                 dispatch_get_main_queue(), ^{
                   __strong __typeof(weakSelf) strongSelf = weakSelf;
                   if (!strongSelf) {
                     return;
                   }
                   NSDictionary<NSString*, LynxMemoryRecord*>* records =
                       [[strongSelf uiOwner] getMemoryUsage];
                   [[strongSelf performanceController] updateMemoryUsageWithRecords:records];
                 });
}

#pragma mark - Preload

- (void)attachLynxView:(LynxView* _Nonnull)lynxView {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_RENDER_ATTACH_LYNX_VIEW);
  _containerView = lynxView;
  _delegate = (id<LynxTemplateRenderDelegate>)lynxView;

  if ([_delegate respondsToSelector:@selector(templateRenderSetLayoutOption:)]) {
    [_delegate templateRenderSetLayoutOption:self];
  }

  if (shell_ != nullptr) {
    shell_->StartJsRuntime();
  }

  if (_uilayoutTick) {
    [_uilayoutTick attach:lynxView];
  }

  if (_lynxUIRenderer != nil) {
    [_lynxUIRenderer attachContainerView:lynxView];
  }

  if (_devTool) {
    [_devTool attachLynxView:lynxView];
  }
}

- (BOOL)processRender:(LynxView* _Nonnull)lynxView {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_RENDER_PROCESS_RENDER, INSTANCE_ID, [self instanceId]);
  if (shell_ == nullptr || shell_->IsDestroyed()) {
    return NO;
  }

  if (_containerView == nil) {
    [self attachLynxView:lynxView];
  }

  if (_needPendingUIOperation) {
    _hasRendered = YES;
    // Should set enable flush before forceFlush for list.
    [self setNeedPendingUIOperation:NO];
    shell_->ForceFlush();
    _LogI(@"LynxTemplateRender process render finished");
  }
  return YES;
}

- (void)processLayout:(nonnull NSData*)tem
              withURL:(nonnull NSString*)url
             initData:(nullable LynxTemplateData*)data {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_RENDER_PROCESS_LAYOUT);
  [self setNeedPendingUIOperation:YES];
  [self loadTemplate:tem withURL:url initData:data];
}

- (void)processLayoutWithTemplateBundle:(LynxTemplateBundle*)bundle
                                withURL:(NSString*)url
                               initData:(LynxTemplateData*)data {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_RENDER_PROCESS_LAYOUT_WITH_TEMPLATE_BUNDLE);
  [self setNeedPendingUIOperation:YES];
  [self loadTemplateBundle:bundle withURL:url initData:data];
}

- (void)processLayoutWithSSRData:(nonnull NSData*)tem
                         withURL:(nonnull NSString*)url
                        initData:(nullable LynxTemplateData*)data {
  [self setNeedPendingUIOperation:YES];
  [self loadSSRData:tem withURL:url initData:data];
}

#pragma mark - TemplateRenderCallbackProtocol

- (void)onDataUpdated {
  [_delegate templateRenderOnDataUpdated:self];
}

- (void)onPageChanged:(BOOL)isFirstScreen {
  if (!isFirstScreen) {
    [_devTool onPageUpdate];
  }
  [_delegate templateRender:self onPageChanged:isFirstScreen];
  [self updateMemoryUsage];
}

- (void)onTasmFinishByNative {
  [_delegate templateRenderOnTasmFinishByNative:self];
}

- (void)onTemplateLoaded:(NSString*)url {
  [_delegate templateRender:self onTemplateLoaded:url];
  [_devTool onLoadFinished];
}

- (void)onSSRHydrateFinished:(NSString*)url {
  [_lynxSSRHelper onHydrateFinished];
}

- (void)onRuntimeReady {
  [_delegate templateRenderOnRuntimeReady:self];
}

- (void)onTimingSetup:(NSDictionary*)timingInfo {
  [[self getLifecycleDispatcher] lynxView:[self getLynxView] onSetup:timingInfo];
}

- (void)onTimingUpdate:(NSDictionary*)timingInfo updateTiming:(NSDictionary*)updateTiming {
  [[self getLifecycleDispatcher] lynxView:[self getLynxView]
                                 onUpdate:timingInfo
                                   timing:updateTiming];
}

- (void)onPerformanceEvent:(NSDictionary*)originDict {
  [[self getLifecycleDispatcher]
      onPerformanceEvent:[LynxPerformanceEntryConverter makePerformanceEntry:originDict]];
}

- (void)onFirstLoadPerf:(NSDictionary*)perf {
}

- (void)onUpdatePerfReady:(NSDictionary*)perf {
}

- (void)onDynamicComponentPerf:(NSDictionary*)perf {
  [_delegate templateRender:self onReceiveDynamicComponentPerf:perf];
}

- (void)setPageConfig:(const std::shared_ptr<lynx::tasm::PageConfig>&)pageConfig {
  pageConfig_ = pageConfig;
  [_lynxUIRenderer setPageConfig:pageConfig context:_context];
}

- (NSString*)translatedResourceWithId:(NSString*)resId themeKey:(NSString*)key {
  return [_delegate templateRender:self translatedResourceWithId:resId themeKey:key];
}

- (void)getI18nResourceForChannel:(NSString*)channel withFallbackUrl:(NSString*)url {
  LynxResourceRequest* request =
      [[LynxResourceRequest alloc] initWithUrl:[channel lowercaseString]];
  __weak typeof(self) weakSelf = self;
  [[_providerRegistry getResourceProviderByKey:LYNX_PROVIDER_TYPE_I18N_TEXT]
         request:request
      onComplete:^(LynxResourceResponse* _Nonnull response) {
        void (^task)() = ^void() {
          __strong __typeof(weakSelf) strongSelf = weakSelf;
          if (strongSelf != nil) {
            if (response.data == nil) {
              strongSelf->shell_->UpdateI18nResource(std::string([channel UTF8String]), "");
            } else {
              strongSelf->shell_->UpdateI18nResource(std::string([channel UTF8String]),
                                                     std::string([[response data] UTF8String]));
            }
          }
        };
        [LynxTemplateRender runOnMainThread:task];
      }];
}

+ (void)runOnMainThread:(void (^)())task {
  if (NSThread.isMainThread) {
    task();
  } else {
    dispatch_async(dispatch_get_main_queue(), task);
  }
}

- (void)onCallJSBFinished:(NSDictionary*)info {
  [_delegate templateRender:self onCallJSBFinished:info];
  [LynxDebugger onPerfMetricsEvent:@"lynxsdk_jsb_timing"
                          withData:info
                        instanceId:[self instanceId]];
}

- (void)onReceiveMessageEvent:(NSDictionary*)event {
  [_devTool onReceiveMessageEvent:event];
}

- (void)onJSBInvoked:(NSDictionary*)info {
  [_delegate templateRender:self onJSBInvoked:info];
}

- (NSMutableDictionary<NSString*, id>*)getLepusModulesClasses {
  return self.lepusModulesClasses;
}

- (BOOL)enableBackgroundShapeLayer {
  return pageConfig_ && pageConfig_->GetEnableBackgroundShapeLayer();
}

- (BOOL)enableAirStrictMode {
  return _enableAirStrictMode;
}

- (void)preloadLazyBundles:(NSArray* _Nonnull)urls {
  std::vector<std::string> preload_urls;
  for (NSString* url : urls) {
    preload_urls.emplace_back(lynx::base::SafeStringConvert([url UTF8String]));
  }
  self->shell_->PreloadLazyBundles(std::move(preload_urls));
}

- (BOOL)registerLazyBundle:(nonnull NSString*)url bundle:(nonnull LynxTemplateBundle*)bundle {
  NSString* errorMsg = nil;
  NSString* rootCause = nil;
  if ([url length] == 0) {
    errorMsg = @"url is empty";
  } else if (bundle == nil) {
    errorMsg = @"bundle is nil";
  } else if ([bundle errorMsg]) {
    errorMsg = @"bundle is invalid";
    rootCause = [bundle errorMsg];
  } else {
    auto shared_raw_bundle = LynxGetRawTemplateBundle(bundle);
    if (shared_raw_bundle && !shared_raw_bundle->IsCard()) {
      // only valid bundles from lazy bundle templates will be passed to the shell
      self->shell_->RegisterLazyBundle(lynx::base::SafeStringConvert([url UTF8String]),
                                       *shared_raw_bundle);
    } else {
      errorMsg = @"input bundle is not from a lazy bundle template";
    }
  }
  if (errorMsg != nil) {
    LynxError* lynxError = [LynxError lynxErrorWithCode:ECLynxLazyBundleLoadBadBundle
                                                message:errorMsg];
    [lynxError setRootCause:rootCause];
    [lynxError addCustomInfo:url forKey:@"component_url"];
    [self onErrorOccurred:lynxError];
    return NO;
  }
  return YES;
}

- (BOOL)onLynxEvent:(LynxEvent*)event {
  if (event.eventType == kTouchEvent && ((LynxTouchEvent*)event).isMultiTouch) {
    NSMutableArray* targetKeysToRemove = [NSMutableArray array];
    NSMutableDictionary* uiTouchMap = ((LynxTouchEvent*)event).uiTouchMap;
    [uiTouchMap enumerateKeysAndObjectsUsingBlock:^(NSString* targetKey, NSMutableArray* touchArray,
                                                    BOOL* stop) {
      NSMutableDictionary* touchMap = [NSMutableDictionary new];
      for (NSMutableArray* touch in touchArray) {
        [touchMap setValue:@[ [touch objectAtIndex:5], [touch objectAtIndex:6] ]
                    forKey:[touch objectAtIndex:0]];
      }
      id<LynxEventTarget> target =
          (id<LynxEventTarget>)[((LynxTouchEvent*)event).activeUIMap valueForKey:targetKey];
      LynxTouchEvent* touchEvent = [[LynxTouchEvent alloc] initWithName:event.eventName
                                                              targetTag:[targetKey intValue]
                                                               touchMap:touchMap];
      LynxEventDetail* detail =
          [[LynxEventDetail alloc] initWithEvent:touchEvent
                                          target:(id<LynxEventTargetBase>)target
                                        lynxView:(LynxView*)[_lynxUIRenderer eventHandlerRootView]];
      if ([target dispatchEvent:detail]) {
        [targetKeysToRemove addObject:targetKey];
      } else {
        if ([_delegate respondsToSelector:@selector(templateRender:onLynxEvent:)]) {
          [_delegate templateRender:self onLynxEvent:detail];
        }
      }
    }];
    [uiTouchMap removeObjectsForKeys:targetKeysToRemove];
    if (![uiTouchMap count]) {
      return YES;
    }
    return NO;
  }

  id<LynxEventTarget> target = (id<LynxEventTarget>)event.eventTarget;
  LynxEventDetail* detail =
      [[LynxEventDetail alloc] initWithEvent:event
                                      target:(id<LynxEventTargetBase>)target
                                    lynxView:(LynxView*)[_lynxUIRenderer rootUI].lynxView];
  if ([target dispatchEvent:detail]) {
    return YES;
  }
  if ([_delegate respondsToSelector:@selector(templateRender:onLynxEvent:)]) {
    [_delegate templateRender:self onLynxEvent:detail];
  }
  return NO;
}

- (BOOL)enableFiberArch {
  return pageConfig_ != nullptr && pageConfig_->GetEnableFiberArch();
}

- (void)onTemplateBundleReady:(LynxTemplateBundle*)bundle {
  if ([_delegate respondsToSelector:@selector(templateRender:onTemplateBundleReady:)]) {
    [_delegate templateRender:self onTemplateBundleReady:bundle];
  }
}

- (void)setEnableUserBytecode:(BOOL)enableUserBytecode url:(nonnull NSString*)url {
  if ([_runtimeOptions backgroundJsRuntimeType] != LynxBackgroundJsRuntimeTypeQuickjs ||
      (_runtimeOptions.enableBytecode == enableUserBytecode &&
       [_runtimeOptions.bytecodeUrl isEqualToString:url])) {
    return;
  }
  _runtimeOptions.enableBytecode = enableUserBytecode;
  _runtimeOptions.bytecodeUrl = url;
  shell_->SetEnableBytecode([_runtimeOptions enableBytecode], [_runtimeOptions bytecodeUrlString]);
}

- (long)initStartTiming {
  return _initStartTiming;
}

- (long)initEndTiming {
  return _initEndTiming;
}

- (void)didInvokeMethod:(nonnull NSString*)method
               inModule:(nonnull NSString*)module
              errorCode:(int32_t)code {
  if (module && method) {
    if (_delegate) {
      // LynxViewLifecycle:willInokveMethod:inModule: is an optional method.
      // Client defined method may throw exception, which will make all module methods not work
      @try {
        [_delegate templateRender:self didInvokeMethod:method inModule:module errorCode:code];
      } @catch (NSException* exception) {
        NSString* errorMsg =
            [NSString stringWithFormat:@"Callback 'didInvokeMethod' called after method '%@' "
                                       @"in module '%@' threw an exception: %@",
                                       method, module, [exception reason]];
        LynxError* error = [LynxError
            lynxErrorWithCode:ECLynxNativeModulesException
                      message:errorMsg
                fixSuggestion:@"This error is caught by native, please ask Lynx for help."
                        level:LynxErrorLevelError];
        [error setCallStack:[LynxCallStackUtil getCallStack]];
        [self onErrorOccurred:error];
      }
    }
  }
}

- (void)invokeLepusFunc:(nonnull NSDictionary*)data callbackID:(int32_t)callbackID {
  [_lynxEngineProxy invokeLepusFunc:data callbackID:callbackID];
}

- (void)onErrorOccurred:(NSInteger)code message:(nonnull NSString*)errMessage {
  if (errMessage) {
    [self onErrorOccurred:[LynxError lynxErrorWithCode:code
                                               message:errMessage
                                         fixSuggestion:@""
                                                 level:LynxErrorLevelError]];
  }
}

- (void)invokeUIMethod:(NSString*)method
                params:(NSDictionary*)params
              callback:(int)callback
                toNode:(int)node {
  __weak LynxTemplateRender* weakSelf = self;
  LynxUIMethodCallbackBlock cb = ^(int code, id _Nullable data) {
    NSDictionary* res = @{@"code" : @(code), @"data" : data ?: @{}};
    if (weakSelf) {
      __strong LynxTemplateRender* strongSelf = weakSelf;
      if (code >= 0) {
        if ([strongSelf enableAirStrictMode]) {
          NSDictionary* resWithEntry = [strongSelf prepareEntryForInvokeUIMethodCallbackParam:res];
          [strongSelf invokeLepusFunc:resWithEntry callbackID:callback];
        } else if (strongSelf->_context && strongSelf->_context->proxy_) {
          strongSelf->_context->proxy_->CallJSApiCallbackWithValue(
              callback, std::make_unique<lynx::pub::ValueImplDarwin>(res));
        }
      }
    }
  };

  [_lynxUIRenderer invokeUIMethodForSelectorQuery:method params:params callback:cb toNode:node];
}

- (id<LynxUIRendererProtocol>)lynxUIRenderer {
  return _lynxUIRenderer;
}

- (LynxShadowNodeOwner*)shadowNodeOwner {
  return _shadowNodeOwner;
}

- (void)onEventCapture:(NSInteger)targetID
        withEventCatch:(BOOL)isCatch
            andEventID:(int64_t)eventID {
  [[_lynxUIRenderer.uiOwner findUIBySign:targetID] onEventCapture:isCatch withEventID:eventID];
}

- (void)onEventBubble:(NSInteger)targetID withEventCatch:(BOOL)isCatch andEventID:(int64_t)eventID {
  [[_lynxUIRenderer.uiOwner findUIBySign:targetID] onEventBubble:isCatch withEventID:eventID];
}

- (void)onEventFire:(NSInteger)targetID withEventStop:(BOOL)isStop andEventID:(int64_t)eventID {
  [[_lynxUIRenderer.uiOwner findUIBySign:targetID] onEventFire:isStop withEventID:eventID];
}

- (LynxViewBuilderBlock)getLynxViewBuilderBlock {
  // TODO(zhoupeng.z): provide with move params
  return ^(LynxViewBuilder* builder) {
    builder.fontScale = self->_fontScale;
    builder.enablePreUpdateData = YES;
    builder.fetcher = self->_fetcher;
    builder.enableGenericResourceFetcher =
        self->_enableGenericResourceFetcher ? LynxBooleanOptionTrue : LynxBooleanOptionFalse;
    builder.genericResourceFetcher = [self->_lynxUIRenderer genericResourceFetcher];
    builder.mediaResourceFetcher = [self->_lynxUIRenderer mediaResourceFetcher];
    builder.templateResourceFetcher = [self->_lynxUIRenderer templateResourceFetcher];
    builder.screenSize = [[self->_lynxUIRenderer getScreenMetrics] screenSize];
    builder.lynxBackgroundRuntimeOptions =
        [[LynxBackgroundRuntimeOptions alloc] initWithOptions:self->_runtimeOptions];
    [builder setThreadStrategyForRender:self->_threadStrategyForRendering];
  };
}

@end
