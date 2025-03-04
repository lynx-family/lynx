// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxEnv.h"

#import <objc/message.h>

#import "LynxComponentRegistry.h"
#import "LynxConfig.h"
#import "LynxEnv+Internal.h"
#import "LynxEnvKey.h"
#import "LynxError.h"
#import "LynxLazyRegister.h"
#import "LynxLifecycleDispatcher.h"
#import "LynxLog.h"
#import "LynxService.h"
#import "LynxSubErrorCode.h"
#import "LynxTraceEvent.h"
#import "LynxTraceEventWrapper.h"
#import "LynxViewClient.h"
#if ENABLE_TRACE_PERFETTO
#import "LynxTraceController.h"
#endif
#import "LynxBaseInspectorOwner.h"
#import "LynxDevToolUtils.h"
#import "LynxService.h"
#import "LynxServiceDevToolProtocol.h"

#include "base/include/fml/synchronization/shared_mutex.h"
#include "base/trace/native/trace_event.h"
#include "core/base/darwin/lynx_env_darwin.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/tasm/config.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/jscache/js_cache_manager_facade.h"
#include "core/services/fluency/fluency_tracer.h"
#include "core/services/ssr/ssr_type_info.h"
#include "core/services/timing_handler/timing.h"

#if OS_IOS
#import "LynxUICollection.h"
#import "LynxUIKitAPIAdapter.h"
#endif

@interface LynxEnv ()

@property(nonatomic, strong) NSMutableDictionary<NSString *, NSString *> *externalEnvCache;

@end

@implementation LynxEnv {
  std::unique_ptr<fml::SharedMutex> external_env_mutex_;
}

@synthesize lynxDebugEnabled = _lynxDebugEnabled;
@synthesize highlightTouchEnabled = _highlightTouchEnabled;

+ (instancetype)sharedInstance {
  static LynxEnv *_instance = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    _instance = [[LynxEnv alloc] init];
    [_instance initDevTool];
    [_instance initLynxTrace];
    // register component here without using +load
#if OS_IOS
    [LynxComponentRegistry registerUI:[LynxUICollection class] withName:@"list"];
#endif
  });

  return _instance;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    external_env_mutex_ = std::unique_ptr<fml::SharedMutex>(fml::SharedMutex::Create());
    _externalEnvCache = [NSMutableDictionary dictionary];
    _lifecycleDispatcher = [[LynxLifecycleDispatcher alloc] init];
    _lynxDebugEnabled = NO;
    _devtoolComponentAttach = NO;
    _resoureProviders = [NSMutableDictionary dictionary];
    _locale = [[NSLocale preferredLanguages] objectAtIndex:0];
    _layoutOnlyEnabled = YES;
    _autoResumeAnimation = YES;
    _enableNewTransformOrigin = YES;
    _recordEnable = NO;
    _highlightTouchEnabled = NO;
    [LynxLazyRegister loadLynxInitTask];
    lynx::tasm::LynxEnvDarwin::initNativeUIThread();
    InitLynxLog(lynx::tasm::LynxEnv::GetInstance().IsDevToolEnabled());
#if OS_IOS
    lynx::tasm::Config::InitializeVersion([[UIDevice currentDevice].systemVersion UTF8String]);
#endif
  }
  _LogI(@"LynxEnv: init success");
  return self;
}

- (void)initLynxTrace {
#if ENABLE_TRACE_PERFETTO
  [[LynxTraceController sharedInstance] startStartupTracingIfNeeded];
#endif
}

- (void)initDevTool {
  BLOCK_FOR_INSPECTOR(^{
    [self initDevToolComponentAttachSwitch];
    // Turn on the lynx_debug_enabled switch if the DevTool Component is attached to the host.
    self->_lynxDebugEnabled = self->_devtoolComponentAttach;
    [self initDevToolEnv];
  });
}

- (void)initDevToolComponentAttachSwitch {
  BLOCK_FOR_INSPECTOR(^{
    Class inspectorClass = [LynxService(LynxServiceDevToolProtocol) inspectorOwnerClass];
    if ([inspectorClass conformsToProtocol:@protocol(LynxBaseInspectorOwner)]) {
      self->_devtoolComponentAttach = YES;
      lynx::tasm::LynxEnv::GetInstance().SetBoolLocalEnv([KEY_DEVTOOL_COMPONENT_ATTACH UTF8String],
                                                         true);
    } else {
      self->_devtoolComponentAttach = NO;
    }
  });
}

