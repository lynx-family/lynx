// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxTemplateRenderHelper.h>

#import <Lynx/LynxAccessibilityModule.h>
#import <Lynx/LynxConfig+Internal.h>
#import <Lynx/LynxContext+Internal.h>
#import <Lynx/LynxDevtool+Internal.h>
#import <Lynx/LynxEngineProxy+Native.h>
#import <Lynx/LynxEngineProxy.h>
#import <Lynx/LynxEnv+Internal.h>
#import <Lynx/LynxEnv.h>
#import <Lynx/LynxEventReporter.h>
#import <Lynx/LynxEventReporterUtils.h>
#import <Lynx/LynxExposureModule.h>
#import <Lynx/LynxFetchModule.h>
#import <Lynx/LynxFrameRender+Internal.h>
#import <Lynx/LynxFrameRender.h>
#import <Lynx/LynxGroup+Internal.h>
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
#import <Lynx/LynxTemplateRenderContext+Internal.h>
#import <Lynx/LynxTemplateRenderContext.h>
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

@implementation LynxTemplateRenderHelper {
  id<TemplateRenderCallbackProtocol> _render;
  LynxTemplateRenderContext* _context;
}

- (instancetype)initWithRender:(id<TemplateRenderCallbackProtocol>)render
                 renderContext:(LynxTemplateRenderContext*)renderContext {
  if (self = [super init]) {
    _render = render;
    _context = renderContext;
  }
  return self;
}

- (void)setUpShadowNodeOwner {
  if (!_context->_uilayoutTick) {
    _context->_uilayoutTick = [[LynxUILayoutTick alloc] initWithRoot:_context->_containerView
                                                               block:_context->_layoutBlock];
  }

  BOOL isAsyncLayout = _context->_threadStrategyForRendering != LynxThreadStrategyForRenderAllOnUI;
  _context->_shadowNodeOwner =
      [[LynxShadowNodeOwner alloc] initWithUIOwner:[_context->_lynxUIRenderer uiOwner]
                                        layoutTick:_context->_uilayoutTick
                                     isAsyncLayout:isAsyncLayout];
}

- (void)setUpUIDelegate {
  lynx::tasm::UIDelegateDarwin* ui_delegate = new lynx::tasm::UIDelegateDarwin(
      [_context->_lynxUIRenderer uiOwner], [[LynxEnv sharedInstance] enableCreateUIAsync],
      _context -> _shadowNodeOwner);
  [_context->_lynxUIRenderer onSetupUIDelegate:ui_delegate];
}

