// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxTemplateRender.h"
#import "JSModule.h"
#import "LynxAccessibilityModule.h"
#import "LynxBackgroundRuntime+Internal.h"
#import "LynxCallStackUtil.h"
#import "LynxConfig+Internal.h"
#import "LynxContext+Internal.h"
#import "LynxDebugger.h"
#import "LynxDevtool+Internal.h"
#import "LynxEngineProxy+Native.h"
#import "LynxEnv+Internal.h"
#import "LynxError.h"
#import "LynxEventReporter.h"
#import "LynxEventReporterUtils.h"
#import "LynxExposureModule.h"
#import "LynxFetchModule.h"
#import "LynxFontFaceManager.h"
#import "LynxGroup+Internal.h"
#import "LynxIntersectionObserverModule.h"
#import "LynxLoadMeta.h"
#import "LynxLog.h"
#import "LynxPerformanceEntryConverter.h"
#import "LynxResourceModule.h"
#import "LynxRootUI.h"
#import "LynxSSRHelper.h"
#import "LynxScreenMetrics.h"
#import "LynxService.h"
#import "LynxServiceExtensionProtocol.h"
#import "LynxSetModule.h"
#import "LynxSubErrorCode.h"
#import "LynxTemplateBundle+Converter.h"
#import "LynxTemplateBundle.h"
#import "LynxTemplateData+Converter.h"
#import "LynxTemplateData.h"
#import "LynxTemplateRender+Protected.h"
#import "LynxTextInfoModule.h"
#import "LynxTheme.h"
#import "LynxTimingConstants.h"
#import "LynxTraceEvent.h"
#import "LynxUIIntersectionObserver+Internal.h"
#import "LynxUILayoutTick.h"
#import "LynxUIMethodModule.h"
#import "LynxUIRenderer.h"
#import "LynxUIRendererProtocol.h"
#import "LynxView.h"
#import "LynxViewBuilder+Internal.h"
#import "LynxViewConfigProcessor.h"
#import "PaintingContextProxy.h"

#include <functional>

#include "base/include/debug/backtrace.h"
#include "core/base/darwin/lynx_env_darwin.h"
#include "core/public/lynx_extension_delegate.h"
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
#include "core/services/timing_handler/timing_collector_platform_impl.h"
#include "core/shell/ios/data_utils.h"
#include "core/shell/ios/native_facade_darwin.h"
#include "core/shell/ios/native_facade_reporter_darwin.h"
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