- (void)setLynxDebugEnabled:(BOOL)lynxDebugEnabled {
  _lynxDebugEnabled = lynxDebugEnabled;
  [self initDevToolEnv];
}

- (BOOL)lynxDebugEnabled {
  // Return true only if the DevTool Component is attached and _lynxDebugEnabled is true. It avoids
  // unnecessary reflection calls.
  return _devtoolComponentAttach && _lynxDebugEnabled;
}

- (void)initDevToolEnv {
  BLOCK_FOR_INSPECTOR(^{
    if ([self lynxDebugEnabled]) {
      [LynxService(LynxServiceDevToolProtocol) devtoolEnvSharedInstance];
    }
  });
}

- (void)setEnableRadonCompatible:(BOOL)value
    __attribute__((deprecated("Radon diff mode can't be close after lynx 2.3."))) {
}

- (void)setEnableLayoutOnly:(BOOL)value {
  _layoutOnlyEnabled = value;
}

- (NSString *)stringFromExternalEnv:(LynxEnvKey)key {
  NSString *keyString = [self.class _keyStringFromType:key];
  return [self _stringFromExternalEnv:keyString];
}

- (BOOL)boolFromExternalEnv:(LynxEnvKey)key defaultValue:(BOOL)defaultValue {
  return [LynxEnv stringValueToBool:[self stringFromExternalEnv:key] defaultValue:defaultValue];
}

- (void)setLocalEnv:(NSString *)value forKey:(NSString *)key {
  lynx::tasm::LynxEnv::GetInstance().SetLocalEnv(key.UTF8String, value.UTF8String);
}

- (void)setDevtoolEnv:(BOOL)value forKey:(NSString *)key {
  [LynxDevToolUtils setDevtoolEnv:value forKey:key];
}

- (BOOL)getDevtoolEnv:(NSString *)key withDefaultValue:(BOOL)value {
  return [LynxDevToolUtils getDevtoolEnv:key withDefaultValue:value];
}

- (void)setDevtoolEnv:(NSSet *)newGroupValues forGroup:(NSString *)groupKey {
  [LynxDevToolUtils setDevtoolEnv:newGroupValues forGroup:groupKey];
}

- (NSSet *)getDevtoolEnvWithGroupName:(NSString *)groupKey {
  return [LynxDevToolUtils getDevtoolEnvWithGroupName:groupKey];
}

- (void)setDevtoolEnabled:(BOOL)enableDevtool {
  _LogI(@"Turn on devtool");
  [self setDevtoolEnv:enableDevtool forKey:SP_KEY_ENABLE_DEVTOOL];
}

- (BOOL)devtoolEnabled {
  return [self getDevtoolEnv:SP_KEY_ENABLE_DEVTOOL withDefaultValue:NO];
}

- (void)setDevtoolEnabledForDebuggableView:(BOOL)enable {
  [self setDevtoolEnv:enable forKey:SP_KEY_ENABLE_DEVTOOL_FOR_DEBUGGABLE_VIEW];
}

- (BOOL)devtoolEnabledForDebuggableView {
  return [self getDevtoolEnv:SP_KEY_ENABLE_DEVTOOL_FOR_DEBUGGABLE_VIEW withDefaultValue:NO];
}

- (BOOL)getEnableRadonCompatible
    __attribute__((deprecated("Radon diff mode can't be close after lynx 2.3."))) {
  return true;
}

- (BOOL)getEnableLayoutOnly {
  return _layoutOnlyEnabled;
}

- (void)setRedBoxEnabled:(BOOL)enableRedBox {
  [self setLogBoxEnabled:enableRedBox];
}

- (BOOL)redBoxEnabled {
  return [self logBoxEnabled];
}

- (void)setLogBoxEnabled:(BOOL)enableLogBox {
  [self setDevtoolEnv:enableLogBox forKey:SP_KEY_ENABLE_LOGBOX];
}

