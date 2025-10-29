// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxBackgroundRuntime.h>
#import <Lynx/LynxDevtool+Internal.h>
#import <Lynx/LynxEnv.h>
#import <Lynx/LynxHttpStreamingDelegate.h>
#import <Lynx/LynxLog.h>
#import <Lynx/LynxProviderRegistry.h>
#import <Lynx/LynxService.h>
#import <Lynx/LynxTraceEvent.h>
#import <Lynx/LynxTraceEventDef.h>
#import <Lynx/LynxViewClient.h>
#import "JSModule+Internal.h"
#import "LynxConfig+Internal.h"
#import "LynxErrorReceiverProtocol.h"
#import "LynxFetchModule.h"
#import "LynxGroup+Internal.h"
#import "LynxTemplateBundle+Converter.h"
#import "LynxTemplateData+Converter.h"

#include "core/inspector/observer/inspector_runtime_observer_ng.h"
#include "core/renderer/dom/ios/lepus_value_converter.h"
#include "core/renderer/ui_wrapper/common/ios/prop_bundle_darwin.h"
#include "core/resource/lynx_resource_loader_darwin.h"
#include "core/shell/ios/js_proxy_darwin.h"
#include "core/shell/ios/lynx_runtime_facade_darwin.h"
#include "core/shell/module_delegate_impl.h"
#include "core/shell/runtime_standalone_helper.h"

@implementation LynxBackgroundRuntimeOptions {
  // Only as a module wrapper container for register.
  std::unique_ptr<lynx::piper::ModuleFactoryDarwin> _moduleWrapperContainer;
  NSMutableDictionary<NSString*, id<LynxResourceProvider>>* _providers;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    _backgroundJsRuntimeType = LynxBackgroundJsRuntimeTypeJSC;
    _enableBytecode = NO;
    _moduleWrapperContainer = std::make_unique<lynx::piper::ModuleFactoryDarwin>();
    _providers = [NSMutableDictionary dictionary];
  }
  return self;
}

- (instancetype)initWithOptions:(LynxBackgroundRuntimeOptions*)other {
  if (self = [self init]) {
    [self merge:other];
    _moduleWrapperContainer->addWrappers([other moduleWrappers]);
    _presetData = other.presetData;
  }
  return self;
}

// There are 2 ways to use this method:
// 1. merge LynxBackgroundRuntimeOptions as `other` into LynxBackgroundRuntime as `this`
// 2. merge LynxBackgroundRuntimeOptions as `other` into LynxViewBuilder as `this`
// for `1` LynxBackgroundRuntime's configurations are not set, any overwrite/assign is safe
// for `2` We need follow the rules described below.
- (void)merge:(LynxBackgroundRuntimeOptions*)other {
  if (!other) {
    return;
  }
  // Merge LynxResourceProviders:
  // This part of providers are deprecated and will be removed later, we won't change this part
  for (NSString* name in other.providers) {
    [self addLynxResourceProvider:name provider:other.providers[name]];
  }

  // Overwrite BackgroundRuntime configurations:
  // This part of configurations are used to create BackgroundRuntime and cannot be modified
  // after it attaches to LynxView. So we use the configurations inside runtime to overwrite
  // LynxViewBuilder
  _group = other.group;
  _backgroundJsRuntimeType = other.backgroundJsRuntimeType;
  _enableBytecode = other.enableBytecode;
  _bytecodeUrl = other.bytecodeUrl;
  _pendingCoreJsLoad = other.pendingCoreJsLoad;

  // Merge these Fetchers only if they are unset:
  // This part of configurations are shared between runtime and platform-level of LynxView.
  // We need to keep the configurations on LynxViewBuilder if Fetchers are set.
  _genericResourceFetcher = _genericResourceFetcher ?: other.genericResourceFetcher;
  _mediaResourceFetcher = _mediaResourceFetcher ?: other.mediaResourceFetcher;
  _templateResourceFetcher = _templateResourceFetcher ?: other.templateResourceFetcher;
}

