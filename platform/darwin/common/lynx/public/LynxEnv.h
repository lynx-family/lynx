// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>

#import <Lynx/LynxBytecodeResponseBlock.h>
#import <Lynx/LynxEnvKey.h>

NS_ASSUME_NONNULL_BEGIN

/** One high-memory event dictionary, delivered asynchronously on the main thread. */
typedef void (^LynxHighMemoryCallback)(NSDictionary<NSString *, id> *event);

typedef NS_ENUM(NSInteger, LynxMemoryPressureLevel) {
  /**
   * No problems, there is enough memory to use. This event is not sent via callback, but the enum
   * is used in other places to find out the current state of the system.
   */
  LynxMemoryPressureLevelNone = 0,
  /**
   * Modules are advised to free buffers that are cheap to re-allocate and not immediately needed.
   */
  LynxMemoryPressureLevelModerate = 1,
  /**
   * At this level, modules are advised to free all possible memory.  The alternative is to be
   * killed by the system, which means all memory will have to be re-created, plus the cost of a
   * cold start.
   */
  LynxMemoryPressureLevelCritical = 2,
};

typedef NS_ENUM(NSInteger, LynxMTSContextType) {
  LynxMTSContextTypeVM = 0,
  LynxMTSContextTypeLepusNG = 1,
  LynxMTSContextTypeRTS = 2,
  LynxMTSContextTypeRTSNative = 3,
};

@class LynxConfig;
@class LynxLifecycleDispatcher;
@protocol LynxViewLifecycle;
@protocol LynxResourceProvider;

/*!
 LynxEnv  can be reused for multiple LynxViews
*/
@interface LynxEnv : NSObject

@property(nonatomic, readonly) LynxConfig *config;
@property(nonatomic, readwrite) NSString *locale;
@property(nonatomic, readonly) LynxLifecycleDispatcher *lifecycleDispatcher;
@property(nonatomic, readonly)
    NSMutableDictionary<NSString *, id<LynxResourceProvider>> *resoureProviders;
@property(nonatomic, readwrite) BOOL lynxDebugEnabled;
/*!
 * mDevtoolComponentAttach: indicates whether DevTool Component is attached to the host.
 * mDevtoolEnabled: control whether to enable DevTool Debug
 *
 * eg:
 * when host client attach DevTool, mDevtoolComponentAttach is set true by reflection to find class
 * defined in DevTool and now if we set mDevtoolEnabled switch true, DevTool Debug is usable. if set
 * mDevtoolEnabled false, DevTool Debug is unavailable.
 *
 * when host client doesn't attach DevTool, can't find class defined in DevTool and
 * mDevtoolComponentAttach is set false in this case, no matter mDevtoolEnabled switch is set true
 * or false ,DevTool Debug is unavailable
 *
 * To sum up, mDevtoolComponentAttach indicates host package type, online package without DevTool or
 * localtest with DevTool mDevtoolEnabled switch is controlled by user to enable/disable DevTool
 * Debug, and useless is host doesn't attach DevTool
 */
@property(nonatomic, readonly) BOOL devtoolComponentAttach;
@property(nonatomic, readwrite) BOOL devtoolEnabled;
@property(nonatomic, readwrite) BOOL redBoxEnabled
    __attribute__((deprecated("Please use logBoxEnabled to instead")));
@property(nonatomic, readwrite) BOOL logBoxEnabled;
@property(nonatomic, readwrite) BOOL highlightTouchEnabled;
@property(nonatomic, readwrite) BOOL fspScreenshotEnabled;
@property(nonatomic, readwrite) BOOL automationEnabled;
@property(nonatomic, readwrite) BOOL layoutOnlyEnabled;
@property(nonatomic, readwrite) BOOL autoResumeAnimation;
@property(nonatomic, readwrite) BOOL enableNewTransformOrigin;
@property(nonatomic, readwrite) BOOL recordEnable;
@property(nonatomic, readwrite) BOOL launchRecordEnabled;

// use for ttnet by reject way .
@property(nonatomic, readwrite) void *cronetEngine;
// use for ttnet by reject way .
@property(nonatomic, readwrite) void *cronetServerConfig;

@property(nonatomic, readwrite) BOOL enableDevMenu
    __attribute__((deprecated("Use unified flag enableDevtoolDebug")));

@property(nonatomic, readwrite) BOOL enableJSDebug
    __attribute__((deprecated("Use unified flag enableDevtoolDebug")));

@property(nonatomic, readwrite) BOOL enableDevtoolDebug
    __attribute__((deprecated("Use devtoolEnabled")));

@property(nonatomic, readwrite) BOOL enableLogBox __attribute__((deprecated("Use logBoxEnabled")));

// values from settings
@property(nonatomic, readonly) BOOL switchRunloopThread;

+ (instancetype)sharedInstance;

/**
 * Applies the development DevTool bootstrap profile before `sharedInstance` initializes LynxEnv.
 */
+ (void)prepareDevToolForDevelopmentBeforeInit;

+ (void)prepareGlobalMTSRuntimePoolWithContextType:(LynxMTSContextType)contextType
                                             count:(NSInteger)count;