- (BOOL)logBoxEnabled {
  return [self devtoolComponentAttach] && [self getDevtoolEnv:SP_KEY_ENABLE_LOGBOX
                                              withDefaultValue:YES];
}

// This interface is used by TestBench and is only used to debug.
- (void)setLaunchRecordEnabled:(BOOL)launchRecord {
  [self setDevtoolEnv:launchRecord forKey:SP_KEY_ENABLE_LAUNCH_RECORD];
}

// This interface is used by TestBench and is only used to debug.
- (BOOL)launchRecordEnabled {
  return [self getDevtoolEnv:SP_KEY_ENABLE_LAUNCH_RECORD withDefaultValue:NO];
}

- (void)setHighlightTouchEnabled:(BOOL)enableHighlightTouch {
  [self setDevtoolEnv:enableHighlightTouch forKey:SP_KEY_ENABLE_HIGHLIGHT_TOUCH];
  _highlightTouchEnabled = enableHighlightTouch;
}

- (BOOL)highlightTouchEnabled {
  return _highlightTouchEnabled && self.lynxDebugEnabled;
}

- (void)setAutomationEnabled:(BOOL)enableAutomation {
  NSUserDefaults *preference = [NSUserDefaults standardUserDefaults];
  [preference setBool:enableAutomation forKey:SP_KEY_ENABLE_AUTOMATION];
  [preference synchronize];
}

- (BOOL)automationEnabled {
  NSUserDefaults *preference = [NSUserDefaults standardUserDefaults];
  return [preference objectForKey:SP_KEY_ENABLE_AUTOMATION]
             ? [preference boolForKey:SP_KEY_ENABLE_AUTOMATION]
             : YES;
}

- (void)setEnableLogBox:(BOOL)enableLogBox __attribute__((deprecated("Use logBoxEnabled"))) {
  [self setLogBoxEnabled:enableLogBox];
}

- (BOOL)enableLogBox __attribute__((deprecated("Use logBoxEnabled"))) {
  return [self logBoxEnabled];
}

- (void)setEnableDevtoolDebug:(BOOL)enableLogBox __attribute__((deprecated("Use devtoolEnabled"))) {
  [self setDevtoolEnabled:enableLogBox];
}

- (BOOL)enableDevtoolDebug __attribute__((deprecated("Use devtoolEnabled"))) {
  return [self devtoolEnabled];
}

- (void)setAutoResumeAnimation:(BOOL)value {
  _autoResumeAnimation = value;
}

- (BOOL)getAutoResumeAnimation {
  return _autoResumeAnimation;
}

- (void)setEnableNewTransformOrigin:(BOOL)value {
  _enableNewTransformOrigin = value;
}

- (BOOL)getEnableNewTransformOrigin {
  return _enableNewTransformOrigin;
}

- (void)prepareConfig:(LynxConfig *)config {
  if (_config) {
    LLogInfo(@"LynxEnv: current global config has been reset");
  }
  _config = config;
#if OS_IOS
  [_config.componentRegistry makeIntoGloabl];
  // notify devtool
  if (self.lynxDebugEnabled) {
    [LynxService(LynxServiceDevToolProtocol) devtoolEnvPrepareWithConfig:config];
  }
#endif
}

- (BOOL)switchRunloopThread {
  static BOOL isSwitchRunloopThread = NO;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    isSwitchRunloopThread = [self boolFromExternalEnv:LynxEnvSwitchRunloopThread defaultValue:NO];
  });
  return isSwitchRunloopThread;
}

- (void)updateSettings:(NSDictionary *)settings {
  {
    fml::UniqueLock lock{*external_env_mutex_};
    [self.externalEnvCache removeAllObjects];
  }

  lynx::tasm::LynxEnv::GetInstance().CleanExternalCache();
  lynx::tasm::FluencyTracer::SetNeedCheck();
}

- (void)reportModuleCustomError:(NSString *)error {
  [_lifecycleDispatcher lynxView:nil
                 didRecieveError:[LynxError lynxErrorWithCode:ECLynxNativeModulesCustomError
                                                      message:error]];
}