- (void)addLynxResourceProvider:(NSString*)resType provider:(id<LynxResourceProvider>)provider {
  [_providers setValue:provider forKey:resType];
}

- (void)registerModule:(Class<LynxModule>)module {
  _moduleWrapperContainer->registerModule(module);
}

- (void)registerModule:(Class<LynxModule>)module param:(id)param {
  _moduleWrapperContainer->registerModule(module, param);
}

- (NSMutableDictionary<NSString*, id>*)moduleWrappers {
  return _moduleWrapperContainer->moduleWrappers();
}

- (NSMutableDictionary<NSString*, id<LynxResourceProvider>>*)providers {
  return _providers;
}

- (std::string)groupThreadName {
  return [[LynxGroup jsThreadNameForLynxGroupOrDefault:_group] UTF8String];
}

- (std::string)groupID {
  return [[LynxGroup groupNameForLynxGroupOrDefault:_group] UTF8String];
}

- (BOOL)enableJSGroupThread {
  return _group.enableJSGroupThread;
}

- (std::vector<std::string>)preloadJSPath {
  std::vector<std::string> ret;
  NSArray* preloadJSPaths = [_group preloadJSPaths];
  for (NSString* path : preloadJSPaths) {
    ret.emplace_back([path UTF8String]);
  }
  return ret;
}

- (void)setBackgroundJsRuntimeType:(LynxBackgroundJsRuntimeType)backgroundJsRuntimeType {
  if (backgroundJsRuntimeType != LynxBackgroundJsRuntimeTypeQuickjs &&
      backgroundJsRuntimeType != LynxBackgroundJsRuntimeTypeJSC) {
    _LogE(@"Lynx background js runtime type don't support this type: %ld, use default type:jsc.",
          backgroundJsRuntimeType);
    backgroundJsRuntimeType = LynxBackgroundJsRuntimeTypeJSC;
  }
  _backgroundJsRuntimeType = backgroundJsRuntimeType;
}

- (std::string)bytecodeUrlString {
  return _bytecodeUrl ? [_bytecodeUrl UTF8String] : "";
}

@end

typedef NS_ENUM(NSInteger, LynxBackgroundRuntimeState) {
  // LynxBackgroundRuntime's initial state, under this state, all
  // APIs on it are functional. If user manually calls `destroy`
  // it transits into `STATE_DESTROYED`. If user uses it to build
  // a LynxView, it transits into `STATE_ATTACHED`.
  LynxBackgroundRuntimeStateStart = 1 << 0,
  // LynxBackgroundRuntime is attached to a LynxView, under this state, all
  // APIs on it are frozen and will be ignored. If user wants to send
  // GlobalEvent, API on LynxView should be used.
  LynxBackgroundRuntimeStateAttached = 1 << 2,
};

@interface LynxBackgroundRuntime () <LynxErrorReceiverProtocol>
@property(nonatomic, strong) LynxBackgroundRuntimeOptions* options;
@property(atomic, nullable) NSString* lastScriptUrl;
@end

@implementation LynxBackgroundRuntime {
  NSHashTable<id<LynxBackgroundRuntimeLifecycle>>* _innerLifecycleClients;
  LynxBackgroundRuntimeState _state;
  std::weak_ptr<lynx::piper::ModuleFactoryDarwin> _weak_module_factory;
  std::shared_ptr<lynx::shell::JSProxyDarwin> _js_proxy;
  std::shared_ptr<lynx::piper::InspectorRuntimeObserverNG> _runtime_observer;
  lynx::shell::InitRuntimeStandaloneResult _runtime_standalone_bundle;
  LynxDevtool* _devTool;
}

- (std::weak_ptr<lynx::piper::ModuleFactoryDarwin>)moduleFactoryPtr {
  return _weak_module_factory;
}

- (std::shared_ptr<lynx::shell::LynxActor<lynx::runtime::LynxRuntime>>)runtimeActor {
  return _runtime_standalone_bundle.runtime_actor_;
}

