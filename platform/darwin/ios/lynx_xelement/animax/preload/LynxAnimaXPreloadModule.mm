// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <XElement/LynxAnimaXPreloadModule.h>

#import <AnimaX/AnimaXContext.h>
#import <AnimaX/AnimaXPlayer.h>
#import <Lynx/LynxContext+Internal.h>
#import <Lynx/LynxContextModule.h>
#import <Lynx/LynxEnv+Internal.h>
#import <Lynx/LynxEnv.h>
#import <Lynx/LynxUIOwner.h>
#import <XElement/LynxAnimaXAbility.h>
#import <XElement/LynxAnimaXImageDecoderServiceImpl.h>
#import <XElement/LynxAnimaXResourceFactoryServiceImpl.h>

#include <cmath>
#include <memory>
#include <string>
#include <utility>

#import <objc/runtime.h>

#include <vector>
#include "core/runtime/js/runtime_lifecycle_listener_delegate.h"
#include "include/player/animax_player.h"
#include "src/base/log/log.h"
#include "src/jsbridge/bindings/animax/preload_host.h"
#include "src/resource/resource_loader/ios/resource_pipeline_ios.h"
#include "src/resource/resource_loader/ios/unzip_loader_ios.h"

using lynx::animax::PreloadHost;
using lynx::animax::PreloadHostCapabilities;

namespace {

char kPageDestroyedObservation;

// One initialization per page runtime. Only this short completion bookkeeping
// is shared across JS and Main; no page/service access occurs under its lock.
struct PreloadInitialization {
  const int64_t runtime_id;
  const std::shared_ptr<PreloadHost> host;
  std::mutex mutex;
  bool completed = false;
  bool ready = false;
  std::vector<std::function<void(bool)>> callbacks;

  PreloadInitialization(int64_t id, std::shared_ptr<PreloadHost> value)
      : runtime_id(id), host(std::move(value)) {}

  void WhenReady(std::function<void(bool)> callback) {
    {
      std::lock_guard<std::mutex> lock(mutex);
      if (!completed) {
        callbacks.push_back(std::move(callback));
        return;
      }
    }
    callback(ready && host->IsValid());
  }

  void Complete(bool success) {
    std::vector<std::function<void(bool)>> pending;
    {
      std::lock_guard<std::mutex> lock(mutex);
      if (completed) return;
      completed = true;
      ready = success;
      pending.swap(callbacks);
    }
    for (auto &callback : pending) callback(success && host->IsValid());
  }

  bool IsReady() {
    std::lock_guard<std::mutex> lock(mutex);
    return completed && ready && host->IsValid();
  }
};

class PreloadRuntimeListener : public lynx::runtime::RuntimeLifecycleListenerDelegate {
 public:
  explicit PreloadRuntimeListener(std::shared_ptr<PreloadInitialization> state)
      // FULL observes this page's Env, including shared-runtime pages.
      : RuntimeLifecycleListenerDelegate(DelegateType::FULL), state_(std::move(state)) {}
  ~PreloadRuntimeListener() override {
    // A runtime actor may discard registration during teardown.
    state_->Complete(false);
  }
  void OnRuntimeCreate(std::shared_ptr<lynx::runtime::IVSyncObserver>) override {}
  void OnRuntimeInit(int64_t) override {}
  void OnAppEnterForeground() override {}
  void OnAppEnterBackground() override {}
  void OnRuntimeAttach(void *env, const char *) override {
    auto native_env = static_cast<napi_env>(env);
    bool attached = PreloadHost::AttachToEnv(native_env, state_->host);
    if (attached) {
      env_ = native_env;
    } else if (state_->host->IsValid()) {
      ANIMAX_LOGW("Failed to attach a valid preload host to its page NAPI Env");
    }
    state_->Complete(attached);
  }
  void OnRuntimeDetach() override {
    if (env_) {
      PreloadHost::DetachFromEnv(env_);
      env_ = nullptr;
    }
    state_->host->Invalidate();
    state_->Complete(false);
  }

 private:
  std::shared_ptr<PreloadInitialization> state_;
  napi_env env_ = nullptr;
};

}  // namespace

@interface LynxAnimaXPreloadModule () {
  __weak LynxContext *_lynxContext;
  BOOL _observingDestroyed;
  // Accessed under @synchronized(self). Each old runtime listener keeps only
  // its own state, so a late detach cannot invalidate a replacement runtime.
  std::shared_ptr<PreloadInitialization> _initialization;
  lynx::animax::Loader<lynx::animax::ResourceRequest, lynx::animax::ResourceResponse>::Ptr
      _resourceLoader;
  lynx::animax::Loader<lynx::animax::UnzipRequest, lynx::animax::UnzipResponse>::Ptr _unzipLoader;
}
- (void)acquireCapabilities:(PreloadHost::Callback)callback runtimeId:(int64_t)runtimeId;
- (void)invalidateHost;
- (void)stopObservingContext;
@end

@implementation LynxAnimaXPreloadModule