- (void)onPiperInvoked:(NSString *)module
                method:(NSString *)method
              paramStr:(NSString *)paramStr
                   url:(NSString *)url
             sessionID:(NSString *)sessionID {
  NSMutableDictionary *info = [[NSMutableDictionary alloc] init];
  [info setObject:module forKey:@"module-name"];
  [info setObject:method forKey:@"method-name"];
  [info setObject:sessionID forKey:@"session-id"];
  [info setObject:url forKey:@"url"];
  if (![paramStr isEqualToString:@""]) {
    NSArray *arr = @[ paramStr ];
    [info setObject:arr forKey:@"params"];
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxViewLifeCycle::onPiperInvoked", "module",
              [module UTF8String], "method", [method UTF8String], "url", [url UTF8String]);

  [_lifecycleDispatcher onPiperInvoked:info];
}

- (void)onPiperResponsed:(NSString *)module
                  method:(NSString *)method
                     url:(NSString *)url
                response:(NSDictionary *)response
               sessionID:(NSString *)sessionID {
  NSMutableDictionary *info = [[NSMutableDictionary alloc] init];
  [info setObject:module ?: @"" forKey:@"module-name"];
  [info setObject:method ?: @"" forKey:@"method-name"];
  [info setObject:sessionID ?: @"" forKey:@"session-id"];
  [info setObject:url ?: @"" forKey:@"url"];
  [info setObject:response ?: @{} forKey:@"response"];
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxViewLifeCycle::onPiperResponsed", "module",
              [module UTF8String], "method", [method UTF8String], "url", [url UTF8String]);
  [_lifecycleDispatcher onPiperResponsed:info];
}

- (void)setPiperMonitorState:(BOOL)state {
  lynx::tasm::LynxEnv::GetInstance().SetBoolLocalEnv(lynx::tasm::LynxEnv::kLynxEnablePiperMonitor,
                                                     state ? true : false);
}

- (void)addResoureProvider:(NSString *)key provider:(id<LynxResourceProvider>)provider {
  _resoureProviders[key] = provider;
}

- (void)initLayoutConfig:(CGSize)screenSize {
#if OS_IOS
  NSString *version = [UIDevice currentDevice].systemVersion;
  lynx::tasm::Config::InitializeVersion([version UTF8String]);
  const CGFloat scale = [UIScreen mainScreen].scale;
  lynx::tasm::Config::InitPixelValues(screenSize.width * scale, screenSize.height * scale, scale);

  if ([NSThread isMainThread]) {
    [self setKeyWindowAndStatusBar];
  } else {
    __weak typeof(self) weakSelf = self;
    dispatch_async(dispatch_get_main_queue(), ^{
      __strong typeof(weakSelf) strongSelf = weakSelf;
      [strongSelf setKeyWindowAndStatusBar];
    });
  }
#endif  // OS_IOS
}

- (void)setKeyWindowAndStatusBar {
#if OS_IOS
  UIWindow *keyWindow = [LynxUIKitAPIAdapter getKeyWindow];
  CGFloat statusBarHeight = [LynxUIKitAPIAdapter getStatusBarFrame].size.height;

  lynx::starlight::ComputedCSSStyle::SAFE_AREA_INSET_TOP_ = statusBarHeight;
  lynx::starlight::ComputedCSSStyle::SAFE_AREA_INSET_BOTTOM_ = 0;
  lynx::starlight::ComputedCSSStyle::SAFE_AREA_INSET_LEFT_ = 0;
  lynx::starlight::ComputedCSSStyle::SAFE_AREA_INSET_RIGHT_ = 0;
  if (@available(iOS 11.0, *)) {
    lynx::starlight::ComputedCSSStyle::SAFE_AREA_INSET_BOTTOM_ = keyWindow.safeAreaInsets.bottom;
    lynx::starlight::ComputedCSSStyle::SAFE_AREA_INSET_LEFT_ = keyWindow.safeAreaInsets.left;
    lynx::starlight::ComputedCSSStyle::SAFE_AREA_INSET_RIGHT_ = keyWindow.safeAreaInsets.right;
  }
  _LogI(@"LynxEnv: init safe area, top:%f , bottom:%f, left:%f, right:%f.",
        lynx::starlight::ComputedCSSStyle::SAFE_AREA_INSET_TOP_,
        lynx::starlight::ComputedCSSStyle::SAFE_AREA_INSET_BOTTOM_,
        lynx::starlight::ComputedCSSStyle::SAFE_AREA_INSET_LEFT_,
        lynx::starlight::ComputedCSSStyle::SAFE_AREA_INSET_RIGHT_);
#endif  // OS_IOS
}

