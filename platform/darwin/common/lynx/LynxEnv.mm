// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxEnv.h>

#import <objc/message.h>

#import <Lynx/DevToolSettings.h>
#import <Lynx/LynxComponentRegistry.h>
#import <Lynx/LynxConfig.h>
#import <Lynx/LynxEnv+Internal.h>
#import <Lynx/LynxEnvKey.h>
#import <Lynx/LynxError.h>
#import <Lynx/LynxEventReporter.h>
#import <Lynx/LynxLazyRegister.h>
#import <Lynx/LynxLifecycleDispatcher.h>
#import <Lynx/LynxService.h>
#import <Lynx/LynxSubErrorCode.h>
#import <Lynx/LynxTraceEvent.h>
#import <Lynx/LynxTraceEventWrapper.h>
#import <Lynx/LynxViewClient.h>
#import <LynxBase/LynxLog.h>
#import "LynxTraceEventDef.h"
#if ENABLE_TRACE_PERFETTO
#import <Lynx/LynxTraceController.h>
#endif
#import <Lynx/LynxBaseInspectorController.h>
#import <Lynx/LynxBaseInspectorOwnerNG.h>
#import <Lynx/LynxService.h>
#import <Lynx/LynxServiceDevToolProtocol.h>
#import <Lynx/LynxServiceExtensionProtocol.h>
#import <LynxBase/LynxBaseEnv.h>
#import "LynxBytecodeResponseBlock+Converter.h"

#include "base/include/fml/synchronization/shared_mutex.h"
#include "base/include/memory/memory_pressure_level.h"
#include "base/include/notification_center.h"
#include "base/trace/native/trace_event.h"
#include "core/base/darwin/lynx_env_darwin.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/tasm/config.h"
#include "core/renderer/utils/devtool_lifecycle.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/js/bytecode/js_cache_manager_facade.h"
#include "core/services/fluency/fluency_tracer.h"
#include "core/services/performance/memory_monitor/memory_monitor.h"
#include "core/services/ssr/ssr_type_info.h"
#include "core/services/timing_handler/timing.h"

#if OS_IOS
#import <Lynx/LynxFontFaceManager.h>
#import <Lynx/LynxTextRendererCache.h>
#import <Lynx/LynxUICollection.h>
#import <Lynx/LynxUIKitAPIAdapter.h>
#import <UIKit/UIKit.h>
#endif

@interface DevToolSettings (LynxEnvBacktraceInternal)
+ (NSString *)buildBacktraceAddressSummary;
@end

@interface LynxEnv ()

@property(nonatomic, strong) NSMutableDictionary<NSString *, NSString *> *externalEnvCache;

@end

#if OS_IOS
@interface LynxFontFaceManager (LynxEnvPrewarm)
// Warm up the locale-aware fallback family resolution used by custom font cascade fallback.
- (void)prewarmFontCascadeIfNeeded;
@end
#endif

static void LynxClaySetup() {
  Class clayViewProviderClass = NSClassFromString(@"ClayViewProvider");
  SEL selector = NSSelectorFromString(@"onLynxEnvSetup");
  if ([clayViewProviderClass respondsToSelector:selector]) {
    reinterpret_cast<void (*)(id, SEL)>(objc_msgSend)(clayViewProviderClass, selector);
  }
}

@implementation LynxEnv {
  std::unique_ptr<fml::SharedMutex> external_env_mutex_;
}

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
    dispatch_async(dispatch_get_main_queue(), ^{
      // Delay text prewarm until LynxEnv setup is fully finished to avoid pulling
      // trail/settings initialization into LynxEnv init.
      [_instance prewarmTextIfNeeded];
    });
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
    _resoureProviders = [NSMutableDictionary dictionary];
    _locale = [[NSLocale preferredLanguages] objectAtIndex:0];
    _layoutOnlyEnabled = YES;
    _autoResumeAnimation = YES;
    _enableNewTransformOrigin = YES;
    _recordEnable = NO;
    [LynxLazyRegister loadLynxInitTask];
    [self initLynxBase];
    lynx::tasm::LynxEnvDarwin::initNativeUIThread();