+ (instancetype)moduleForContext:(LynxContext *)context {
  if (!context) return nil;
  // NativeModule calls and component creation may race. The association is
  // page-owned, created only on use, and never retains the page in reverse.
  @synchronized(context) {
    LynxAnimaXPreloadModule *module =
        objc_getAssociatedObject(context, @selector(moduleForContext:));
    if (!module) {
      module = [LynxAnimaXPreloadModule new];
      module->_lynxContext = context;
      objc_setAssociatedObject(context, @selector(moduleForContext:), module,
                               OBJC_ASSOCIATION_RETAIN_NONATOMIC);
    }
    return module;
  }
}

- (void)ensureReady:(void (^)(BOOL))callback {
  LynxContext *context = _lynxContext;
  if (!context) {
    callback(NO);
    return;
  }
  int64_t runtimeId = context.getLynxRuntimeId.longLongValue;
  std::shared_ptr<PreloadInitialization> state;
  std::shared_ptr<PreloadInitialization> previous;
  BOOL start = NO;
  @synchronized(self) {
    if (!_initialization || _initialization->runtime_id != runtimeId) {
      previous = _initialization;
      __weak LynxAnimaXPreloadModule *weakSelf = self;
      auto host =
          std::make_shared<PreloadHost>([weakSelf, runtimeId](PreloadHost::Callback completion) {
            dispatch_async(dispatch_get_main_queue(), ^{
              LynxAnimaXPreloadModule *module = weakSelf;
              if (module)
                [module acquireCapabilities:completion runtimeId:runtimeId];
              else
                completion({}, "Preload host is unavailable");
            });
          });
      _initialization = std::make_shared<PreloadInitialization>(runtimeId, std::move(host));
      start = YES;
    }
    state = _initialization;
  }
  if (previous) {
    previous->host->Invalidate();
    previous->Complete(false);
  }
  state->WhenReady([callback](bool ready) { callback(ready); });
  if (!start) return;

  // Lightweight host allocation above touches no UI or services. Page lifetime
  // observation is installed on Main; Env attachment stays on the JS runtime.
  void (^initialize)(void) = ^{
    LynxContext *owner = self->_lynxContext;
    if (!owner || owner.hasLynxViewDestroyed || !owner->proxy_ ||
        owner.getLynxRuntimeId.longLongValue != state->runtime_id || !state->host->IsValid()) {
      state->host->Invalidate();
      state->Complete(false);
      return;
    }
    if (!self->_observingDestroyed) {
      self->_observingDestroyed = YES;
      [owner addObserver:self
              forKeyPath:@"hasLynxViewDestroyed"
                 options:NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew
                 context:&kPageDestroyedObservation];
    }
    // Existing lifecycle replay supplies the Env even after runtime attach.
    owner->proxy_->AddLifecycleListener(std::make_unique<PreloadRuntimeListener>(state));
  };
  if (NSThread.isMainThread)
    initialize();
  else
    dispatch_async(dispatch_get_main_queue(), initialize);
}

- (void)dealloc {
  [self invalidateHost];
  [self stopObservingContext];
}

- (BOOL)attachCompositionPlayer:(AnimaXPlayer *)player {
  auto nativePlayer = [player getPlayerShared];
  std::shared_ptr<PreloadInitialization> state;
  @synchronized(self) {
    state = _initialization;
  }
  if (!nativePlayer || !state || !state->IsReady()) return NO;
  std::weak_ptr<PreloadHost> host = state->host;
  nativePlayer->SetCompositionHost(host, [host](std::function<void()> commit) {
    auto owner = host.lock();
    return owner && owner->RunIfValid(std::move(commit));
  });
  return YES;
}

+ (void)setCompositionMode:(BOOL)enabled forPlayer:(AnimaXPlayer *)player {
  auto nativePlayer = [player getPlayerShared];
  if (nativePlayer) nativePlayer->SetCompositionMode(enabled);
}

+ (NSDictionary *)compositionReceiverInfoForPlayer:(AnimaXPlayer *)player {
  auto nativePlayer = [player getPlayerShared];
  if (!nativePlayer) return nil;
  return @{
    @"elementID" : [NSString stringWithUTF8String:nativePlayer->GetPlayerID().c_str()],
    @"instanceToken" :
        [NSString stringWithUTF8String:std::to_string(nativePlayer->GetInstanceToken()).c_str()]
  };
}

+ (void)cancelPendingCompositionBindingForPlayer:(AnimaXPlayer *)player {
  auto nativePlayer = [player getPlayerShared];
  if (nativePlayer) nativePlayer->CancelPendingCompositionBinding();
}