- (void)setUpLynxShellWithLastInstanceId:(int32_t)lastInstanceId {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_RENDER_SETUP_SHELL);

  // Env
  lynx::tasm::LynxEnvDarwin::initNativeUIThread();
  LynxScreenMetrics* screenMetrics = [_context->_lynxUIRenderer getScreenMetrics];
  auto lynx_env_config = lynx::tasm::LynxEnvConfig(
      screenMetrics.screenSize.width, screenMetrics.screenSize.height, 1.f, screenMetrics.scale);

  // Resource Loader
  id<LynxTemplateResourceFetcher> templateResourceFetcher =
      [_context->_lynxUIRenderer templateResourceFetcher];
  id<LynxGenericResourceFetcher> genericResourceFetcher =
      [_context->_lynxUIRenderer genericResourceFetcher];
  auto loader = std::make_shared<lynx::tasm::LazyBundleLoader>(
      std::make_shared<lynx::shell::LynxResourceLoaderDarwin>(
          nil, _context->_fetcher, _render, templateResourceFetcher, genericResourceFetcher));

  // Build shell
  auto ui_delegate = [_context->_lynxUIRenderer uiDelegate];
  auto painting_context = ui_delegate->CreatePaintingContext();
  if ([_context->_lynxUIRenderer needPaintingContextProxy]) {
    _context->_paintingContextProxy = [[PaintingContextProxy alloc]
        initWithPaintingContext:reinterpret_cast<lynx::tasm::PaintingContextDarwin*>(
                                    painting_context.get())];
    [_context->_shadowNodeOwner setDelegate:_context->_paintingContextProxy];
  }

  _context->_performanceController =
      [[LynxPerformanceController alloc] initWithObserver:_context->_lifecycleDispatcher];
  auto* a = reinterpret_cast<lynx::tasm::PaintingContextDarwinRef*>(
      painting_context->GetPlatformRef().get());
  a->SetPerformanceController(_context->_performanceController);

  _context->shell_.reset(
      lynx::shell::LynxShellBuilder()
          .SetNativeFacade(std::make_unique<lynx::shell::NativeFacadeDarwin>(_render))
          .SetPaintingContextPlatformImpl(std::move(painting_context))
          .SetLynxEnvConfig(lynx_env_config)
          .SetEnableElementManagerVsyncMonitor(true)
          .SetEnableLayoutOnly(_context->_enableLayoutOnly)
          .SetWhiteBoard(_context->_runtimeOptions.group
                             ? _context->_runtimeOptions.group.whiteBoard
                             : nullptr)
          .SetLazyBundleLoader(loader)
          .SetEnableUnifiedPipeline(_context->_enableUnifiedPipeline)
          .SetTasmLocale(std::string([[[LynxEnv sharedInstance] locale] UTF8String]))
          .SetEnablePreUpdateData(_context->_enablePreUpdateData)
          .SetLayoutContextPlatformImpl(ui_delegate->CreateLayoutContext())
          .SetStrategy(static_cast<lynx::base::ThreadStrategyForRendering>(
              _context->_threadStrategyForRendering))
          .SetEngineActor([loader, lynxEngineProxy = _context->_lynxEngineProxy](auto& actor) {
            loader->SetEngineActor(actor);
            [lynxEngineProxy
                setNativeEngineProxy:std::make_shared<lynx::shell::LynxEngineProxyDarwin>(actor)];
          })
          .SetPropBundleCreator(ui_delegate->CreatePropBundleCreator())
          .SetRuntimeActor(_context->_runtime ? _context->_runtime.runtimeActor : nullptr)
          .SetPerfControllerActor(_context->_runtime ? _context->_runtime.perfControllerActor
                                                     : nullptr)
          .SetPerformanceControllerPlatform(
              std::make_unique<lynx::tasm::performance::PerformanceControllerDarwin>(
                  _context->_performanceController))
          .SetShellOption([self setUpShellOption])
          .SetTasmPlatformInvoker(std::make_unique<lynx::shell::TasmPlatformInvokerDarwin>(_render))
          .SetUseInvokeUIMethodFunction(_context->_lynxUIRenderer.useInvokeUIMethodFunction)
          .build());

  [_context->_devTool onTemplateAssemblerCreated:(intptr_t)_context->shell_.get()];

  // Runtime
  if (_context->_embeddedMode == UNSET) {
    [self setUpRuntimeWithLastInstanceId:lastInstanceId];
  }

  // reset ui flush flag
  if (!_context->shell_->IsDestroyed()) {
    _context->shell_->SetEnableUIFlush(!_context->_needPendingUIOperation);
  }

  // FIXME
  _context->shell_->SetFontScale(_context->_fontScale);

  // Thread pool
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    lynx::tasm::LynxGlobalPool::GetInstance().PreparePool();
  });
}

- (void)setUpEventHandler {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_RENDER_SETUP_EVENT_HANDLER);
  [_context->_lynxUIRenderer setupEventHandler:_render
                                   engineProxy:_context->_lynxEngineProxy
                                 containerView:_context->_containerView
                                       context:_context->_context
                                      shellPtr:reinterpret_cast<int64_t>(_context->shell_.get())];
}

