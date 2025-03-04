// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>

#import "LynxBackgroundRuntime.h"
#import "LynxConfig.h"
#import "LynxDynamicComponentFetcher.h"
#import "LynxGenericResourceFetcher.h"
#import "LynxGroup.h"
#import "LynxMediaResourceFetcher.h"
#import "LynxTemplateResourceFetcher.h"
#import "LynxViewEnum.h"

typedef NS_ENUM(NSInteger, LynxBooleanOption) {
  LynxBooleanOptionUnset = 0,
  LynxBooleanOptionTrue = 1,
  LynxBooleanOptionFalse = 2,
};

@interface LynxViewBuilder : NSObject

@property(nonatomic, nullable) LynxConfig* config;
@property(nonatomic, nullable) LynxGroup* group;
@property(nonatomic, nullable) LynxBackgroundRuntime* lynxBackgroundRuntime;
@property(nonatomic, assign) BOOL enableLayoutSafepoint;
@property(nonatomic, assign) BOOL enableAutoExpose;
@property(nonatomic, assign) BOOL enableTextNonContiguousLayout;
@property(nonatomic, assign) BOOL enableLayoutOnly;
@property(nonatomic, assign) BOOL enableUIOperationQueue;
@property(nonatomic, assign) BOOL enablePendingJSTaskOnLayout;
@property(nonatomic, assign) BOOL enableJSRuntime;
@property(nonatomic, assign) BOOL enableAirStrictMode;
@property(nonatomic, assign) BOOL enableAsyncCreateRender;
@property(nonatomic, assign) BOOL enableRadonCompatible;
@property(nonatomic, assign) BOOL enableSyncFlush;
@property(nonatomic, assign) BOOL enableMultiAsyncThread;
@property(nonatomic, assign) BOOL enableVSyncAlignedMessageLoop;
// Run the hydration process in a async thread.
@property(nonatomic, assign) BOOL enableAsyncHydration;
@property(nonatomic, assign) CGRect frame;
@property(nonatomic, nullable) id<LynxDynamicComponentFetcher> fetcher;
@property(nonatomic, assign) CGFloat fontScale;
@property(nonatomic, nullable, strong) NSMutableDictionary<NSString*, id>* lynxViewConfig;
@property(nonatomic, assign) LynxBooleanOption enableGenericResourceFetcher;

// generic resource fetcher api.
@property(nonatomic, nonnull) id<LynxGenericResourceFetcher> genericResourceFetcher;
@property(nonatomic, nonnull) id<LynxMediaResourceFetcher> mediaResourceFetcher;
@property(nonatomic, nonnull) id<LynxTemplateResourceFetcher> templateResourceFetcher;

/**
 * Pass extra data to LynxModule, the usage of data depends on module's implementation
 */
@property(nonatomic, nullable) id lynxModuleExtraData;

/**
 * You can set a virtual screen size to lynxview by this way.
 * Generally, you don't need to set it.The screen size of lynx is the real device size by default.
 * It will be useful for the split-window, this case, it can make some css properties based on rpx
 * shows better.
 * screenSize.width (dp)
 * screenSize.height (dp)
 */
@property(nonatomic, assign) CGSize screenSize;

/**
 * indicates lynx view can be debug or not
 * when switch enableDevtool is disabled and
 * switch enableDevtoolForDebuggableView is enabled
 */
@property(nonatomic, assign) BOOL debuggable;

/**
 * Control whether updateData can take effect before loadTemplate
 */
@property(nonatomic, assign) BOOL enablePreUpdateData;

/**
 * enable LynxResourceService loader injection
 */
@property(nonatomic, assign) BOOL enableLynxResourceServiceLoaderInjection;

/**
 * Set backgroundJSRuntime js engine type.
 */
@property(nonatomic, assign) LynxBackgroundJsRuntimeType backgroundJsRuntimeType;

/**
 * Only when backgroundJsRuntimeType == LynxBackgroundJsRuntimeTypeQuickjs,it will take effect.
 * Enable bytecode for quickjs.
 */
@property(nonatomic, assign) BOOL enableBytecode;

/** Only when enableBytecode is YES, it will take effect.
 * Set bytecode key for current lynxview.
 */
@property(nonatomic, strong, nullable) NSString* bytecodeUrl;

@property(nonatomic, assign) BOOL isUIRunningMode __attribute__((deprecated(
    "try to set 'threadStrategy' variable if you want to change the thread strategy for rendering"))
);

- (void)setThreadStrategyForRender:(LynxThreadStrategyForRender)threadStrategy;
- (LynxThreadStrategyForRender)getThreadStrategyForRender;

- (void)addLynxResourceProvider:(NSString* _Nonnull)resType
                       provider:(id<LynxResourceProvider> _Nonnull)provider;

/**
 * You can use the following interface to create an alias for system fonts or registered fonts
 * (fontFamily) that can be used in Lynx style sheets. This is an instance level interface and can
 * override settings in the global interface.
 */
- (void)registerFont:(UIFont* _Nonnull)font forName:(NSString* _Nonnull)name;
- (void)registerFamilyName:(NSString* _Nonnull)fontFamilyName
             withAliasName:(NSString* _Nonnull)aliasName;

@end