- (void)acquireCapabilities:(PreloadHost::Callback)callback runtimeId:(int64_t)runtimeId {
  LynxContext *context = _lynxContext;
  std::shared_ptr<PreloadInitialization> state;
  @synchronized(self) {
    state = _initialization;
  }
  // Main may have replaced the page runtime while this request was queued and
  // before the old JS thread delivered detach. Do not start work for that Env.
  if (!context || context.hasLynxViewDestroyed || !state || state->runtime_id != runtimeId ||
      context.getLynxRuntimeId.longLongValue != runtimeId || !state->IsReady()) {
    callback({}, "Preload host is unavailable");
    return;
  }
  LynxUIContext *uiContext = context.uiOwner.uiContext;
  float scale = uiContext.screenMetrics.scale;
  // Match Player's page screen metrics. Never fall back to the main screen or 1.
  if (!uiContext || !std::isfinite(scale) || scale <= 0) {
    callback({}, "Preload host has no valid screen scale");
    return;
  }

  if (!_resourceLoader) {
    LynxAnimaXAbility *ability = [[LynxAnimaXAbility alloc] init];
    [ability registerService:@protocol(AnimaXResourceFactoryService)
                    withImpl:[[LynxAnimaXResourceFactoryServiceImpl alloc]
                                 initWithLynxUIContext:uiContext]];
    BOOL enableImageDecoderService =
        [LynxEnv stringValueToBool:[[LynxEnv sharedInstance]
                                       _stringFromExternalEnv:@"enable_image_decoder_service"]
                      defaultValue:YES];
    if (enableImageDecoderService) {
      [ability registerService:@protocol(AnimaXImageDecoderService)
                      withImpl:[LynxAnimaXImageDecoderServiceImpl new]];
    }
    AnimaXContext *animaxContext = [[AnimaXContext alloc] initWithAbility:ability];
    // Keep the page's service instance in its own pipeline. The global native
    // factory cache is unsuitable for distinct pages with the same service scope.
    _resourceLoader = lynx::animax::ResourcePipelineIOS::MakePipeline(animaxContext);
    _unzipLoader = lynx::animax::UnzipLoaderIOS::Make<lynx::animax::UnzipLoaderIOS>();
  }
  if (!_resourceLoader || !_unzipLoader) {
    callback({}, "Preload resource services are unavailable");
    return;
  }
  callback(PreloadHostCapabilities{_resourceLoader, _unzipLoader, scale}, {});
}

- (void)invalidateHost {
  std::shared_ptr<PreloadInitialization> state;
  @synchronized(self) {
    state = _initialization;
  }
  if (state) {
    state->host->Invalidate();
    state->Complete(false);
  }
  // Drop the page's ownership immediately. In-flight native requests retain
  // their own snapshots only until their completion/cleanup runs.
  _resourceLoader.reset();
  _unzipLoader.reset();
}

- (void)stopObservingContext {
  if (_observingDestroyed) {
    [_lynxContext removeObserver:self
                      forKeyPath:@"hasLynxViewDestroyed"
                         context:&kPageDestroyedObservation];
    _observingDestroyed = NO;
  }
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary<NSKeyValueChangeKey, id> *)change
                       context:(void *)context {
  if (context != &kPageDestroyedObservation) {
    [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    return;
  }
  if (((LynxContext *)object).hasLynxViewDestroyed) {
    [self invalidateHost];
    [self stopObservingContext];
  }
}

@end

// JS reads this callback as (success, code, message), and Android passes all
// three. The Darwin bridge passes a callback result as one argument, so pass
// only success; JS then reports a failure as EnvironmentUnavailable.
static void EnsurePageReady(LynxContext *context, LynxCallbackBlock callback) {
  if (!context) {
    callback(@NO);
    return;
  }
  [[LynxAnimaXPreloadModule moduleForContext:context] ensureReady:^(BOOL ready) {
    callback(@(ready));
  }];
}

// The standard per-page NativeModule factory creates this entry lazily,
// including pages sharing a JavaScript runtime through LynxGroup.
@interface LynxAnimaXPreloadInitModule : NSObject <LynxContextModule>
@end
@implementation LynxAnimaXPreloadInitModule {
  __weak LynxContext *_context;
}
+ (NSString *)name {
  return @"AnimaXPreload";
}
+ (NSDictionary *)methodLookup {
  return @{@"ensureReady" : NSStringFromSelector(@selector(ensureReady:))};
}
- (instancetype)initWithLynxContext:(LynxContext *)context {
  if ((self = [super init])) _context = context;
  return self;
}
- (void)ensureReady:(LynxCallbackBlock)callback {
  EnsurePageReady(_context, callback);
}
@end

// Shared modules receive the calling page from the existing native bridge.
// Never retain a default page here: each page still owns its own module and host.
@interface LynxAnimaXSharedPreloadInitModule : NSObject <LynxModule>
@end
@implementation LynxAnimaXSharedPreloadInitModule
+ (NSString *)name {
  return @"AnimaXPreload";
}
+ (NSDictionary *)methodLookup {
  return @{@"ensureReady" : NSStringFromSelector(@selector(ensureReady:callback:))};
}
- (void)ensureReady:(LynxContext *)context callback:(LynxCallbackBlock)callback {
  EnsurePageReady(context, callback);
}
@end
