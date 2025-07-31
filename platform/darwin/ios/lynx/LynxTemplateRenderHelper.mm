// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxTemplateRenderHelper.h>

#import <Lynx/LynxAccessibilityModule.h>
#import <Lynx/LynxConfig+Internal.h>
#import <Lynx/LynxContext+Internal.h>
#import <Lynx/LynxDevtool+Internal.h>
#import <Lynx/LynxEngine.h>
#import <Lynx/LynxEngineProxy+Native.h>
#import <Lynx/LynxEngineProxy.h>
#import <Lynx/LynxEnv+Internal.h>
#import <Lynx/LynxEnv.h>
#import <Lynx/LynxEventReporter.h>
#import <Lynx/LynxEventReporterUtils.h>
#import <Lynx/LynxExposureModule.h>
#import <Lynx/LynxFetchModule.h>
#import <Lynx/LynxGroup+Internal.h>
#import <Lynx/LynxHttpStreamingDelegate.h>
#import <Lynx/LynxIntersectionObserverModule.h>
#import <Lynx/LynxLog.h>
#import <Lynx/LynxProviderRegistry.h>
#import <Lynx/LynxResourceModule.h>
#import <Lynx/LynxService.h>
#import <Lynx/LynxServiceExtensionProtocol.h>
#import <Lynx/LynxSetModule.h>
#import <Lynx/LynxSubErrorCode.h>
#import <Lynx/LynxTemplateData+Converter.h>
#import <Lynx/LynxTemplateRender+Protected.h>
#import <Lynx/LynxTextInfoModule.h>
#import <Lynx/LynxTraceEventDef.h>
#import <Lynx/LynxUILayoutTick.h>
#import <Lynx/LynxUIMethodModule.h>
#import <Lynx/LynxUIRenderer.h>
#import <Lynx/LynxViewBuilder+Internal.h>
#import <Lynx/PaintingContextProxy.h>

#include "core/base/darwin/lynx_env_darwin.h"
#include "core/public/lynx_extension_delegate.h"
#include "core/renderer/lynx_global_pool.h"
#include "core/renderer/ui_wrapper/painting/ios/painting_context_darwin.h"
#include "core/resource/lynx_resource_loader_darwin.h"
#include "core/services/performance/darwin/performance_controller_darwin.h"
#include "core/shell/ios/js_proxy_darwin.h"
#include "core/shell/ios/lynx_engine_proxy_darwin.h"
#include "core/shell/ios/native_facade_darwin.h"
#include "core/shell/ios/tasm_platform_invoker_darwin.h"
#include "core/shell/lynx_shell_builder.h"
#include "core/shell/module_delegate_impl.h"

@implementation LynxTemplateRender (Helper)

- (void)setUpShadowNodeOwner {
  // FIXME(huangweiwu): fix layout tick both android.
  if (!_uilayoutTick) {
    __weak typeof(self) weakSelf = self;
    _uilayoutTick = [[LynxUILayoutTick alloc] initWithRoot:_containerView
                                                     block:^() {
                                                       __strong LynxTemplateRender* strongSelf =
                                                           weakSelf;
                                                       strongSelf->shell_->TriggerLayout();
                                                     }];
  }
  if (!_isEngineInitFromReusePool) {
    BOOL isAsyncLayout = _threadStrategyForRendering != LynxThreadStrategyForRenderAllOnUI;
    _shadowNodeOwner = [[LynxShadowNodeOwner alloc] initWithUIOwner:[_lynxUIRenderer uiOwner]
                                                         layoutTick:_uilayoutTick
                                                      isAsyncLayout:isAsyncLayout];
  } else {
    [_shadowNodeOwner setLayoutTick:_uilayoutTick];
  }
}

- (void)setUpUIDelegate {
  if (!_isEngineInitFromReusePool) {
    lynx::tasm::UIDelegateDarwin* ui_delegate = new lynx::tasm::UIDelegateDarwin(
        [_lynxUIRenderer uiOwner], [[LynxEnv sharedInstance] enableCreateUIAsync],
        _shadowNodeOwner);
    [_lynxUIRenderer onSetupUIDelegate:ui_delegate];
  }
}