- (void)setUpRuntimeWithLastInstanceId:(int32_t)lastInstanceId {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_RENDER_SETUP_RUNTIME);

  [self setUpLynxContextWithLastInstanceId:lastInstanceId];

  auto module_manager = [self setUpModuleManager];

  // Attach runtime
  if (_context->_runtime) {
    _context->shell_->AttachRuntime(module_manager);
    const auto& actor = _context->_runtime.runtimeActor;
    auto js_proxy = lynx::shell::JSProxyDarwin::Create(actor, _render, actor->GetInstanceId(),
                                                       [_context->_runtimeOptions groupThreadName]);
    [_context->_context setJSProxy:js_proxy];
    [self setUpExtensionModules];
    return;
  }

  // Resource loader
  id<LynxTemplateResourceFetcher> templateResourceFetcher =
      [_context->_lynxUIRenderer templateResourceFetcher];
  id<LynxGenericResourceFetcher> genericResourceFetcher =
      [_context->_lynxUIRenderer genericResourceFetcher];
  auto resource_loader = std::make_shared<lynx::shell::LynxResourceLoaderDarwin>(
      _context->_providerRegistry, _context->_fetcher, _render, templateResourceFetcher,
      genericResourceFetcher);

  __weak LynxTemplateRenderContext* weakContext = _context;
  __weak id<TemplateRenderCallbackProtocol> weakRender = _render;
  auto on_runtime_actor_created =
      [&weakContext, &weakRender, &module_manager, lynx_ui_renderer = _context->_lynxUIRenderer,
       context = _context->_context,
       js_group_thread_name = [_context->_runtimeOptions groupThreadName]](auto& actor) {
        std::shared_ptr<lynx::piper::ModuleDelegate> module_delegate =
            std::make_shared<lynx::shell::ModuleDelegateImpl>(actor);
        module_manager->initBindingPtr(module_manager, module_delegate);

        auto js_proxy = lynx::shell::JSProxyDarwin::Create(
            actor, weakRender, actor->Impl()->GetRuntimeId(), std::move(js_group_thread_name));
        [context setJSProxy:js_proxy];

        __strong LynxTemplateRenderContext* strongContext = weakContext;
        [lynx_ui_renderer onSetupUIDelegate:strongContext->shell_.get()
                          withModuleManager:module_manager.get()
                                withJSProxy:std::move(js_proxy)];
      };

  // Init Runtime
  TRACE_EVENT(LYNX_TRACE_CATEGORY, TEMPLATE_RENDER_INIT_RUNTIME);
  auto runtime_flags = lynx::runtime::CalcRuntimeFlags(
      false,
      _context->_runtimeOptions.backgroundJsRuntimeType == LynxBackgroundJsRuntimeTypeQuickjs,
      _context->_enablePendingJSTaskOnLayout, _context->_runtimeOptions.enableBytecode);
  _context->shell_->InitRuntime([_context->_runtimeOptions groupID], resource_loader,
                                module_manager, std::move(on_runtime_actor_created),
                                [_context->_runtimeOptions preloadJSPath], runtime_flags,
                                [_context->_runtimeOptions bytecodeUrlString]);
  [self setUpExtensionModules];
}

- (void)setUpExtensionModules {
  if (!_context->_enableJSRuntime) {
    return;
  }
  NSDictionary* modules = _context->_context.extentionModules;
  for (NSString* key in modules) {
    id<LynxExtensionModule> instance = modules[key];
    auto* extension_delegate =
        reinterpret_cast<lynx::pub::LynxExtensionDelegate*>([instance getExtensionDelegate]);
    extension_delegate->SetRuntimeActor(_context->shell_->GetRuntimeActor());
    [instance setUp];
  }
}