- (std::shared_ptr<lynx::shell::LynxActor<lynx::tasm::performance::PerformanceController>>)
    perfControllerActor {
  return _runtime_standalone_bundle.perf_controller_actor_;
}

- (LynxDevtool*)devtool {
  return _devTool;
}

- (void)setRuntimeObserver:
    (const std::shared_ptr<lynx::piper::InspectorRuntimeObserverNG>&)observer {
  _runtime_observer = observer;
}

- (void)initDevTool:(BOOL)debuggable {
  if (LynxEnv.sharedInstance.lynxDebugEnabled) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnonnull"
    _devTool = [[LynxDevtool alloc] initWithLynxView:nil debuggable:debuggable];
#pragma clang diagnostic pop
    auto group_thread_name = [NSString stringWithUTF8String:[_options groupThreadName].c_str()];
    [_devTool onBackgroundRuntimeCreated:self groupThreadName:group_thread_name];
  }
}

- (instancetype)initWithOptions:(LynxBackgroundRuntimeOptions*)options {
  return [self initWithOptions:options debuggable:NO];
}

- (instancetype)initWithOptions:(LynxBackgroundRuntimeOptions*)options debuggable:(BOOL)debuggable {
  self = [super init];
  if (self) {
    _state = LynxBackgroundRuntimeStateStart;
    _options = [[LynxBackgroundRuntimeOptions alloc] initWithOptions:options];
    _innerLifecycleClients = [NSHashTable hashTableWithOptions:NSPointerFunctionsWeakMemory];
    [self initDevTool:debuggable];

    // create NativeModuleManager
    std::shared_ptr<lynx::pub::LynxNativeModuleManager> native_module_manager =
        std::make_shared<lynx::pub::LynxNativeModuleManager>();
    // create ModuleFactory
    auto module_factory = std::make_shared<lynx::piper::ModuleFactoryDarwin>();
    // cache ModuleFactory with weak_ptr
    _weak_module_factory = module_factory;
    native_module_manager->SetPlatformModuleFactory(module_factory);

    LynxFetchModuleEventSender* eventSender = [[LynxFetchModuleEventSender alloc] init];
    eventSender.eventSender = self;
    module_factory->registerModule(LynxFetchModule.class, eventSender);
    module_factory->addWrappers([options moduleWrappers]);
    LynxConfig* globalConfig = [LynxEnv sharedInstance].config;
    if (globalConfig) {
      module_factory->parent = globalConfig.moduleFactoryPtr;
    }

    LynxProviderRegistry* registry = [[LynxProviderRegistry alloc] init];
    NSDictionary* providers = [LynxEnv sharedInstance].resoureProviders;
    for (NSString* globalKey in providers) {
      [registry addLynxResourceProvider:globalKey provider:providers[globalKey]];
    }

    for (NSString* key in _options.providers) {
      [registry addLynxResourceProvider:key provider:_options.providers[key]];
    }
    auto loader = std::make_shared<lynx::shell::LynxResourceLoaderDarwin>(
        registry, nil, self, _options.templateResourceFetcher, _options.genericResourceFetcher);

    auto on_runtime_actor_created = [native_module_manager](auto& actor, auto facade_actor) {
      std::shared_ptr<lynx::piper::ModuleDelegate> module_delegate =
          std::make_shared<lynx::shell::ModuleDelegateImpl>(actor);
      native_module_manager->SetModuleDelegate(module_delegate);
    };

    auto native_runtime = std::make_unique<lynx::shell::NativeRuntimeFacadeDarwin>(self);

    auto bundle_creator = std::make_shared<lynx::tasm::PropBundleCreatorDarwin>();

    auto group_thread_name = [_options groupThreadName];

    bool enableJSGroupThread = [_options enableJSGroupThread] == YES;
    bool pendingCoreJsLoad = [_options pendingCoreJsLoad] == YES;
    auto runtime_flags = lynx::runtime::CalcRuntimeFlags(
        false, _options.backgroundJsRuntimeType == LynxBackgroundJsRuntimeTypeQuickjs, false,
        _options.enableBytecode, &enableJSGroupThread, &pendingCoreJsLoad);
    _runtime_standalone_bundle = lynx::shell::InitRuntimeStandalone(
        group_thread_name, [_options groupID], std::move(native_runtime), _runtime_observer, loader,
        native_module_manager, bundle_creator, _options.group.whiteBoard,
        std::move(on_runtime_actor_created), [_options preloadJSPath], [_options bytecodeUrlString],
        runtime_flags, nullptr, debuggable, false);

    const auto& runtime_actor = _runtime_standalone_bundle.runtime_actor_;
    _js_proxy = lynx::shell::JSProxyDarwin::Create(
        runtime_actor, self, runtime_actor->Impl()->GetRuntimeId(), group_thread_name, true);

    if (_options.presetData) {
      lynx::lepus::Value presetData = *LynxGetLepusValueFromTemplateData(_options.presetData);

      lynx::lepus::Value data;
      if (_options.presetData.isReadOnly) {
        data = presetData;
      } else {
        data = lynx::lepus::Value::Clone(presetData);
      }
      runtime_actor->Act([data = std::move(data)](auto& runtime) mutable {
        runtime->OnSetPresetData(std::move(data));
      });
    }
  }
  return self;
}