/**
 * Initializes LynxEnv and invokes the completion handler asynchronously on the main thread.
 *
 * When the completion handler is invoked, `isInitCompleted` returns `YES`.
 */
- (instancetype)init:(nullable dispatch_block_t)completionHandler;

/**
 * Returns whether LynxEnv initialization has completed successfully.
 *
 * This method is safe to call from any thread. It returns true only after the singleton
 * initialization flow and native UI thread initialization have both completed.
 */
- (BOOL)isInitCompleted;

- (void)prepareConfig:(LynxConfig *)config;
- (void)reportModuleCustomError:(NSString *)error;
- (void)onPiperInvoked:(NSString *)module
                method:(NSString *)method
              paramStr:(NSString *)paramStr
                   url:(NSString *)url
             sessionID:(NSString *)sessionID;
- (void)onPiperResponsed:(NSString *)module
                  method:(NSString *)method
                     url:(NSString *)url
                response:(NSDictionary *)response
               sessionID:(NSString *)sessionID;
- (void)updateSettings:(NSDictionary *)settings;
- (void)addResoureProvider:(NSString *)key provider:(id<LynxResourceProvider>)provider;

- (BOOL)boolFromExternalEnv:(LynxEnvKey)key defaultValue:(BOOL)defaultValue;
- (NSString *)stringFromExternalEnv:(LynxEnvKey)key;

- (void)setLocalEnv:(NSString *)value forKey:(NSString *)key;

- (void)setEnableRadonCompatible:(BOOL)value
    __attribute__((deprecated("Radon diff mode can't be close after lynx 2.3.")));
- (BOOL)getEnableRadonCompatible
    __attribute__((deprecated("Radon diff mode can't be close after lynx 2.3.")));

- (void)setEnableLayoutOnly:(BOOL)value;
- (BOOL)getEnableLayoutOnly;

- (void)setPiperMonitorState:(BOOL)state;
- (void)initLayoutConfig:(CGSize)screenSize;

- (void)setAutoResumeAnimation:(BOOL)value;
- (BOOL)getAutoResumeAnimation;

- (void)setEnableNewTransformOrigin:(BOOL)value;
- (BOOL)getEnableNewTransformOrigin;

- (void)setCronetEngine:(void *)engine;
- (void)setCronetServerConfig:(void *)config;

- (void)enableFluencyTracer:(BOOL)value;

- (BOOL)enableComponentStatisticReport;

- (BOOL)enableImageEventReport;

- (BOOL)enableImageAsyncLayout;

- (BOOL)enableImageLoadSVG;

- (BOOL)enableImageCancelRequest;

- (BOOL)enableImagePlaceholderResetFix;

- (BOOL)enableImageCIGaussianBlur;

- (BOOL)getUseNewImage;

- (BOOL)enableGenericResourceFetcher;

- (BOOL)enableTextContainerOpt;

- (BOOL)enableTextGradientOpt;

- (BOOL)enableTextFontCascadeOpt;

/**
 * Dispatches a memory pressure signal to registered callbacks.
 */
- (void)trimMemory:(LynxMemoryPressureLevel)pressure;

#pragma mark - FSP Config
- (BOOL)enableFSP;

- (NSDictionary<NSString *, id> *_Nullable)fspConfig;

- (NSDictionary<NSString *, NSString *> *)cppEnvDebugDescription;

- (NSDictionary<NSString *, NSString *> *)platformEnvDebugDescription;

/**
 * Get the version of the SSR API. You should always include the SSR API version when generating SSR
 *data with the SSR server, otherwise you may encounter compatibility issues.
 *
 *@return ssr api version.
 */
+ (NSString *_Nonnull)getSSRApiVersion;

/**
 * Clear bytecode for bytecodeSourceUrl.
 * When bytecodeSourceUrl is empty, that means clear all bytecode.
 */
+ (void)clearBytecode:(nonnull NSString *)bytecodeSourceUrl;

/**
 * set global bytecode generate callback
 * @param callback will call when generate bytecode success or failed.This will be held globally;
 *     please pay attention to memory management.
 */
+ (void)setGlobalBytecodeGenerateCallback:(nullable LynxBytecodeResponseBlock)callback;

/**
 * Appends a process-wide callback, or clears all callbacks with nil. The block is copied.
 * Callbacks run in registration order using a snapshot taken at main-thread delivery.
 * Changes during callbacks affect later events.
 * Each VM or page event type is emitted once, including when no callback is registered.
 * Events are not replayed. The type is large_vm, large_bts, or large_ui.
 * Events contain key (VM name or page key) and bytes; large_vm and large_bts add vm_type
 * (quickjs, v8, jsvm, jsc, or unknown). large_ui adds counts with alternating category
 * names and instance counts.
 * Monitored VM events add total_page_count and slots with alternating names and byte counts,
 * sorted by descending bytes. Page and V8/JSVM events require memory monitoring;
 * shared QuickJS VM events do not.
 */
+ (void)setHighMemoryCallback:(nullable LynxHighMemoryCallback)callback;

@end

NS_ASSUME_NONNULL_END