- (instancetype)initWithBuilderBlock:(void (^)(__attribute__((noescape))
                                               LynxViewBuilder* _Nonnull))block
                            lynxView:(LynxView*)lynxView {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxTemplateRender::initWithBuilderBlock");
  if (self = [super init]) {
    _initStartTiming = [[NSDate date] timeIntervalSince1970] * 1000 * 1000;

    /// Builder
    LynxViewBuilder* builder = [self setUpBuilder];
    if (block) {
      TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxTemplateRender::customBuilder");
      block(builder);
      [LynxViewConfigProcessor processorMap:builder.lynxViewConfig lynxViewBuilder:builder];
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
      [_runtimeOptions merge:_runtime.options];
    }

    [builder.lynxUIRenderer attachLynxView:lynxView];

    /// Member Variable
    CGSize screenSize;
    if (!CGSizeEqualToSize(builder.screenSize, CGSizeZero)) {
      screenSize = builder.screenSize;
    } else {
      screenSize = [UIScreen mainScreen].bounds.size;
    }
    [self setUpVariableWithBuilder:builder lynxView:lynxView screenSize:screenSize];

    /// DevTool
    [self setUpDevTool:builder.debuggable];

    /// UIRenderer
    [self setUpUIRendererWithBuilder:builder screenSize:screenSize];

    /// LynxShell
    [self setUpLynxShellWithLastInstanceId:kUnknownInstanceId];

    /// Event
    [self setUpEventHandler];

    /// Frame
    [self setUpFrame:builder.frame];

    // get runtime id from jsproxy
    if (_enableJSRuntime) {
      [_devTool setRuntimeId:[[self getLynxRuntimeId] integerValue]];
    }

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

- (void)setUpVariableWithBuilder:(LynxViewBuilder*)builder
                        lynxView:(LynxView*)lynxView
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
  // First prepare env
  if (!builder.enableAsyncCreateRender) {
    [self setUpEnvWidthScreenSize:screenSize];
  }

  _enableAsyncDisplayFromNative = YES;
  _enableTextNonContiguousLayout = [builder enableTextNonContiguousLayout];
  _enableLayoutOnly = [LynxEnv.sharedInstance getEnableLayoutOnly];

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

  [self setUpLynxView:lynxView builder:builder];
}

- (void)setUpLynxView:(LynxView*)lynxView builder:(LynxViewBuilder*)builder {
  if (lynxView != nil) {
    _lynxView = lynxView;
    lynxView.clipsToBounds = YES;
    _delegate = (id<LynxTemplateRenderDelegate>)lynxView;
    [lynxView setEnableTextNonContiguousLayout:[builder enableTextNonContiguousLayout]];
    [lynxView setEnableLayoutOnly:[LynxEnv.sharedInstance getEnableLayoutOnly]];
    [lynxView setEnableSyncFlush:[builder enableSyncFlush]];
  }
}

- (void)setUpEnvWidthScreenSize:(CGSize)screenSize {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxTemplateRender::setUpEnvWidthScreenSize");
  [[LynxEnv sharedInstance] initLayoutConfig:screenSize];
}

- (void)setUpDevTool:(BOOL)debuggable {
  if (LynxEnv.sharedInstance.lynxDebugEnabled) {
    if (_runtime) {
      _devTool = _runtime.devtool;
      [_devTool attachLynxView:_lynxView];
    } else {
      _devTool = [[LynxDevtool alloc] initWithLynxView:_lynxView debuggable:debuggable];
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

- (void)setUpUIRendererWithBuilder:(LynxViewBuilder*)builder screenSize:(CGSize)screenSize {
  [builder.lynxUIRenderer setupWithContainerView:_lynxView
                                templateRenderer:self
                                         builder:builder
                                      screenSize:screenSize];
  [_devTool attachLynxUIOwner:[builder.lynxUIRenderer uiOwner]];

  [self setUpResourceProviderWithBuilder:builder];
  [self setUpShadowNodeOwner];
  [self setUpUIDelegate];
}

- (void)setUpResourceProviderWithBuilder:(LynxViewBuilder*)builder {
  LynxProviderRegistry* registry = [[LynxProviderRegistry alloc] init];
  NSDictionary* providers = [LynxEnv sharedInstance].resoureProviders;
  for (NSString* globalKey in providers) {
    [registry addLynxResourceProvider:globalKey provider:providers[globalKey]];
  }
  providers = [builder getLynxResourceProviders];
  for (NSString* key in providers) {
    [registry addLynxResourceProvider:key provider:providers[key]];
  }
  _providerRegistry = registry;

  [_lynxUIRenderer setupResourceProvider:[registry getResourceProviderByKey:LYNX_PROVIDER_TYPE_FONT]
                             withBuilder:builder];
}

- (void)setUpShadowNodeOwner {
  if (!_uilayoutTick) {
    __weak typeof(self) weakSelf = self;
    _uilayoutTick = [[LynxUILayoutTick alloc] initWithRoot:_lynxView
                                                     block:^() {
                                                       __strong LynxTemplateRender* strongSelf =
                                                           weakSelf;
                                                       strongSelf->shell_->TriggerLayout();
                                                     }];
  }

  BOOL isAsyncLayout = _threadStrategyForRendering != LynxThreadStrategyForRenderAllOnUI;
  _shadowNodeOwner = [[LynxShadowNodeOwner alloc] initWithUIOwner:[_lynxUIRenderer uiOwner]
                                                       layoutTick:_uilayoutTick
                                                    isAsyncLayout:isAsyncLayout];
}

- (void)setUpEventHandler {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxTemplateRender::setUpEventHandler");
  [_lynxUIRenderer setupEventHandler:self
                         engineProxy:_lynxEngineProxy
                            lynxView:_lynxView
                             context:_context
                            shellPtr:reinterpret_cast<int64_t>(shell_.get())];
}

- (void)setUpUIDelegate {
  lynx::tasm::UIDelegateDarwin* ui_delegate = new lynx::tasm::UIDelegateDarwin(
      [_lynxUIRenderer uiOwner], [[LynxEnv sharedInstance] enableCreateUIAsync], _shadowNodeOwner);
  [_lynxUIRenderer onSetupUIDelegate:ui_delegate];
}

- (void)setUpLynxShellWithLastInstanceId:(int32_t)lastInstanceId {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxTemplateRender::setUpLynxShell");

  // Env
  lynx::tasm::LynxEnvDarwin::initNativeUIThread();
  LynxScreenMetrics* screenMetrics = [_lynxUIRenderer getScreenMetrics];
  auto lynx_env_config = lynx::tasm::LynxEnvConfig(
      screenMetrics.screenSize.width, screenMetrics.screenSize.height, 1.f, screenMetrics.scale);

  // Resource Loader
  id<LynxTemplateResourceFetcher> templateResourceFetcher =
      [_lynxUIRenderer templateResourceFetcher];
  id<LynxGenericResourceFetcher> genericResourceFetcher = [_lynxUIRenderer genericResourceFetcher];
  auto loader = std::make_shared<lynx::tasm::LazyBundleLoader>(
      std::make_shared<lynx::shell::LynxResourceLoaderDarwin>(
          nil, _fetcher, self, templateResourceFetcher, genericResourceFetcher));

  // Timing
  timing_collector_platform_impl_ =
      std::make_shared<lynx::tasm::timing::TimingCollectorPlatformImpl>();

  // Build shell
  auto ui_delegate = [_lynxUIRenderer uiDelegate];
  auto painting_context = ui_delegate->CreatePaintingContext();
  if ([_lynxUIRenderer needPaintingContextProxy]) {
    _paintingContextProxy = [[PaintingContextProxy alloc]
        initWithPaintingContext:reinterpret_cast<lynx::tasm::PaintingContextDarwin*>(
                                    painting_context.get())];
    [_shadowNodeOwner setDelegate:_paintingContextProxy];
  }
  shell_.reset(
      lynx::shell::LynxShellBuilder()
          .SetNativeFacade(std::make_unique<lynx::shell::NativeFacadeDarwin>(self))
          .SetNativeFacadeReporter(std::make_unique<lynx::shell::NativeFacadeReporterDarwin>(self))
          .SetPaintingContextPlatformImpl(std::move(painting_context))
          .SetLynxEnvConfig(lynx_env_config)
          .SetEnableElementManagerVsyncMonitor(true)
          .SetEnableLayoutOnly(_enableLayoutOnly)
          .SetWhiteBoard(_runtimeOptions.group ? _runtimeOptions.group.whiteBoard : nullptr)
          .SetLazyBundleLoader(loader)
          .SetTasmLocale(std::string([[[LynxEnv sharedInstance] locale] UTF8String]))
          .SetEnablePreUpdateData(_enablePreUpdateData)
          .SetLayoutContextPlatformImpl(ui_delegate->CreateLayoutContext())
          .SetStrategy(
              static_cast<lynx::base::ThreadStrategyForRendering>(_threadStrategyForRendering))
          .SetEngineActor([loader, lynxEngineProxy = _lynxEngineProxy](auto& actor) {
            loader->SetEngineActor(actor);
            [lynxEngineProxy
                setNativeEngineProxy:std::make_shared<lynx::shell::LynxEngineProxyDarwin>(actor)];
          })
          .SetPropBundleCreator(ui_delegate->CreatePropBundleCreator())
          .SetRuntimeActor(_runtime ? _runtime.runtimeActor : nullptr)
          .SetTimingActor(_runtime ? _runtime.timingActor : nullptr)
          .SetShellOption([self setUpShellOption])
          .SetTasmPlatformInvoker(std::make_unique<lynx::shell::TasmPlatformInvokerDarwin>(self))
          .SetTimingCollectorPlatform(timing_collector_platform_impl_)
          .SetUseInvokeUIMethodFunction(_lynxUIRenderer.useInvokeUIMethodFunction)
          .build());

  [_devTool onTemplateAssemblerCreated:(intptr_t)shell_.get()];

  // Runtime
  [self setUpRuntimeWithLastInstanceId:lastInstanceId];

  // Update info
  [self updateNativeTheme];
  [self updateNativeGlobalProps];

  // reset ui flush flag
  [self setNeedPendingUIOperation:_needPendingUIOperation];

  // FIXME
  shell_->SetFontScale(_fontScale);

  // Thread pool
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    lynx::tasm::LynxGlobalPool::GetInstance().PreparePool();
  });
}

- (lynx::shell::ShellOption)setUpShellOption {
  lynx::shell::ShellOption option;
  option.enable_js_ = self.enableJSRuntime;
  option.enable_js_group_thread_ = _enableJSGroupThread;
  if (_enableJSGroupThread) {
    option.js_group_thread_name_ = [_runtimeOptions groupID];
  }
  option.enable_multi_tasm_thread_ =
      _enableMultiAsyncThread ||
      [[LynxEnv sharedInstance] boolFromExternalEnv:LynxEnvEnableMultiTASMThread defaultValue:NO];
  option.enable_multi_layout_thread_ =
      _enableMultiAsyncThread ||
      [[LynxEnv sharedInstance] boolFromExternalEnv:LynxEnvEnableMultiLayoutThread defaultValue:NO];
  option.enable_async_hydration_ = _enableAsyncHydration;
  option.enable_vsync_aligned_msg_loop_ = _enableVSyncAlignedMessageLoop;
  if (_runtime) {
    option.instance_id_ = _runtime.runtimeActor->GetInstanceId();
  }
  return option;
}

- (void)setUpRuntimeWithLastInstanceId:(int32_t)lastInstanceId {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxTemplateRender::setUpRuntime");

  [self setUpLynxContextWithLastInstanceId:lastInstanceId];

  auto module_manager = [self setUpModuleManager];

  // Attach runtime
  if (_runtime) {
    shell_->AttachRuntime(module_manager);
    const auto& actor = _runtime.runtimeActor;
    auto js_proxy = lynx::shell::JSProxyDarwin::Create(actor, _lynxView, actor->GetInstanceId(),
                                                       [_runtimeOptions groupThreadName]);
    [_context setJSProxy:js_proxy];
    return;
  }

  // Resource loader
  id<LynxTemplateResourceFetcher> templateResourceFetcher =
      [_lynxUIRenderer templateResourceFetcher];
  id<LynxGenericResourceFetcher> genericResourceFetcher = [_lynxUIRenderer genericResourceFetcher];
  auto resource_loader = std::make_shared<lynx::shell::LynxResourceLoaderDarwin>(
      _providerRegistry, _fetcher, self, templateResourceFetcher, genericResourceFetcher);

  __weak typeof(self) weakSelf = self;
  auto on_runtime_actor_created =
      [&weakSelf, &module_manager, lynx_ui_renderer = _lynxUIRenderer, lynx_view = _lynxView,
       context = _context, js_group_thread_name = [_runtimeOptions groupThreadName]](auto& actor) {
        std::shared_ptr<lynx::piper::ModuleDelegate> module_delegate =
            std::make_shared<lynx::shell::ModuleDelegateImpl>(actor);
        module_manager->initBindingPtr(module_manager, module_delegate);

        auto js_proxy = lynx::shell::JSProxyDarwin::Create(
            actor, lynx_view, actor->Impl()->GetRuntimeId(), std::move(js_group_thread_name));
        [context setJSProxy:js_proxy];

        __strong LynxTemplateRender* strongSelf = weakSelf;
        [lynx_ui_renderer onSetupUIDelegate:strongSelf->shell_.get()
                          withModuleManager:module_manager.get()
                                withJSProxy:std::move(js_proxy)];
      };

  // Init Runtime
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxTemplateRender::setUpRuntime:InitRuntime");
  // TODO(liyanbo): refactor this interface.
  shell_->InitRuntime([_runtimeOptions groupID], resource_loader, module_manager,
                      std::move(on_runtime_actor_created), [_runtimeOptions preloadJSPath], false,
                      _runtimeOptions.backgroundJsRuntimeType == LynxBackgroundJsRuntimeTypeQuickjs,
                      _enablePendingJSTaskOnLayout, _runtimeOptions.enableBytecode,
                      [_runtimeOptions bytecodeUrlString]);
}

- (void)setUpLynxContextWithLastInstanceId:(int32_t)lastInstanceId {
  _context = [[LynxContext alloc] initWithLynxView:_lynxView];
  _context.instanceId = shell_->GetInstanceId();
  [_lynxUIRenderer setLynxContext:_context];
  [LynxEventReporter moveExtraParams:lastInstanceId toInstanceId:_context.instanceId];
  [LynxEventReporter updateGenericInfo:@(_threadStrategyForRendering)
                                   key:kPropThreadMode
                            instanceId:_context.instanceId];
  // TODO(chenyouhui): Move this function call to a more appropriate place.
  [LynxService(LynxServiceExtensionProtocol) onLynxViewSetup:_context
                                                       group:_runtimeOptions.group
                                                      config:_config];
}

- (lynx::piper::ModuleFactoryDarwin*)getModuleFactory {
  auto manager = module_manager_.lock();
  if (manager) {
    return static_cast<lynx::piper::ModuleFactoryDarwin*>(manager->GetPlatformModuleFactory());
  }
  return nullptr;
}

- (std::shared_ptr<lynx::piper::LynxModuleManager>)setUpModuleManager {
  std::shared_ptr<lynx::piper::LynxModuleManager> module_manager;
  lynx::piper::ModuleFactoryDarwin* module_factory = nullptr;
  if (_runtime) {
    module_manager = [_runtime moduleManagerPtr].lock();
    if (module_manager) {
      module_factory = static_cast<lynx::piper::ModuleFactoryDarwin*>(
          module_manager->GetPlatformModuleFactory());
      // Merge NativeModules
      module_factory->addModuleParamWrapperIfAbsent(_config.moduleFactoryPtr->getModuleClasses());
    } else {
      _LogE(@"RuntimeStandalone's module_manager shouldn't be null!");
    }
  }
  if (!module_manager) {
    module_manager = std::make_shared<lynx::piper::LynxModuleManager>();
    auto factory = std::make_unique<lynx::piper::ModuleFactoryDarwin>();
    module_factory = factory.get();
    module_manager->SetPlatformModuleFactory(std::move(factory));
    if (_config) {
      TRACE_EVENT(LYNX_TRACE_CATEGORY, "ModuleManager::addWrappers");
      module_factory->addWrappers(_config.moduleFactoryPtr->moduleWrappers());
    }
  }
  module_manager_ = module_manager;

  LynxConfig* globalConfig = [LynxEnv sharedInstance].config;
  if (_config != globalConfig && globalConfig) {
    module_factory->parent = globalConfig.moduleFactoryPtr;
  }
  module_factory->context = _context;

  module_factory->lynxModuleExtraData_ = _lynxModuleExtraData;

  // register auth module blocks
  for (LynxMethodBlock methodAuth in _config.moduleFactoryPtr->methodAuthWrappers()) {
    module_factory->registerMethodAuth(methodAuth);
  }

  // register piper session info block
  for (LynxMethodSessionBlock methodSessionBlock in _config.moduleFactoryPtr
           ->methodSessionWrappers()) {
    module_factory->registerMethodSession(methodSessionBlock);
  }

  if (_extra == nil) {
    _extra = [[NSMutableDictionary alloc] init];
  }
  [_extra addEntriesFromDictionary:[module_factory->extraWrappers() copy]];

  [self setUpBuiltModuleWithFactory:module_factory];
  [self setUpLepusModulesWithFactory:module_factory];
  [self setUpExtensionModules];

  return module_manager;
}

- (void)setUpBuiltModuleWithFactory:(lynx::piper::ModuleFactoryDarwin*)module_factory {
  // register built in module
  module_factory->registerModule(LynxIntersectionObserverModule.class);
  module_factory->registerModule(LynxUIMethodModule.class);
  module_factory->registerModule(LynxTextInfoModule.class);
  module_factory->registerModule(LynxResourceModule.class);
  module_factory->registerModule(LynxAccessibilityModule.class);
  module_factory->registerModule(LynxExposureModule.class);
  module_factory->registerModule(LynxFetchModule.class);
  module_factory->registerModule(LynxSetModule.class);
  [_devTool registerModule:self];
}

- (void)setUpLepusModulesWithFactory:(lynx::piper::ModuleFactoryDarwin*)module_factory {
  self.lepusModulesClasses = [NSMutableDictionary new];
  if (module_factory->parent) {
    [self.lepusModulesClasses addEntriesFromDictionary:module_factory->parent->modulesClasses_];
  }
  [self.lepusModulesClasses addEntriesFromDictionary:module_factory->modulesClasses_];
}

- (void)setUpExtensionModules {
  NSDictionary* modules = _context.extentionModules;
  for (NSString* key in modules) {
    id<LynxExtensionModule> instance = modules[key];
    auto* extension_delegate =
        reinterpret_cast<lynx::pub::LynxExtensionDelegate*>([instance getExtensionDelegate]);
    shell_->RegisterModuleFactory(extension_delegate->CreateModuleFactory());
    shell_->RegisterRuntimeLifecycleObserver(
        extension_delegate->GetRuntimeLifecycleObserver(),
        [](fml::RefPtr<fml::TaskRunner> js_runner) {} /* TODO(chenyouhui) remove this param*/);
    extension_delegate->SetRuntimeTaskRunner(shell_->GetRunners()->GetJSTaskRunner());
    [instance setUp];
  }
}

- (void)setUpFrame:(CGRect)frame {
  // update viewport when preset width and height
  if ((!CGRectEqualToRect(frame, CGRectZero) && !CGSizeEqualToSize(frame.size, CGSizeZero))) {
    _layoutWidthMode = LynxViewSizeModeExact;
    _layoutHeightMode = LynxViewSizeModeExact;
    _preferredLayoutWidth = frame.size.width;
    _preferredLayoutHeight = frame.size.height;
    [self updateViewport];
  }
  _frameOfLynxView = frame;
  if (_lynxView && !CGRectEqualToRect(_lynxView.frame, _frameOfLynxView) &&
      !CGRectEqualToRect(CGRectZero, _frameOfLynxView)) {
    _lynxView.frame = _frameOfLynxView;
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
  [_lynxUIRenderer reset];

  shell_->ClearAllTimingInfo();
  // remove generic info
  [LynxEventReporter removeGenericInfo:_context.instanceId];
  int32_t lastInstanceId = _context.instanceId;
  _context.instanceId = kUnknownInstanceId;
  timing_collector_platform_impl_.reset();
  shell_->Destroy();

  if ([_delegate respondsToSelector:@selector(templateRenderOnTransitionUnregister:)]) {
    [_delegate templateRenderOnTransitionUnregister:self];
  }

  if (_shadowNodeOwner) {
    [_shadowNodeOwner destroySelf];
  }

  [self setUpShadowNodeOwner];
  [self setUpUIDelegate];
  [self setUpLynxShellWithLastInstanceId:lastInstanceId];

  [self setUpEventHandler];
  [self updateViewport];
  [self setUpTiming];
}

- (void)clearForDestroy {
  [_lynxUIRenderer reset];

  [LynxEventReporter clearCacheForInstanceId:_context.instanceId];
  _context.instanceId = kUnknownInstanceId;
  timing_collector_platform_impl_.reset();
  shell_->Destroy();
}

- (void)dealloc {
  [_lynxUIRenderer reset];
  pageConfig_.reset();
  timing_collector_platform_impl_.reset();
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
  [self internalLoadTemplate:tem withUrl:url initData:data];
}

- (void)loadTemplateFromURL:(NSString*)url initData:(LynxTemplateData*)data {
  if (nil == url) {
    _LogE(@"LynxTemplateRender loadTemplateFromURL url is empty! in render %p", self);
    return;
  }
  [self onLoadFromURL:url initData:data];
  [self dispatchViewDidStartLoading];
  __weak LynxTemplateRender* weakSelf = self;

  _LogI(@"LynxTemplateRender loadTemplate url after process is %@", url);
  [weakSelf markTiming:kTimingPrepareTemplateStart pipelineID:nil];
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
                 [weakSelf loadTemplateBundle:templateRes.bundle withURL:url initData:data];
               } else if (templateRes.data) {
                 [weakSelf.devTool onTemplateLoadSuccess:templateRes.data];
                 [weakSelf internalLoadTemplate:templateRes.data withUrl:url initData:data];
               }
             } else {
               [weakSelf onFetchTemplateError:error];
             }
           }];
  } else {
    _LogI(@"loadTemplateFromURL with legacy templateProvider.");
    LynxTemplateLoadBlock complete = ^(id tem, NSError* error) {
      [weakSelf markTiming:kTimingPrepareTemplateEnd pipelineID:nil];
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
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxTemplateRender::loadTemplateBundle", "url",
              [url UTF8String]);
  [self updateUrl:url];
  [self dispatchViewDidStartLoading];
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
          TRACE_EVENT(LYNX_TRACE_CATEGORY, "CreateTemplateData");
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
        [self->_devTool attachDebugBridge:url];
        self->shell_->LoadTemplateBundle(lynx::base::SafeStringConvert([url UTF8String]),
                                         std::move(copied_bundle), ptr, _enablePrePainting,
                                         _enableDumpElement);
        _hasStartedLoad = YES;
      }
      withErrorCallback:^(NSString* msg, NSString* stack) {
        __strong LynxTemplateRender* strongSelf = weakSelf;
        [strongSelf reportError:ECLynxAppBundleLoadException withMsg:msg rawStack:stack];
      }];
}