- (void)setUpLynxContextWithLastInstanceId:(int32_t)lastInstanceId {
  _context->_context = [[LynxContext alloc] initWithContainerView:_context->_containerView];
  _context->_context.instanceId = _context->shell_->GetInstanceId();
  auto layout_proxy =
      std::make_shared<lynx::shell::LynxLayoutProxyDarwin>(_context->shell_->GetLayoutActor());
  [_context->_context setLayoutProxy:layout_proxy];
  [_context->_lynxUIRenderer setLynxContext:_context->_context];
  [LynxEventReporter moveExtraParams:lastInstanceId toInstanceId:_context->_context.instanceId];
  [LynxEventReporter updateGenericInfo:@(_context->_threadStrategyForRendering)
                                   key:kPropThreadMode
                            instanceId:_context->_context.instanceId];
  // TODO(chenyouhui): Move this function call to a more appropriate place.
  [LynxService(LynxServiceExtensionProtocol) onLynxViewSetup:_context->_context
                                                       group:_context->_runtimeOptions.group
                                                      config:_context->_config];
}

- (std::shared_ptr<lynx::piper::LynxModuleManager>)setUpModuleManager {
  std::shared_ptr<lynx::piper::LynxModuleManager> module_manager;
  lynx::piper::ModuleFactoryDarwin* module_factory = nullptr;
  if (_context->_runtime) {
    module_manager = [_context->_runtime moduleManagerPtr].lock();
    if (module_manager) {
      module_factory = static_cast<lynx::piper::ModuleFactoryDarwin*>(
          module_manager->GetPlatformModuleFactory());
      // Merge NativeModules
      module_factory->addModuleParamWrapperIfAbsent(
          _context->_config.moduleFactoryPtr->getModuleClasses());
    } else {
      _LogE(@"RuntimeStandalone's module_manager shouldn't be null!");
    }
  }
  if (!module_manager) {
    module_manager = std::make_shared<lynx::piper::LynxModuleManager>();
    auto factory = std::make_unique<lynx::piper::ModuleFactoryDarwin>();
    module_factory = factory.get();
    module_manager->SetPlatformModuleFactory(std::move(factory));
    if (_context->_config) {
      TRACE_EVENT(LYNX_TRACE_CATEGORY, MODULE_MANAGER_ADD_WRAPPERS);
      module_factory->addWrappers(_context->_config.moduleFactoryPtr->moduleWrappers());
    }
  }
  _context->module_manager_ = module_manager;

  LynxConfig* globalConfig = [LynxEnv sharedInstance].config;
  if (_context->_config != globalConfig && globalConfig) {
    module_factory->parent = globalConfig.moduleFactoryPtr;
  }
  module_factory->context = _context->_context;

  module_factory->lynxModuleExtraData_ = _context->_lynxModuleExtraData;

  // register auth module blocks
  for (LynxMethodBlock methodAuth in _context->_config.moduleFactoryPtr->methodAuthWrappers()) {
    module_factory->registerMethodAuth(methodAuth);
  }

  // register piper session info block
  for (LynxMethodSessionBlock methodSessionBlock in _context->_config.moduleFactoryPtr
           ->methodSessionWrappers()) {
    module_factory->registerMethodSession(methodSessionBlock);
  }

  if (_context->_extra == nil) {
    _context->_extra = [[NSMutableDictionary alloc] init];
  }
  [_context->_extra addEntriesFromDictionary:[module_factory->extraWrappers() copy]];

  [self setUpBuiltModuleWithFactory:module_factory];
  [self setUpLepusModulesWithFactory:module_factory];

  return module_manager;
}

- (void)setUpLepusModulesWithFactory:(lynx::piper::ModuleFactoryDarwin*)module_factory {
  _context->_lepusModulesClasses = [NSMutableDictionary new];
  if (module_factory->parent) {
    [_context->_lepusModulesClasses
        addEntriesFromDictionary:module_factory->parent->modulesClasses_];
  }
  [_context->_lepusModulesClasses addEntriesFromDictionary:module_factory->modulesClasses_];
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
  if ([_render isKindOfClass:[LynxTemplateRender class]]) {
    [_context->_devTool registerModule:(LynxTemplateRender*)_render];
  }
}