- (void)setCronetEngine:(void *)engine {
  _cronetEngine = engine;
}

- (void)setCronetServerConfig:(void *)config {
  _cronetServerConfig = config;
}

- (void)enableFluencyTracer:(BOOL)value {
  lynx::tasm::FluencyTracer::SetForceEnable(value);
}

- (BOOL)enableComponentStatisticReport {
  static dispatch_once_t onceToken;
  static BOOL enableComponentStatisticReport = NO;
  dispatch_once(&onceToken, ^{
    enableComponentStatisticReport = [self boolFromExternalEnv:LynxEnvEnableComponentStatisticReport
                                                  defaultValue:NO];
  });
  return enableComponentStatisticReport;
}

- (BOOL)enableCreateUIAsync {
  static dispatch_once_t onceToken;
  static BOOL enableCreateUIAsync = NO;
  dispatch_once(&onceToken, ^{
    enableCreateUIAsync = [self boolFromExternalEnv:LynxEnvEnableCreateUIAsync defaultValue:NO];
  });
  return enableCreateUIAsync;
}

- (BOOL)enableImageEventReport {
  static dispatch_once_t onceToken;
  static BOOL enableImageEventReport = NO;
  dispatch_once(&onceToken, ^{
    enableImageEventReport = [self boolFromExternalEnv:LynxEnvEnableImageEventReport
                                          defaultValue:NO];
  });
  return enableImageEventReport;
}

- (BOOL)enableTextContainerOpt {
  static dispatch_once_t onceToken;
  static BOOL enableTextContainerOpt = NO;
  dispatch_once(&onceToken, ^{
    enableTextContainerOpt = [self boolFromExternalEnv:LynxEnvEnableTextContainerOpt
                                          defaultValue:NO];
  });
  return enableTextContainerOpt;
}

- (BOOL)enableGenericResourceFetcher {
  static dispatch_once_t onceToken;
  static BOOL enableGenericResourceFetcher = NO;
  dispatch_once(&onceToken, ^{
    enableGenericResourceFetcher = [self boolFromExternalEnv:LynxEnvEnableGenericResourceFetcher
                                                defaultValue:NO];
  });
  return enableGenericResourceFetcher;
}

- (BOOL)enableAnimationSyncTimeOpt {
  static dispatch_once_t onceToken;
  static BOOL enableAnimationSyncTimeOpt = NO;
  dispatch_once(&onceToken, ^{
    enableAnimationSyncTimeOpt = [self boolFromExternalEnv:LynxEnvEnableAnimationSyncTimeOpt
                                              defaultValue:NO];
  });
  return enableAnimationSyncTimeOpt;
}

- (NSDictionary<NSString *, NSString *> *)cppEnvDebugDescription {
  std::string cppEnvJson = lynx::tasm::LynxEnv::GetInstance().GetDebugDescription();
  NSString *cppEnvJsonString = [NSString stringWithUTF8String:cppEnvJson.c_str()];
  NSData *cppEnvJsonData = [cppEnvJsonString dataUsingEncoding:NSUTF8StringEncoding];
  NSError *error = nil;
  NSDictionary<NSString *, NSString *> *cppEnvDict =
      [NSJSONSerialization JSONObjectWithData:cppEnvJsonData options:0 error:&error];
  if (error) {
    LLogError(@"Convert native env json string failed, error: %@", error.description);
  }
  return cppEnvDict;
}

- (NSDictionary<NSString *, NSString *> *)platformEnvDebugDescription {
  NSMutableDictionary<NSString *, NSString *> *platformEnvDict = [NSMutableDictionary dictionary];
  for (LynxEnvKey key = (LynxEnvKey)0; key < LynxEnvKeyEndMark;) {
    NSString *keyString = [self.class _keyStringFromType:key];
    NSString *value = [self stringFromExternalEnv:key];
    if (keyString && value) {
      platformEnvDict[keyString] = value;
    }
    key = (LynxEnvKey)((uint64_t)key + 1);
  }
  return platformEnvDict;
}