#if OS_IOS
    lynx::tasm::Config::InitializeVersion([[UIDevice currentDevice].systemVersion UTF8String]);
#endif
    [LynxService(LynxServiceExtensionProtocol) onLynxEnvSetup];
    LynxClaySetup();
  }
  _LogI(@"LynxEnv: init success");
  return self;
}

- (void)initLynxBase {
  [[LynxBaseEnv sharedInstance] initialize:[self devtoolEnabled]];
}

- (void)initLynxTrace {
#if ENABLE_TRACE_PERFETTO
  [[LynxTraceController sharedInstance] startStartupTracingIfNeeded];
#endif
}

- (void)prewarmTextIfNeeded {
#if OS_IOS
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    if (![self enableTextFontCascadeOpt]) {
      return;
    }
    // Precompute the locale fallback family once so the first custom font measure
    // does not pay the initialization cost on the hot path.
    [[LynxFontFaceManager sharedManager] prewarmFontCascadeIfNeeded];
    @autoreleasepool {
      // Warm up TextKit on the main thread to avoid first-use initialization work
      // racing with background text measurement.
      NSLayoutManager *layoutManager = [[NSLayoutManager alloc] init];
      (void)layoutManager;
    }
  });
#endif
}

- (void)initDevTool {
#if ENABLE_INSPECTOR
  [self initDevToolComponentAttachSwitch];
  [self initDevToolEnv];
#endif
}

- (void)initDevToolComponentAttachSwitch {
#if ENABLE_INSPECTOR
  Class inspectorClass = [LynxService(LynxServiceDevToolProtocol) inspectorOwnerClass];
  if ([inspectorClass conformsToProtocol:@protocol(LynxBaseInspectorOwnerNG)] &&
      [inspectorClass conformsToProtocol:@protocol(LynxBaseInspectorController)]) {
    lynx::tasm::DevToolLifecycle::GetInstance().OnAttached();
    if ([DevToolSettings sharedInstance].bootstrap.lynxDebugEnabled) {
      lynx::tasm::DevToolLifecycle::GetInstance().OnEnabled();
    }
  }
#endif  // ENABLE_INSPECTOR
}

// Since `LynxEnv init` is called inside `LynxEnv sharedInstance`,
// this function is garuanteed to be called after first initialization.
- (void)setLynxDebugEnabled:(BOOL)lynxDebugEnabled {
  if (lynxDebugEnabled) {
    // Intentionally report this event on iOS only.
#if OS_IOS
    NSString *traceString = [DevToolSettings buildBacktraceAddressSummary];
    [LynxEventReporter onEvent:@"lynxsdk_enable_lynx_debug_event"
                    instanceId:-1
                  propsBuilder:^NSDictionary * {
                    return @{@"backtrace" : traceString};
                  }];
#endif

    lynx::tasm::DevToolLifecycle::GetInstance().OnEnabled();
    [self initDevToolEnv];
  } else {
    lynx::tasm::DevToolLifecycle::GetInstance().OnDisabled();
  }
}

- (BOOL)lynxDebugEnabled {
  return lynx::tasm::DevToolLifecycle::GetInstance().IsEnabled();
}