- (lynx::shell::ShellOption)setUpShellOption {
  lynx::shell::ShellOption option;
  option.enable_js_ = _context->_enableJSRuntime;
  option.enable_js_group_thread_ = _context->_enableJSGroupThread;
  if (_context->_enableJSGroupThread) {
    option.js_group_thread_name_ = [_context->_runtimeOptions groupID];
  }
  option.enable_multi_tasm_thread_ =
      _context->_enableMultiAsyncThread ||
      [[LynxEnv sharedInstance] boolFromExternalEnv:LynxEnvEnableMultiTASMThread defaultValue:NO];
  option.enable_multi_layout_thread_ =
      _context->_enableMultiAsyncThread ||
      [[LynxEnv sharedInstance] boolFromExternalEnv:LynxEnvEnableMultiLayoutThread defaultValue:NO];
  option.enable_async_hydration_ = _context->_enableAsyncHydration;
  option.enable_vsync_aligned_msg_loop_ = _context->_enableVSyncAlignedMessageLoop;
  if (_context->_runtime) {
    option.instance_id_ = _context->_runtime.runtimeActor->GetInstanceId();
  }
  option.page_options_.SetInstanceID(option.instance_id_);
  option.page_options_.SetEmbeddedMode(
      static_cast<lynx::tasm::EmbeddedMode>(_context->_embeddedMode));
  return option;
}

#pragma mark-- Setup

+ (void)setUpTemplateRender:(LynxTemplateRender*)render
                    builder:(LynxViewBuilder*)builder
                 screenSize:(CGSize)screenSize {
  LynxTemplateRenderContext* context = [render getRenderContext];
  [[[LynxTemplateRenderHelper alloc] initWithRender:render
                                      renderContext:context] setUpWithBuilder:builder
                                                                   screenSize:screenSize];
  [render attachRenderContext:context];
}

+ (void)setUpFrameRender:(LynxFrameRender*)render
                 builder:(LynxViewBuilder*)builder
              screenSize:(CGSize)screenSize {
  LynxTemplateRenderContext* context = [render getRenderContext];
  [[[LynxTemplateRenderHelper alloc] initWithRender:render
                                      renderContext:context] setUpWithBuilder:builder
                                                                   screenSize:screenSize];
  [render attachRenderContext:context];
}

- (void)setUpWithBuilder:(LynxViewBuilder*)builder screenSize:(CGSize)screenSize {
  /// UIRenderer
  [self setUpUIRendererWithBuilder:builder screenSize:screenSize];

  /// LynxShell
  [self setUpLynxShellWithLastInstanceId:kUnknownInstanceId];

  /// Event
  [self setUpEventHandler];
}

- (void)setUpUIRendererWithBuilder:(LynxViewBuilder*)builder screenSize:(CGSize)screenSize {
  [builder.lynxUIRenderer setupWithContainerView:_context->_containerView
                                         builder:builder
                                      screenSize:screenSize];
  [_context->_devTool attachLynxUIOwner:[builder.lynxUIRenderer uiOwner]];

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
  _context->_providerRegistry = registry;

  [_context->_lynxUIRenderer
      setupResourceProvider:[registry getResourceProviderByKey:LYNX_PROVIDER_TYPE_FONT]
                withBuilder:builder];
}

#pragma mark-- Reset

+ (void)resetTemplateRender:(LynxTemplateRender*)render lastInstanceId:(int32_t)lastInstanceId {
  [[[LynxTemplateRenderHelper alloc] initWithRender:render renderContext:[render getRenderContext]]
      resetWithInstanceId:lastInstanceId];
}

- (void)resetWithInstanceId:(int32_t)lastInstanceId {
  [self setUpShadowNodeOwner];
  [self setUpUIDelegate];
  [self setUpLynxShellWithLastInstanceId:lastInstanceId];
  [self setUpEventHandler];
}

@end