- (void)addLifecycleClient:(nonnull id<LynxBackgroundRuntimeLifecycle>)lifecycleClient {
  if (!lifecycleClient) {
    return;
  }
  @synchronized(_innerLifecycleClients) {
    [_innerLifecycleClients addObject:lifecycleClient];
  }
}

- (void)removeLifecycleClient:(nonnull id<LynxBackgroundRuntimeLifecycle>)lifecycleClient {
  if (!lifecycleClient) {
    return;
  }
  @synchronized(_innerLifecycleClients) {
    [_innerLifecycleClients removeObject:lifecycleClient];
  }
}

- (void)evaluateJavaScript:(NSString*)url withSources:(NSString*)sources {
  if (!url || !sources) {
    _LogE(@"LynxBackgroundRuntime, evaluateJavaScript warning: url or sources is nil: %@, %@", url,
          sources);
    return;
  }

  [_devTool onStandaloneRuntimeLoadFromURL:url];
  [_devTool attachDebugBridge:url];

  _lastScriptUrl = url;

  uint64_t trace_flow_id = TRACE_FLOW_ID();
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, EVALUATE_SCRIPT_STANDALONE,
              [&url, trace_flow_id](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_debug_annotations("url", std::string([url UTF8String]));
                ctx.event()->add_flow_ids(trace_flow_id);
              });
  _runtime_standalone_bundle.runtime_actor_->Act([url = std::string([url UTF8String]),
                                                  sources = std::string([sources UTF8String]),
                                                  trace_flow_id](auto& runtime) mutable {
    runtime->EvaluateScriptStandalone(std::move(url), std::move(sources), trace_flow_id);
  });
}

- (void)evaluateTemplateBundle:(NSString*)url
                   widthBundle:(LynxTemplateBundle*)bundle
                    withJSFile:(NSString*)jsFile {
  if (_state == LynxBackgroundRuntimeStateStart) {
    [_devTool onStandaloneRuntimeLoadFromURL:url];
    [_devTool attachDebugBridge:url];
  }
  auto raw_bundle = LynxGetRawTemplateBundle(bundle).get();
  std::string js_path = [jsFile UTF8String];
  auto js_content = raw_bundle->GetJsBundle().GetJsContent(js_path);
  if (!js_content.has_value()) {
    return;
  }
  auto js_content_val = js_content->get();
  if (js_content_val.IsError()) {
    return;
  }
  auto buffer = js_content_val.GetBuffer();
  if (!buffer || !buffer->data()) {
    return;
  }
  const auto length = buffer->size();
  const auto* data = reinterpret_cast<const char*>(buffer->data());
  uint64_t trace_flow_id = TRACE_FLOW_ID();
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, EVALUATE_SCRIPT_STANDALONE_FROM_BUNDLE,
              [&url, trace_flow_id](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_debug_annotations("url", std::string([url UTF8String]));
                ctx.event()->add_flow_ids(trace_flow_id);
              });
  _runtime_standalone_bundle.runtime_actor_->Act([url = std::string([url UTF8String]),
                                                  sources = std::string(data, length),
                                                  trace_flow_id](auto& runtime) mutable {
    runtime->EvaluateScriptStandalone(std::move(url), std::move(sources), trace_flow_id);
  });
}