- (void)internalLoadTemplate:(NSData*)tem withUrl:(NSString*)url initData:(LynxTemplateData*)data {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxTemplateRender::internalLoadTemplate", "url",
              [url UTF8String]);
  __weak LynxTemplateRender* weakSelf = self;
  [self
      executeNativeOpSafely:^() {
        [self prepareForLoadTemplateWithUrl:url initData:data];
        lynx::lepus::Value value;
        std::shared_ptr<lynx::tasm::TemplateData> ptr(nullptr);
        if (data != nil) {
          TRACE_EVENT(LYNX_TRACE_CATEGORY, "CreateTemplateData");
          value = *LynxGetLepusValueFromTemplateData(data);
          ptr = std::make_shared<lynx::tasm::TemplateData>(
              value, data.isReadOnly, data.processorName ? data.processorName.UTF8String : "");
          ptr->SetPlatformData(std::make_unique<lynx::tasm::PlatformDataDarwin>(data));
        }
        auto securityService = LynxService(LynxServiceSecurityProtocol);
        if (securityService == nil) {
          [self->_devTool attachDebugBridge:url];
          // if securityService is nil, Skip Security Check.
          self->shell_->LoadTemplate([url UTF8String], ConvertNSBinary(tem), ptr,
                                     _enablePrePainting, _enableRecycleTemplateBundle);
          _hasStartedLoad = YES;
        } else {
          LynxVerificationResult* verification = [securityService verifyTASM:tem
                                                                        view:_lynxView
                                                                         url:url
                                                                        type:LynxTASMTypeTemplate];
          if (verification.verified) {
            [self->_devTool attachDebugBridge:url];
            self->shell_->LoadTemplate([url UTF8String], ConvertNSBinary(tem), ptr,
                                       _enablePrePainting, _enableRecycleTemplateBundle);
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
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxService::reportErrorGlobalContextTag");
    NSString* finalSchema = [self formatLynxSchema:url];
    [LynxService(LynxServiceMonitorProtocol) reportErrorGlobalContextTag:LynxContextTagLastLynxURL
                                                                    data:finalSchema];
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
  TRACE_EVENT_INSTANT(LYNX_TRACE_CATEGORY_VITALS, "StartLoad");
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
      if (globalProps == nil) {
        self->shell_->ReloadTemplate(template_data);
      } else {
        self->_globalProps = globalProps;
        auto props_value = LynxGetLepusValueFromTemplateData(globalProps);
        self->shell_->ReloadTemplate(
            template_data,
            props_value ? *props_value : lynx::lepus::Value(lynx::lepus::Dictionary::Create()));
      }
    }];
  }
}