- (void)setUpLynxShellWithLastInstanceId:(int32_t)lastInstanceId {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_RENDER_SETUP_SHELL);

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

  // Build shell
  auto ui_delegate = [_lynxUIRenderer uiDelegate];
  std::unique_ptr<lynx::tasm::PaintingCtxPlatformImpl> painting_context;
  if (!_isEngineInitFromReusePool) {
    painting_context = ui_delegate->CreatePaintingContext();
    if ([_lynxUIRenderer needPaintingContextProxy]) {
      painting_context = ui_delegate->CreatePaintingContext();
      _paintingContextProxy = [[PaintingContextProxy alloc]
          initWithPaintingContext:reinterpret_cast<lynx::tasm::PaintingContextDarwin*>(
                                      painting_context.get())];
      [_shadowNodeOwner setDelegate:_paintingContextProxy];
      auto* painting_context_ref = reinterpret_cast<lynx::tasm::PaintingContextDarwinRef*>(
          painting_context->GetPlatformRef().get());
      if (_embeddedMode == LynxEmbeddedModeUnset) {
        _performanceController =
            [[LynxPerformanceController alloc] initWithObserver:[self getLifecycleDispatcher]];
        painting_context_ref->SetPerformanceController(_performanceController);
      }
    }
  }

  shell_.reset(
      lynx::shell::LynxShellBuilder()
          .SetNativeFacade(std::make_unique<lynx::shell::NativeFacadeDarwin>(self))
          .SetPaintingContextPlatformImpl(std::move(painting_context))
          .SetLynxEnvConfig(lynx_env_config)
          .SetEnableElementManagerVsyncMonitor(true)
          .SetEnableLayoutOnly(_enableLayoutOnly)
          .SetWhiteBoard(_runtimeOptions.group ? _runtimeOptions.group.whiteBoard : nullptr)
          .SetLazyBundleLoader(loader)
          .SetEnableUnifiedPipeline(_enableUnifiedPipeline)
          .SetTasmLocale(std::string([[[LynxEnv sharedInstance] locale] UTF8String]))
          .SetEnablePreUpdateData(_enablePreUpdateData)
          .SetLayoutContextPlatformImpl(
              _isEngineInitFromReusePool ? nullptr : ui_delegate->CreateLayoutContext())
          .SetStrategy(
              static_cast<lynx::base::ThreadStrategyForRendering>(_threadStrategyForRendering))
          .SetEngineActor([loader, lynxEngineProxy = _lynxEngineProxy](auto& actor) {
            loader->SetEngineActor(actor);
            [lynxEngineProxy
                setNativeEngineProxy:std::make_shared<lynx::shell::LynxEngineProxyDarwin>(actor)];
          })
          .SetPropBundleCreator(ui_delegate->CreatePropBundleCreator())
          .SetRuntimeActor(_runtime ? _runtime.runtimeActor : nullptr)
          .SetPerfControllerActor(_runtime ? _runtime.perfControllerActor : nullptr)
          .SetPerformanceControllerPlatform(
              _performanceController
                  ? std::make_unique<lynx::tasm::performance::PerformanceControllerDarwin>(
                        _performanceController)
                  : nullptr)
          .SetShellOption([self setUpShellOption])
          .SetTasmPlatformInvoker(std::make_unique<lynx::shell::TasmPlatformInvokerDarwin>(self))
          .SetUseInvokeUIMethodFunction(_lynxUIRenderer.useInvokeUIMethodFunction)
          .SetLynxEngineWrapper(_lynxEngine ? [_lynxEngine getEngineNative] : nullptr)
          .build());

  [_devTool onTemplateAssemblerCreated:(intptr_t)shell_.get()];

  // Runtime
  if (_embeddedMode == LynxEmbeddedModeUnset) {
    [self setUpRuntimeWithLastInstanceId:lastInstanceId];
  }

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

- (void)setUpEventHandler {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_RENDER_SETUP_EVENT_HANDLER);
  [_lynxUIRenderer setupEventHandler:self
                         engineProxy:_lynxEngineProxy
                       containerView:_containerView
                             context:_context
                            shellPtr:reinterpret_cast<int64_t>(shell_.get())];
}