- (void)sendGlobalEvent:(nonnull NSString*)name withParams:(nullable NSArray*)params {
  if (name == nil) {
    _LogE(@"Lynx sendGlobalEvent warning: name is nil");
    return;
  }

  auto eventEmitter = [[JSModule alloc] initWithModuleName:@"GlobalEventEmitter"];
  [eventEmitter setJSProxy:_js_proxy];

  NSMutableArray* args = [[NSMutableArray alloc] init];
  [args addObject:name];

  if (params == nil) {
    _LogW(@"Lynx sendGlobalEvent warning: params is nil");
    [args addObject:[[NSArray alloc] init]];
  } else {
    [args addObject:params];
  }
  [eventEmitter fire:@"emit" withParams:args];
}

- (void)callFunction:(nonnull NSString*)moduleName
          withMethod:(nonnull NSString*)methodName
          withParams:(nullable NSArray*)params {
  if (_js_proxy) {
    _js_proxy->CallJSFunction([moduleName UTF8String], [methodName UTF8String],
                              std::make_unique<lynx::pub::ValueImplDarwin>(params));
  }
}

- (void)setSessionStorageItem:(nonnull NSString*)key
             withTemplateData:(nullable LynxTemplateData*)data {
  _runtime_standalone_bundle.runtime_actor_->Act(
      [key = std::string([key UTF8String]), lepus_data = *LynxGetLepusValueFromTemplateData(data),
       delegate = _runtime_standalone_bundle.white_board_delegate_](auto& runtime) mutable {
        delegate->SetSessionStorageItem(std::move(key), std::move(lepus_data));
      });
}

- (void)getSessionStorageItem:(nonnull NSString*)key
                 withCallback:(void (^_Nullable)(id<NSObject> _Nullable))callback {
  auto platform_callback =
      std::make_unique<lynx::shell::PlatformCallBack>([callback](const lepus_value& value) {
        if (callback) {
          callback(lynx::tasm::convertLepusValueToNSObject(value));
        }
      });

  _runtime_standalone_bundle.runtime_actor_->Act(
      [key = std::string([key UTF8String]), platform_callback = std::move(platform_callback),
       facade = _runtime_standalone_bundle.native_runtime_facade_,
       delegate = _runtime_standalone_bundle.white_board_delegate_](auto& runtime) mutable {
        auto callback_holder =
            facade->ActSync([callback = std::move(platform_callback)](auto& facade) mutable {
              return facade->CreatePlatformCallBackHolder(std::move(callback));
            });

        auto value = delegate->GetSessionStorageItem(key);
        delegate->CallPlatformCallbackWithValue(callback_holder, value);
      });
}

- (double)subscribeSessionStorage:(nonnull NSString*)key
                     withCallback:(void (^_Nullable)(id<NSObject> _Nullable))callback {
  auto platform_callback =
      std::make_unique<lynx::shell::PlatformCallBack>([callback](const lepus_value& value) {
        if (callback) {
          callback(lynx::tasm::convertLepusValueToNSObject(value));
        }
      });

  auto callback_holder = _runtime_standalone_bundle.native_runtime_facade_->ActSync(
      [callback = std::move(platform_callback)](auto& facade) mutable {
        return facade->CreatePlatformCallBackHolder(std::move(callback));
      });

  double callback_id = callback_holder->id();

  _runtime_standalone_bundle.runtime_actor_->Act(
      [key = std::string([key UTF8String]), callback_holder = std::move(callback_holder),
       delegate = _runtime_standalone_bundle.white_board_delegate_](auto& runtime) mutable {
        delegate->SubScribeClientSessionStorage(std::move(key), std::move(callback_holder));
      });
  return callback_id;
}