#pragma mark - SSR

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
        [strongSelf.lynxSSRHelper onLoadSSRDataBegan:url];
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

- (void)ssrHydrate:(nonnull NSData*)tem
           withURL:(nonnull NSString*)url
          initData:(nullable LynxTemplateData*)data {
  if ([_lynxSSRHelper isHydratePending]) {
    _hasStartedLoad = NO;
    [_lynxSSRHelper onHydrateBegan:url];
  }

  [self loadTemplate:tem withURL:url initData:data];
}

- (void)ssrHydrateFromURL:(NSString*)url initData:(nullable LynxTemplateData*)data {
  if ([_lynxSSRHelper isHydratePending]) {
    _hasStartedLoad = NO;
    [_lynxSSRHelper onHydrateBegan:url];
  }

  [self loadTemplateFromURL:url initData:data];
}

#pragma mark - Global Props

- (void)updateGlobalPropsWithDictionary:(NSDictionary<NSString*, id>*)data {
  LYNX_TRACE_SECTION_WITH_INFO(LYNX_TRACE_CATEGORY_WRAPPER, @"TemplateRender::updateGlobalProps",
                               data);
  if (data.count > 0) {
    [self updateGlobalPropsWithTemplateData:[[LynxTemplateData alloc] initWithDictionary:data]];
  }
  LYNX_TRACE_END_SECTION(LYNX_TRACE_CATEGORY_WRAPPER);
}