+ (NSString *)getSSRApiVersion {
  return [NSString stringWithUTF8String:lynx::ssr::kSSRCurrentApiVersion];
}

/**
 * This is a private method and never call it directly.
 * Please use stringFromExternalEnv: instead.
 */
- (NSString *)_stringFromExternalEnv:(NSString *)key {
  NSAssert(key, @"The key should not be nil in stringFromExternalEnv.");
  NSString *value = nil;
  {
    fml::SharedLock lock{*external_env_mutex_};
    value = [self.externalEnvCache objectForKey:key];
  }
  if (!value) {
    // get env from TrailService
    value = [LynxTrail stringValueForTrailKey:key];
    if (!value) {
      value = @"";
    }
    fml::UniqueLock lock{*external_env_mutex_};
    [self.externalEnvCache setObject:value forKey:key];
  }
  return value;
}

// Provide an interface for UT (Unit Testing) that can update the key value of
// externalEnvCacheForKey.
- (void)updateExternalEnvCacheForKey:(NSString *)key withValue:(NSString *)value {
  if (self.externalEnvCache == nil) {
    return;
  }
  fml::UniqueLock lock{*external_env_mutex_};
  [self.externalEnvCache setObject:value forKey:key];
}

+ (NSString *)_keyStringFromType:(LynxEnvKey)key {
  static NSDictionary *const envKeyBinding = @{
    @(LynxEnvSwitchRunloopThread) : @"IOS_SWITCH_RUNLOOP_THREAD",
    @(LynxEnvEnableComponentStatisticReport) : @"enable_component_statistic_report",
    @(LynxEnvEnableLynxDetailLog) : @"enable_lynx_detail_log",
    @(LynxEnvFreeImageMemory) : @"free_image_memory",
    @(LynxEnvFreeImageMemoryForce) : @"free_image_memory_force",
    @(LynxEnvUseNewImage) : @"use_New_Image",
    @(LynxEnvEnableImageExposure) : @"enable_image_exposure",
    @(LynxEnvEnableMultiTASMThread) : @"enable_multi_tasm_thread",
    @(LynxEnvEnableMultiLayoutThread) : @"enable_multi_layout_thread",
    @(LynxEnvTextRenderCacheLimit) : @"text_render_cache_limit",
    @(LynxEnvEnableTextRenderCacheHitRate) : @"enable_text_render_cache_hit_rate",
    @(LynxEnvEnableImageMonitor) : @"enable_image_monitor",
    @(LynxEnvEnableTextLayerRender) : @"enable_text_layer_render",
    @(LynxEnvEnableCreateUIAsync) : @"enable_create_ui_async",
    @(LynxEnvEnableImageEventReport) : @"enable_image_event_report",
    @(LynxEnvEnableGenericResourceFetcher) : @"enable_generic_resource_fetcher",
    @(LynxEnvEnableAnimationSyncTimeOpt) : @"enable_animation_sync_time_opt",
    @(LynxEnvFixNewImageDownSampling) : @"fix_new_image_downsampling",
    @(LynxEnvCachesExpirationDurationInDays) : @"caches_expiration_duration_in_days",
    @(LynxEnvEnableLifecycleTimeReport) : @"enable_lifecycle_time_report",
    @(LynxEnvCachesCleanupUntrackedFiles) : @"caches_cleanup_untracked_files",
    @(LynxEnvEnableTextContainerOpt) : @"enable_text_container_opt",
  };
  NSString *keyString = envKeyBinding[@(key)];
  NSAssert(keyString.length > 0, @"LynxEnv key string should not be nill.");
  return keyString;
}

+ (BOOL)stringValueToBool:(NSString *)value defaultValue:(BOOL)defaultValue {
  if (value.length > 0) {
    return [value isEqualToString:@"1"] || [value.lowercaseString isEqualToString:@"true"];
  }
  return defaultValue;
}

+ (void)clearBytecode:(nonnull NSString *)bytecodeSourceUrl {
  lynx::piper::cache::JsCacheManagerFacade::ClearBytecode([bytecodeSourceUrl UTF8String],
                                                          lynx::piper::JSRuntimeType::quickjs);
}
@end