- (void)unSubscribeSessionStorage:(nonnull NSString*)key withId:(double)callbackId {
  _runtime_standalone_bundle.runtime_actor_->Act(
      [key = std::string([key UTF8String]), callbackId,
       delegate = _runtime_standalone_bundle.white_board_delegate_](auto& runtime) mutable {
        delegate->UnsubscribeClientSessionStorage(std::move(key), callbackId);
      });
}

- (void)addRuntimeLifecycleListener:(nonnull id<LynxRuntimeLifecycleListener>)listener {
  _js_proxy->AddLifecycleListener(listener);
}

- (void)transitionToFullRuntime {
  _runtime_standalone_bundle.runtime_actor_->Act(
      [](auto& runtime) { runtime->TransitionToFullRuntime(); });
}

- (void)dealloc {
  if (_state == LynxBackgroundRuntimeStateAttached) {
    // LynxRuntime/NativeRuntimeFacade's release is handled by LynxView
    return;
  }

  _runtime_standalone_bundle.perf_controller_actor_->Act(
      [instance_id =
           _runtime_standalone_bundle.perf_controller_actor_->GetInstanceId()](auto& facade) {
        facade = nullptr;
        lynx::tasm::report::FeatureCounter::Instance()->ClearAndReport(instance_id);
      });

  _runtime_standalone_bundle.native_runtime_facade_->Act(
      [instance_id =
           _runtime_standalone_bundle.perf_controller_actor_->GetInstanceId()](auto& facade) {
        facade = nullptr;
        lynx::tasm::report::FeatureCounter::Instance()->ClearAndReport(instance_id);
      });

  _runtime_standalone_bundle.runtime_actor_->ActAsync(
      [runtime_actor = _runtime_standalone_bundle.runtime_actor_,
       js_group_thread_name = [_options groupThreadName]](auto& runtime) {
        lynx::shell::LynxShell::TriggerDestroyRuntime(runtime_actor, js_group_thread_name);
      });
}

// If user uses the Runtime to build LynxView, `attachToLynxView` will be
// called in LynxTemplateRender. This method can only be called in
// UI Thread, during LynxView build. Once a runtime is used, it
// cannot be used twice, so the state here will prevent this usage.
- (BOOL)attachToLynxView {
  if (_state != LynxBackgroundRuntimeStateStart) {
    _LogE(@"LynxBackgroundRuntime, build LynxView using an invalid LynxBackgroundRuntime, state: "
          @"%ld, runtime: %@",
          (long)_state, self);
    return NO;
  }
  _state = LynxBackgroundRuntimeStateAttached;
  return YES;
}

- (void)onErrorOccurred:(LynxError*)error {
  NSHashTable<id<LynxBackgroundRuntimeLifecycle>>* currTable;
  @synchronized(_innerLifecycleClients) {
    currTable = [_innerLifecycleClients copy];
  }
  for (id<LynxBackgroundRuntimeLifecycle> client in currTable) {
    dispatch_async(dispatch_get_main_queue(), ^{
      if ([client respondsToSelector:@selector(runtime:didRecieveError:)]) {
        [client runtime:self didRecieveError:error];
      }
    });
  }
}

- (void)onEvaluateJavaScriptEnd:(NSString*)url {
  NSHashTable<id<LynxBackgroundRuntimeLifecycle>>* currTable;
  @synchronized(_innerLifecycleClients) {
    currTable = [_innerLifecycleClients copy];
  }
  for (id<LynxBackgroundRuntimeLifecycle> client in currTable) {
    dispatch_async(dispatch_get_main_queue(), ^{
      if ([client respondsToSelector:@selector(runtime:didEvaluateJavaScriptEnd:)]) {
        [client runtime:self didEvaluateJavaScriptEnd:url];
      }
    });
  }
}
@end