- (void)updateGlobalPropsWithTemplateData:(LynxTemplateData*)data {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxTemplateRender::updateGlobalProps");
  if (data) {
    if (!_globalProps) {
      _globalProps = [[LynxTemplateData alloc] initWithDictionary:[NSDictionary new]];
    }
    [_globalProps updateWithTemplateData:data];
    [self updateNativeGlobalProps];
  }
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

#pragma mark - Life Cycle

- (void)dispatchViewDidStartLoading {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxTemplateRender::dispatchViewDidStartLoading");
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
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxTemplateRender::updateViewport");
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
  if (_lynxView) {
    _lynxView = nil;
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

- (LynxSSRHelper*)lynxSSRHelper {
  if (!_lynxSSRHelper) {
    _lynxSSRHelper = [[LynxSSRHelper alloc] init];
  }
  return _lynxSSRHelper;
}

- (LynxUIOwner*)uiOwner {
  return [_lynxUIRenderer uiOwner];
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
  if (_lynxView != nil) {
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
  std::function<void(void)> native_task = [task]() { task(); };
  shell_->RunOnTasmThread(std::move(native_task));
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
    if (_delegate) {
      [_delegate templateRenderOnResetViewAndLayer:self];
    }

    [LynxEventReporter clearCacheForInstanceId:_context.instanceId];
    _context.instanceId = kUnknownInstanceId;
    timing_collector_platform_impl_.reset();
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
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxTemplateRender::setExtraTiming");
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

- (void)putExtraParamsForReportingEvents:(NSDictionary<NSString*, id>*)params {
  [LynxEventReporter putExtraParams:params forInstanceId:self.instanceId];
}

- (nullable NSDictionary*)getAllTimingInfo {
  return lynx::tasm::convertLepusValueToNSObject(shell_->GetAllTimingInfo());
}

- (void)setTiming:(uint64_t)timestamp key:(NSString*)key pipelineID:(nullable NSString*)pipelineID {
  if (!timing_collector_platform_impl_) {
    return;
  }
  std::string keyStr = std::string([key UTF8String]);
  std::string pipelineIDStr = pipelineID ? std::string([pipelineID UTF8String]) : std::string();
  timing_collector_platform_impl_->SetTiming(pipelineIDStr, keyStr, timestamp);
}

- (void)markTiming:(NSString*)key pipelineID:(nullable NSString*)pipelineID {
  if (!timing_collector_platform_impl_) {
    return;
  }
  std::string keyStr = std::string([key UTF8String]);
  std::string pipelineIDStr = pipelineID ? std::string([pipelineID UTF8String]) : std::string();
  timing_collector_platform_impl_->MarkTiming(pipelineIDStr, keyStr);
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
  NSString* error_msg = @"Error occurred when fetch template resource";
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

#pragma mark - Preload

- (void)attachLynxView:(LynxView* _Nonnull)lynxView {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxTemplateRender::attachLynxView");
  _lynxView = lynxView;
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
    [_lynxUIRenderer attachLynxView:lynxView];
  }

  if (_devTool) {
    [_devTool attachLynxView:lynxView];
  }
}

- (BOOL)processRender:(LynxView* _Nonnull)lynxView {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxTemplateRender::processRender", "instanceId",
              [self instanceId]);
  if (shell_ == nullptr || shell_->IsDestroyed()) {
    return NO;
  }

  if (_lynxView == nil) {
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
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxTemplateRender::processLayout");
  [self setNeedPendingUIOperation:YES];
  [self loadTemplate:tem withURL:url initData:data];
}

- (void)processLayoutWithTemplateBundle:(LynxTemplateBundle*)bundle
                                withURL:(NSString*)url
                               initData:(LynxTemplateData*)data {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxTemplateRender::processLayoutWithTemplateBundle");
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
}

- (void)onTasmFinishByNative {
  [_delegate templateRenderOnTasmFinishByNative:self];
}

- (void)onTemplateLoaded:(NSString*)url {
  [_delegate templateRender:self onTemplateLoaded:url];
  [_devTool onLoadFinished];
}

- (void)onSSRHydrateFinished:(NSString*)url {
  [_lynxSSRHelper onHydrateFinished:url];
}

- (void)onRuntimeReady {
  [_delegate templateRenderOnRuntimeReady:self];
}

- (void)onTimingSetup:(NSDictionary*)timingInfo {
  [[_lynxView getLifecycleDispatcher] lynxView:_lynxView onSetup:timingInfo];
}

- (void)onTimingUpdate:(NSDictionary*)timingInfo updateTiming:(NSDictionary*)updateTiming {
  [[_lynxView getLifecycleDispatcher] lynxView:_lynxView onUpdate:timingInfo timing:updateTiming];
}

- (void)onPerformanceEvent:(NSDictionary*)originDict {
  [[_lynxView getLifecycleDispatcher]
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

@end