- (void)setUpRuntimeWithLastInstanceId:(int32_t)lastInstanceId {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_RENDER_SETUP_RUNTIME);

  [self setUpLynxContextWithLastInstanceId:lastInstanceId];

  auto module_manager = [self setUpModuleManager];

  // Attach runtime
  if (_runtime) {
    shell_->AttachRuntime(module_manager);
    const auto& actor = _runtime.runtimeActor;
    auto js_proxy = lynx::shell::JSProxyDarwin::Create(actor, self, actor->GetInstanceId(),
                                                       [_runtimeOptions groupThreadName]);
    [_context setJSProxy:js_proxy];
    [self setUpExtensionModules];
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
      [&weakSelf, &module_manager, lynx_ui_renderer = _lynxUIRenderer, context = _context,
       js_group_thread_name = [_runtimeOptions groupThreadName]](auto& actor) {
        std::shared_ptr<lynx::piper::ModuleDelegate> module_delegate =
            std::make_shared<lynx::shell::ModuleDelegateImpl>(actor);
        module_manager->initBindingPtr(module_manager, module_delegate);

        auto js_proxy = lynx::shell::JSProxyDarwin::Create(
            actor, weakSelf, actor->Impl()->GetRuntimeId(), std::move(js_group_thread_name));
        [context setJSProxy:js_proxy];

        __strong LynxTemplateRender* strongSelf = weakSelf;
        [lynx_ui_renderer onSetupUIDelegate:strongSelf->shell_.get()
                          withModuleManager:module_manager.get()
                                withJSProxy:std::move(js_proxy)];
      };

  // Init Runtime
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_RENDER_INIT_RUNTIME);
  auto runtime_flags = lynx::runtime::CalcRuntimeFlags(
      false, _runtimeOptions.backgroundJsRuntimeType == LynxBackgroundJsRuntimeTypeQuickjs,
      _enablePendingJSTaskOnLayout, _runtimeOptions.enableBytecode);
  shell_->InitRuntime([_runtimeOptions groupID], resource_loader, module_manager,
                      std::move(on_runtime_actor_created), [_runtimeOptions preloadJSPath],
                      runtime_flags, [_runtimeOptions bytecodeUrlString]);
  [self setUpExtensionModules];
}

- (void)setUpExtensionModules {
  if (!_enableJSRuntime) {
    return;
  }
  NSDictionary* modules = _context.extentionModules;
  for (NSString* key in modules) {
    id<LynxExtensionModule> instance = modules[key];
    auto* extension_delegate =
        reinterpret_cast<lynx::pub::LynxExtensionDelegate*>([instance getExtensionDelegate]);
    extension_delegate->SetRuntimeActor(shell_->GetRuntimeActor());
    [instance setUp];
  }
}

- (void)setUpLynxContextWithLastInstanceId:(int32_t)lastInstanceId {
  _context = [[LynxContext alloc] initWithContainerView:_containerView];
  _context.instanceId = shell_->GetInstanceId();
  auto layout_proxy =
      std::make_shared<lynx::shell::LynxLayoutProxyDarwin>(shell_->GetLayoutActor());
  [_context setLayoutProxy:layout_proxy];
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
      TRACE_EVENT(LYNX_TRACE_CATEGORY, MODULE_MANAGER_ADD_WRAPPERS);
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

  return module_manager;
}

- (void)setUpLepusModulesWithFactory:(lynx::piper::ModuleFactoryDarwin*)module_factory {
  self.lepusModulesClasses = [NSMutableDictionary new];
  if (module_factory->parent) {
    [self.lepusModulesClasses addEntriesFromDictionary:module_factory->parent->modulesClasses_];
  }
  [self.lepusModulesClasses addEntriesFromDictionary:module_factory->modulesClasses_];
}

- (void)setUpBuiltModuleWithFactory:(lynx::piper::ModuleFactoryDarwin*)module_factory {
  // register built in module
  module_factory->registerModule(LynxIntersectionObserverModule.class);
  module_factory->registerModule(LynxUIMethodModule.class);
  module_factory->registerModule(LynxTextInfoModule.class);
  module_factory->registerModule(LynxResourceModule.class);
  module_factory->registerModule(LynxAccessibilityModule.class);
  module_factory->registerModule(LynxExposureModule.class);
  LynxFetchModuleEventSender* eventSender = [[LynxFetchModuleEventSender alloc] init];
  eventSender.eventSender = _context;
  module_factory->registerModule(LynxFetchModule.class, eventSender);
  module_factory->registerModule(LynxSetModule.class);
  [_devTool registerModule:self];
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
  option.page_options_.SetInstanceID(option.instance_id_);
  option.page_options_.SetEmbeddedMode(static_cast<lynx::tasm::EmbeddedMode>(_embeddedMode));
  return option;
}

#pragma mark-- Setup

- (void)setUpWithBuilder:(LynxViewBuilder*)builder screenSize:(CGSize)screenSize {
  /// UIRenderer
  [self setUpUIRendererWithBuilder:builder screenSize:screenSize];

  /// LynxShell
  [self setUpLynxShellWithLastInstanceId:kUnknownInstanceId];

  /// Event
  [self setUpEventHandler];
}

- (void)setUpUIRendererWithBuilder:(LynxViewBuilder*)builder screenSize:(CGSize)screenSize {
  [builder.lynxUIRenderer setupWithContainerView:_containerView
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

#pragma mark-- Reset

- (void)reset:(int32_t)lastInstanceId {
  [self setUpShadowNodeOwner];
  [self setUpUIDelegate];
  [self setUpLynxShellWithLastInstanceId:lastInstanceId];
  [self setUpEventHandler];
}

@end