- (void)initDevToolEnv {
#if ENABLE_INSPECTOR
  if ([self lynxDebugEnabled]) {
    [LynxService(LynxServiceDevToolProtocol) devtoolEnvSharedInstance];
  }
#endif
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

- (int)intFromExternalEnv:(LynxEnvKey)key defaultValue:(int)defaultValue {
  NSString *value = [self stringFromExternalEnv:key];
  if (value && value.length > 0) {
    NSScanner *scanner = [NSScanner scannerWithString:value];
    int result = defaultValue;
    if ([scanner scanInt:&result] && scanner.isAtEnd) {
      return result;
    }
  }
  return defaultValue;
}

- (BOOL)boolFromExternalEnv:(LynxEnvKey)key defaultValue:(BOOL)defaultValue {
  return [LynxEnv stringValueToBool:[self stringFromExternalEnv:key] defaultValue:defaultValue];
}

- (void)setLocalEnv:(NSString *)value forKey:(NSString *)key {
  lynx::tasm::LynxEnv::GetInstance().SetLocalEnv(key.UTF8String, value.UTF8String);
}

- (void)setDevtoolEnabled:(BOOL)enableDevtool {
  if (!lynx::tasm::DevToolLifecycle::GetInstance().IsEnabled()) {
    return;
  }
  _LogI(@"Turn %@ devtool", enableDevtool ? @"on" : @"off");
  [DevToolSettings sharedInstance].devToolEnabled = enableDevtool;
}

- (BOOL)devtoolEnabled {
  if (!lynx::tasm::DevToolLifecycle::GetInstance().IsEnabled()) {
    return NO;
  }
  return [DevToolSettings sharedInstance].devToolEnabled;
}

- (BOOL)fspScreenshotEnabled {
  if (!lynx::tasm::DevToolLifecycle::GetInstance().IsEnabled()) {
    return NO;
  }
  return [DevToolSettings sharedInstance].fspScreenshotEnabled;
}

- (void)setFspScreenshotEnabled:(BOOL)fspEnabled {
  if (!lynx::tasm::DevToolLifecycle::GetInstance().IsEnabled()) {
    return;
  }
  [DevToolSettings sharedInstance].fspScreenshotEnabled = fspEnabled;
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
  if (!lynx::tasm::DevToolLifecycle::GetInstance().IsEnabled()) {
    return;
  }
  [DevToolSettings sharedInstance].logBoxEnabled = enableLogBox;
}

// Returns true only if all the following conditions are met:
// 1. The DevTool component is attached.
// 2. logBoxEnabled in setting is true.
// 3. The bootstrap LogBox default is true.
- (BOOL)logBoxEnabled {
  return [self lynxDebugEnabled] && [DevToolSettings sharedInstance].logBoxEnabled &&
         [DevToolSettings sharedInstance].bootstrap.logBoxEnabled;
}

// This interface is used by TestBench and is only used to debug.
- (void)setLaunchRecordEnabled:(BOOL)launchRecord {
  if (!lynx::tasm::DevToolLifecycle::GetInstance().IsEnabled()) {
    return;
  }
  [DevToolSettings sharedInstance].launchRecordEnabled = launchRecord;
}

// This interface is used by TestBench and is only used to debug.
- (BOOL)launchRecordEnabled {
  if (!lynx::tasm::DevToolLifecycle::GetInstance().IsEnabled()) {
    return NO;
  }
  return [DevToolSettings sharedInstance].launchRecordEnabled;
}

- (void)setHighlightTouchEnabled:(BOOL)enableHighlightTouch {
  if (!lynx::tasm::DevToolLifecycle::GetInstance().IsEnabled()) {
    return;
  }
  [DevToolSettings sharedInstance].highlightTouchEnabled = enableHighlightTouch;
}

- (BOOL)highlightTouchEnabled {
  return [DevToolSettings sharedInstance].highlightTouchEnabled && self.lynxDebugEnabled;
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
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LYNX_VIEW_LIFECYCLE_ON_PIPER_INVOKED, "module",
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
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LYNX_VIEW_LIFECYCLE_ON_PIPER_RESPONSE, "module",
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
    lynx::starlight::ComputedCSSStyle::SAFE_AREA_INSET_TOP_ = keyWindow.safeAreaInsets.top;
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

- (void)setEnableMemoryMonitor:(BOOL)value {
  lynx::tasm::performance::MemoryMonitor::SetForceEnable(value);
}

- (BOOL)enableMemoryMonitor {
  return lynx::tasm::performance::MemoryMonitor::Enable();
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

- (BOOL)enableImageAsyncLayout {
  static dispatch_once_t onceToken;
  static BOOL enableImageAsyncLayout = NO;
  dispatch_once(&onceToken, ^{
    enableImageAsyncLayout = [self boolFromExternalEnv:LynxEnvEnableImageAsyncLayout
                                          defaultValue:NO];
  });
  return enableImageAsyncLayout;
}

- (BOOL)enableImageCancelRequest {
  static dispatch_once_t onceToken;
  static BOOL enableImageCancelRequest = NO;
  dispatch_once(&onceToken, ^{
    enableImageCancelRequest = [self boolFromExternalEnv:LynxEnvEnableImageCancelRequest
                                            defaultValue:NO];
  });
  return enableImageCancelRequest;
}

- (BOOL)enableImageCIGaussianBlur {
  static dispatch_once_t onceToken;
  static BOOL enableCIGaussianBlur = NO;
  dispatch_once(&onceToken, ^{
    enableCIGaussianBlur = [self boolFromExternalEnv:LynxEnvEnableImageCIGaussianBlur
                                        defaultValue:NO];
  });
  return enableCIGaussianBlur;
}

- (BOOL)getUseNewImage {
  static dispatch_once_t onceToken;
  static BOOL useNewImage = NO;
  dispatch_once(&onceToken, ^{
    useNewImage = [[LynxEnv sharedInstance] boolFromExternalEnv:LynxEnvUseNewImage defaultValue:NO];
  });
  return useNewImage;
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

- (BOOL)enableTextGradientOpt {
  static dispatch_once_t onceToken;
  static BOOL enableTextGradientOpt = NO;
  dispatch_once(&onceToken, ^{
    enableTextGradientOpt = [self boolFromExternalEnv:LynxEnvEnableTextGradientOpt defaultValue:NO];
  });
  return enableTextGradientOpt;
}

- (BOOL)enableTextFontCascadeOpt {
  static dispatch_once_t onceToken;
  static BOOL enableTextFontCascadeOpt = NO;
  dispatch_once(&onceToken, ^{
    enableTextFontCascadeOpt = [self boolFromExternalEnv:LynxEnvEnableTextFontCascadeOpt
                                            defaultValue:NO];
  });
  return enableTextFontCascadeOpt;
}

- (int)memoryAcquisitionDelaySec {
  static dispatch_once_t onceToken;
  static int delaySecond = 0;
  dispatch_once(&onceToken, ^{
    delaySecond = lynx::tasm::LynxEnv::GetInstance().GetMemoryAcquisitionDelaySec();
  });
  return delaySecond;
}

- (int)memoryReportIntervalSec {
  static dispatch_once_t onceToken;
  static int delayMin = 0;
  dispatch_once(&onceToken, ^{
    delayMin = lynx::tasm::LynxEnv::GetInstance().GetMemoryReportIntervalSec();
  });
  return delayMin;
}

- (int)globalMemoryReportThresholdMB {
  static dispatch_once_t onceToken;
  static int thresholdMB = 0;
  dispatch_once(&onceToken, ^{
    thresholdMB = [self intFromExternalEnv:LynxEnvGlobalMemoryReportThresholdMB defaultValue:30];
  });
  return thresholdMB;
}

#pragma mark - FSP Config
- (BOOL)enableFSP {
  static dispatch_once_t onceToken;
  static BOOL enableFSP = NO;
  dispatch_once(&onceToken, ^{
    enableFSP = [self boolFromExternalEnv:LynxEnvFSPEnable defaultValue:NO];
  });
  return enableFSP;
}

- (NSDictionary<NSString *, id> *_Nullable)fspConfig {
  if (![self enableFSP]) {
    return nil;
  }
  static dispatch_once_t onceToken;
  static NSDictionary<NSString *, id> *fspConfig = nil;
  dispatch_once(&onceToken, ^{
    NSString *jsonStr = [self stringFromExternalEnv:LynxEnvFSPConfigJsonString];
    if (!jsonStr) {
      return;
    }
    NSData *jsonData = [jsonStr dataUsingEncoding:NSUTF8StringEncoding];
    if (!jsonData) {
      return;
    }
    NSError *error = nil;
    fspConfig = [NSJSONSerialization JSONObjectWithData:jsonData options:0 error:&error];
    if (error) {
      LLogError(@"Convert FSP config json string failed, error: %@", error.description);
    }
  });
  return fspConfig;
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
    @(LynxEnvEnableImageAsyncLayout) : @"enable_image_async_layout",
    @(LynxEnvEnableImageCancelRequest) : @"enable_image_cancel_request",
    @(LynxEnvEnableImageCIGaussianBlur) : @"enable_image_ci_gaussian_blur",
    @(LynxEnvEnableGenericResourceFetcher) : @"enable_generic_resource_fetcher",
    @(LynxEnvEnableAnimationSyncTimeOpt) : @"enable_animation_sync_time_opt",
    @(LynxEnvFixNewImageDownSampling) : @"fix_new_image_downsampling",
    @(LynxEnvCachesExpirationDurationInDays) : @"caches_expiration_duration_in_days",
    @(LynxEnvCachesCleanupUntrackedFiles) : @"caches_cleanup_untracked_files",
    @(LynxEnvEnableTextContainerOpt) : @"enable_text_container_opt",
    @(LynxEnvEnableJSGroupThreadByDefault) : @"enable_multi_js_thread_by_default",
    @(LynxEnvEnableTextLayoutCache) : @"enable_text_layout_cache",
    @(LynxEnvEnableForceMemoryMonitorOnOom) : @"enable_force_memory_monitor_on_oom",
    @(LynxEnvEnableTextGradientOpt) : @"lynx_text_gradient_opt",
    @(LynxEnvEnableTextFontCascadeOpt) : @"lynx_text_font_cascade_opt",
    @(LynxEnvGlobalMemoryReportThresholdMB) : @"global_memory_report_threshold_mb",
    @(LynxEnvFSPEnable) : @"enable_fsp",
    @(LynxEnvFSPConfigJsonString) : @"fsp_config_json_string",
    @(LynxEnvSetupCanvasSurfaceEarlier) : @"setup_canvas_surface_earlier"
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
  lynx::runtime::js::cache::JsCacheManagerFacade::ClearBytecode(
      [bytecodeSourceUrl UTF8String], lynx::runtime::js::JSRuntimeType::quickjs);
}

+ (void)setGlobalBytecodeGenerateCallback:(nullable LynxBytecodeResponseBlock)callback {
  lynx::runtime::js::cache::JsCacheManagerFacade::SetGlobalBytecodeGenerateCallback(
      CreateBytecodeGenerateCallback(callback));
}

- (void)trimMemory:(LynxMemoryPressureLevel)pressure {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LynxEnv.trimMemory");
  lynx::base::MemoryPressureLevel level =
      lynx::base::MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_NONE;
  switch (pressure) {
    case LynxMemoryPressureLevelModerate:
      level = lynx::base::MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_MODERATE;
      break;
    case LynxMemoryPressureLevelCritical:
      level = lynx::base::MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_CRITICAL;
#if OS_IOS
      [[LynxTextRendererCache cache] clearCache];
#endif
      break;
    case LynxMemoryPressureLevelNone:
    default:
      level = lynx::base::MemoryPressureLevel::MEMORY_PRESSURE_LEVEL_NONE;
      return;
  }
  lynx::base::NotificationCallback::Notify(lynx::base::MEMORY_PRESSURE_NOTIFICATION,
                                           static_cast<intptr_t>(level));
}

@end
